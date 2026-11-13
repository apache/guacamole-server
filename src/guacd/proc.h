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

#ifndef GUACD_PROC_H
#define GUACD_PROC_H

#include <guacamole/client.h>
#include <guacamole/parser.h>

#include <sys/time.h>
#include <unistd.h>

/**
 * The number of milliseconds to wait for messages in any phase before
 * timing out and closing the connection with an error.
 */
#define GUACD_TIMEOUT 15000

/**
 * The number of microseconds to wait for messages in any phase before
 * timing out and closing the connection with an error. This is always
 * equal to GUACD_TIMEOUT * 1000.
 */
#define GUACD_USEC_TIMEOUT (GUACD_TIMEOUT*1000)

/**
 * The number of seconds to wait for messages in any phase before timing out
 * and closing the connection with an error. This is always equal to
 * GUACD_TIMEOUT / 1000.
 */
#define GUACD_SEC_TIMEOUT (GUACD_TIMEOUT/1000)

/**
 * The number of milliseconds to wait for messages in any phase before timing
 * out and closing the connection with an error, as a struct timeval.
 */
#define GUACD_TIMEOUT_TIMEVAL ((struct timeval) { \
    .tv_sec  =  GUACD_TIMEOUT / 1000,             \
    .tv_usec = (GUACD_TIMEOUT % 1000) * 1000      \
})

/**
 * The number of milliseconds to wait for a connection process to be entirely
 * cleaned up following disconnect. If the cleanup operation does not complete
 * within this period of time, the process will be forcibly terminated.
 */
#define GUACD_CLEANUP_TIMEOUT 5000

/**
 * Process information of the internal remote desktop client.
 */
typedef struct guacd_proc {

    /**
     * The process ID of the client. This will only be available to the
     * parent process. The child process will see this as 0.
     */
    pid_t pid;

    /**
     * The file descriptor of the UNIX domain socket to use for sending and
     * receiving file descriptors of new users. This parent will see this
     * as the file descriptor for communicating with the child and vice
     * versa.
     */
    int fd_socket;

    /**
     * The actual client instance. This will be visible to both child and
     * parent process, but only the child will have a full guac_client
     * instance, containing handlers from the plugin, etc.
     *
     * The parent process will receive a skeleton guac_client, containing only
     * a proper connection_id and logging handlers. The actual
     * protocol-specific handling will be absent.
     */
    guac_client* client;

    /**
     * The watchdog of the client. If guac_client_watchdog_start() has not yet
     * been called by the connection process,  this will be NULL.
     */
    guac_client_watchdog* watchdog;

} guacd_proc;

/**
 * Returns whether the given connection process is still running.
 *
 * @param proc
 *     The connection process to test.
 *
 * @return
 *     Non-zero if the process is still running, zero if it has terminated or
 *     can no longer be observed.
 */
int guacd_proc_is_running(guacd_proc* proc);

/**
 * Waits (blocks) until the given connection process has terminated. No process
 * is reaped by this function.
 *
 * @param proc
 *     The connection process to await the termination of.
 */
void guacd_proc_await_exit(guacd_proc* proc);

/**
 * Returns a file descriptor referring to the executable of the running guacd
 * instance. It is intended that this file descriptor remain open for the
 * lifetime of guacd, to ensure subprocesses can be reliably exec'd without
 * breaking the ABI, even if guacd is deleted/upgraded while running. If the
 * executable cannot be located, -1 is returned, and errno is set appropriately.
 *
 * @param invoked_path
 *     The path and name provided when guacd was invoked (argv[0]). This value
 *     is used as a fallback when the executable cannot be located by
 *     platform-specific means. If provided, this value must contain at least
 *     one directory reference (at least one "/") to be usable as a fallback,
 *     and will be resolved relative to the current working directory. This
 *     value may be NULL.
 *
 * @return
 *     A file descriptor referring to the running guacd executable, or -1 if the
 *     executable could not be located.
 */
int guacd_open_exe(const char* invoked_path);

/**
 * Creates a new background process for handling the given protocol, returning
 * a structure allowing communication with and monitoring of the process
 * created. Within the child process, this function does not return - the
 * entire child process is replaced via exec() instead, serving the connection
 * through guacd_connection_process().
 *
 * @param protocol
 *     The protocol for which this process is client being created.
 *
 * @param exe_fd
 *     A file descriptor referring to the guacd executable, as opened by
 *     guacd_open_exe().
 *
 * @return
 *     A newly-allocated process structure pointing to the file descriptor of
 *     the background process specific to the specified protocol, or NULL of
 *     the process could not be created.
 */
guacd_proc* guacd_create_proc(const char* protocol, int exe_fd);

/**
 * Frees the given process structure and any associated resources, including
 * reaping the underlying process. The relevant process and its group MUST
 * already have been terminated.
 *
 * @see guacd_proc_stop()
 *
 * @note
 *     This should be invoked only by the thread which created the guacd_proc,
 *     and only once no other thread may still hold a reference to that
 *     guacd_proc (it is no longer in the process map).
 *
 * @param proc
 *     The process structure to free.
 */
void guacd_proc_free(guacd_proc* proc);

/**
 * Starts protocol-specific handling on the given process by loading the client
 * plugin for that protocol. This function does NOT return. It initializes the
 * process with protocol-specific handlers and then runs until the guacd_proc's
 * fd_socket is shut down or closed, adding any file descriptors received along
 * fd_socket as new users.
 *
 * @param proc
 *     The process that any new users received along fd_socket should be added
 *     to (after the process has been initialized for the given protocol).
 *
 * @param protocol
 *     The protocol to initialize the given process for.
 */
void guacd_exec_proc(guacd_proc* proc, const char* protocol);

/**
 * Signals the given process to stop accepting new users and clean up. This
 * will eventually cause the child process to exit.
 *
 * @param proc
 *     The process to stop.
 */
void guacd_proc_stop(guacd_proc* proc);

#endif

