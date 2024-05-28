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

#ifndef GUACD_MOVE_HANDLE_H
#define GUACD_MOVE_HANDLE_H

#include <guacamole/id.h>
#include <handleapi.h>

/*
 * The required prefix for all pipe names in Windows.
 */
#define PIPE_NAME_PREFIX "\\\\.\\pipe\\"

/* 
 * The length of a named pipe as used by guacamole. Every pipe name will consist
 * of PIPE_NAME_PREFIX, plus a the length of a UUID as returned from
 * guac_generate_id(), plus a null-terminator.
 */
#define GUAC_PIPE_NAME_LENGTH (strlen(PIPE_NAME_PREFIX) + GUAC_UUID_LEN + 1)

/**
 * Sends the given pipe name along the given socket. Returns non-zero on success, 
 * zero on error, just as a normal call to sendmsg() would. If an error does occur, 
 * GetLastError() will return the appropriate error.
 *
 * @param sock
 *     The file descriptor of an open UNIX domain socket along which the pipe
 *     name specified by pipe_name should be sent.
 *
 * @param pipe_name
 *     The null-terminated name of the pipe to send across the socket. The name
 *     MUST be GUAC_PIPE_NAME_LENGTH characters long, and end with a null
 *     terminator.
 *
 * @return
 *     Non-zero if the send operation succeeded, zero on error.
 */
int guacd_send_pipe(int sock, char* pipe_name);

/**
 * Waits for a pipe name on the given socket, returning a handle to the client
 * end of the named pipe with that name. The pipe name must have been sent via 
 * guacd_send_pipe_name. If an error occurs, NULL is returned, and GetLastError()
 * will return the appropriate error.
 *
 * @param sock
 *     The file descriptor of an open UNIX domain socket along which the file
 *     handle will be sent (by guacd_send_handle()).
 *
 * @return
 *     The handle to the client end of the named pipe if the operation succeeded, 
 *     NULL otherwise.
 */
HANDLE guacd_recv_pipe(int sock);

#endif

