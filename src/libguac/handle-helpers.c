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


#include "guacamole/error.h"
#include "guacamole/handle-helpers.h"

#include <errhandlingapi.h>
#include <windef.h>
#include <handleapi.h>
#include <winbase.h>

#include <stdio.h>

int guac_read_from_handle(
        HANDLE handle, void* buffer, DWORD count, DWORD* num_bytes_read) {
    
    /* 
     * Overlapped structure and associated event for waiting on async call.
     * The event isn't used by this function, but is required in order to
     * reliably wait on an async operation anyway - see
     * https://learn.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-getoverlappedresult#remarks.
     */
    OVERLAPPED overlapped = { 0 };
    overlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    /* Attempt to start the async read operation */
    if (!ReadFile(handle, buffer, count, NULL, &overlapped)) {
        
        DWORD error = GetLastError();

        /* 
         * If an error other than the expected ERROR_IO_PENDING happens,
         * return it as the error code immediately.
         */ 
        if (error != ERROR_IO_PENDING) {
            return error;
        }
        
    }

    /* 
     * Wait on the result of the read. If any error occurs when waiting,
     * return the error.
     */
    if (!GetOverlappedResult(handle, &overlapped, num_bytes_read, TRUE)) 
        return GetLastError();
    
    /* No errors occured, so the read was successful */
    return 0;

}

int guac_write_to_handle(
        HANDLE handle, const void* buffer, DWORD count, DWORD* num_bytes_written) {
    
    /* 
     * Overlapped structure and associated event for waiting on async call.
     */
    OVERLAPPED overlapped = { 0 };
    overlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    /* Attempt to start the async write operation */
    if (!WriteFile(handle, buffer, count, NULL, &overlapped)) {
        
        DWORD error = GetLastError();

        /* 
         * If an error other than the expected ERROR_IO_PENDING happens,
         * return it as the error code immediately.
         */ 
        if (error != ERROR_IO_PENDING) 
            return error;
        
    }

    /* 
     * Wait on the result of the write. If any error occurs when waiting,
     * return the error.
     */
    if (!GetOverlappedResult(handle, &overlapped, num_bytes_written, TRUE)) 
        return GetLastError();

    /* No errors occured, so the write was successful */
    return 0;
    
}