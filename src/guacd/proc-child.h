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

#ifndef GUACD_PROC_CHILD_H
#define GUACD_PROC_CHILD_H

#include "proc-title.h"

#include <guacamole/plugin-constants.h>
#include <guacamole/timestamp-types.h>

#include <sys/types.h>

/**
 * The maximum number of bytes required to compose the process name (argv[0]) of
 * a connection process, including any padding for the generated process title.
 */
#define GUACD_PROC_NAME_LENGTH (                                \
                                                                \
      sizeof(GUACD_PROC_TITLE_PREFIX)  - 1 /* "guacd("       */ \
    +        GUAC_PROTOCOL_NAME_LIMIT - 1  /* protocol name  */ \
    + sizeof(GUACD_PROC_TITLE_SUFFIX)  - 1 /* ")"            */ \
    +        GUACD_PROC_TITLE_BUFSIZE      /* title padding  */ \
                                                                \
)

/**
 * The fixed file descriptor that the per-connection child process inherits as
 * the socket to the parent guacd process.
 */
#define GUACD_PROC_SOCKET_FD 3

/**
 * The fixed file descriptor that the per-connection child process inherits as
 * the means of receiving its configuration (a single guacd_proc_config
 * structure). The parent observes arrival of the child process via the child
 * reading that configuration and closing this file descriptor.
 *
 * @see guacd_proc_config
 */
#define GUACD_PROC_CONFIG_FD 4

/**
 * The configuration of a per-connection process, received from the parent guacd
 * process as the raw structure's bytes written over GUACD_PROC_CONFIG_FD.
 */
typedef struct guacd_proc_config {

    /**
     * The maximum log level to be logged by the child process. This value is
     * copied from guacd.
     */
    int log_level;

    /**
     * The process ID of the parent guacd process.
     */
    pid_t parent_pid;

    /**
     * The connection ID of the connection being served by this process.
     */
    char connection_id[256];

    /**
     * The time after which the connection process must have reached the point
     * of supervising itself. If the connection process fails to complete
     * startup by this point in time, the connection process is presumed
     * to be malfunctioning and is killed.
     */
    guac_timestamp startup_deadline;

} guacd_proc_config;

/**
 * Opens the dedicated pipe that the parent guacd process uses to send a
 * connection process its configuration information. If the pipe cannot be
 * opened, a non-zero value is returned and errno is set appropriately.
 *
 * @param config_pipe
 *     A two-element array to receive the read and write ends of the pipe,
 *     as produced by pipe().
 *
 * @return
 *     Zero if the pipe was successfully opened, non-zero otherwise.
 */
int guacd_proc_config_open_pipe(int config_pipe[2]);

/**
 * Writes the given configuration to the given file descriptor. It is expected
 * that the file descriptor provided will be the write end of the pipe opened by
 * guacd_proc_config_open_pipe(). If the configuration cannot be written,
 * non-zero is returned and errno is set appropriately.
 *
 * @see guacd_proc_config_open_pipe()
 *
 * @param fd
 *     The file descriptor to write the configuration to.
 *
 * @param config
 *     The configuration to provide to the connection process.
 *
 * @return
 *     Zero if the configuration was sent, non-zero otherwise.
 */
int guacd_proc_config_write(int fd, const guacd_proc_config* config);

/**
 * Extracts the protocol name from the given guacd child process name, storing
 * the result in the given buffer as a null-terminated string. If the process
 * name does not conform to the naming pattern used for child connection
 * processes, a non-zero value is returned and the buffer is left untouched.
 * 
 * @param process_name
 *     The name of the current process, as received in argv[0].
 * 
 * @param buffer
 *     The buffer that should receive the protocol name, if present.
 *
 * @param buffer_length
 *     The number of bytes available in the buffer.
 * 
 * @return
 *     Zero if the process name refers to a connection process and the protocol
 *     has been stored in the given buffer, non-zero otherwise.
 */
int guacd_proc_name_get_protocol(const char* process_name,
    char* buffer, size_t buffer_length);

/**
 * Serves a single connection for the given protocol as a re-exec'd
 * per-connection child process. Any other configuration information is taken
 * from GUACD_PROC_CONFIG_FD.
 *
 * This is the entry point for the process image created by guacd_create_proc()
 * and does not return.
 *
 * @param protocol
 *     The name of the protocol to be served, as returned by
 *     guacd_proc_name_get_protocol().
 */
void guacd_connection_process(const char* protocol);

#endif

