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

#include <guacamole/timestamp-types.h>

#include <sys/types.h>

/**
 * The prefix of the process name (argv[0]) with which guacd_create_proc()
 * re-execs guacd to serve a single connection. A process whose argv[0] begins
 * with this prefix serves the protocol named by the remainder of argv[0].
 */
#define GUACD_PROC_NAME_PREFIX "guacd-connection-"

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
 * Serves a single connection as a re-exec'd per-connection child process,
 * taking the protocol from the suffix of argv[0] (see GUACD_PROC_NAME_PREFIX)
 * and any other configuration information from GUACD_PROC_CONFIG_FD.
 *
 * This is the entry point for the process image created by guacd_create_proc()
 * and does not return.
 *
 * @param argc
 *     The number of arguments within argv, exactly as passed to main().
 *
 * @param argv
 *     The arguments received by the child process, exactly as passed to main().
 */
void guacd_connection_process(int argc, char** argv);

#endif

