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

#ifndef GUAC_HANDLE_HELPERS_H
#define GUAC_HANDLE_HELPERS_H

#include <handleapi.h>

/**
 * Attempt to read the provided count of bytes from the provided handle, into the
 * provided buffer. If the read is successful, 0 will be returned, and the actual 
 * number of bytes written to the buffer will be saved to the provided 
 * num_bytes_read pointer. If an error occurs while attempting to read from the
 * handle, or while waiting on the results of the read attempt, the error code
 * (as returned by GetLastError()) will be returned.
 * 
 * @param handle
 *     The handle to read from. This handle MUST have been opened in overlapped
 *     mode.
 * 
 * @param buffer
 *     The buffer to write the data into. It must be at least `count` bytes.
 * 
 * @param count
 *     The maximum number of bytes to read from the handle.
 * 
 * @param num_bytes_read
 *     The actual number of bytes read from the handle. This value is valid only
 *     if this function returns successfully. This value may be less than `count`.
 * 
 * @return
 *     Zero on success, or the failure code (as returned by GetLastError()) if
 *     the read attempt, or the wait on that read attempt fails.
 */
int guac_read_from_handle(
        HANDLE handle, void* buffer, DWORD count, DWORD* num_bytes_read);

/**
 * Attempt to wrtie the provided count of bytes to the provided handle, from the
 * provided buffer. If the write is successful, 0 will be returned, and the actual 
 * number of bytes written to the handle will be saved to the provided 
 * num_bytes_written pointer. If an error occurs while attempting to write to the
 * handle, or while waiting on the results of the write attempt, the error code
 * (as returned by GetLastError()) will be returned.
 * 
 * @param handle
 *     The handle to write to. This handle MUST have been opened in overlapped
 *     mode.
 * 
 * @param buffer
 *     The buffer to write to the handle. It must be at least `count` bytes.
 * 
 * @param count
 *     The maximum numer of bytes to write to the handle.
 * 
 * @param num_bytes_written
 *     The actual number of bytes written to the handle. This value is valid only
 *     if this function returns successfully. This value may be less than `count`.
 * 
 * @return
 *     Zero on success, or the failure code (as returned by GetLastError()) if
 *     the write attempt, or the wait on that write attempt fails.
 */
int guac_write_to_handle(
        HANDLE handle, const void* buffer, DWORD count, DWORD* num_bytes_written);

#endif