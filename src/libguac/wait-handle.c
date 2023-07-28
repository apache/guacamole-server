/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "config.h"

#include <stdio.h>

#include <errhandlingapi.h>
#include <windef.h>
#include <handleapi.h>
#include <winbase.h>

int guac_wait_for_handle(HANDLE handle, int usec_timeout) {

    /* Create an event to be used to signal comm events */
    HANDLE event = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (event == NULL) 
        return GetLastError();

    OVERLAPPED overlapped = { 0 };
    overlapped.hEvent = event;

    /* Request to wait for new data to be available */
    char buff[1]; 
    if (!ReadFile(handle, &buff, 0, NULL, &overlapped)) {
        
        DWORD error = GetLastError();

        /* ERROR_IO_PENDING is expected in overlapped mode */
        if (error != ERROR_IO_PENDING) {
            CloseHandle(event);
            return error;
        }

    }

    int millis = (usec_timeout + 999) / 1000;
    
    DWORD result = WaitForSingleObject(event, millis);
    
    /* The wait attempt failed */ 
    if (result == WAIT_FAILED) {
        CloseHandle(event);
        return GetLastError();
    }

    /* The event was signalled, which should indicate data is ready */
    else if (result == WAIT_OBJECT_0) {
        CloseHandle(event);
        return 0;
    }

    /* 
     * If the event didn't trigger and the wait didn't fail, data just isn't 
     * ready yet.
     */
    CloseHandle(event);
    return -1;
    
}
