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

#ifndef GUAC_WAIT_HANDLE_H
#define GUAC_WAIT_HANDLE_H

#include <handleapi.h>

/**
 * Waits for data to be available for reading on a given file handle. Returns
 * zero if data is available, a negative value if the wait timed out without 
 * data being available, or a positive Windows error code if the wait failed.
 *
 * @param handle
 *     The file handle to wait for.
 *
 * @param usec_timeout
 *     The maximum number of microseconds to wait for data, or -1 to
 *     potentially wait forever.
 *
 * @return
 *     Zero if data is available for reading, negative if the timeout elapsed
 *     and no data is available, or a positive Windows error code if an error
 *     occurs.
 */
int guac_wait_for_handle(HANDLE handle, int usec_timeout);

#endif
