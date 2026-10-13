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

#include "log.h"
#include "proc.h"
#include "proc-child.h"

#include <guacamole/assert.h>
#include <guacamole/client.h>
#include <guacamole/error.h>
#include <guacamole/mem.h>
#include <guacamole/proctitle.h>
#include <guacamole/string.h>
#include <guacamole/timestamp.h>

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#ifdef HAVE_SYS_PRCTL_H
#include <sys/prctl.h>
#endif

#ifdef HAVE_SYS_PROCCTL_H
#include <sys/procctl.h>
#endif

/**
 * Whether this platform can deliver a signal to a child process upon the death
 * of its parent. Where unsupported, guacd_parent_watch_thread() polls the
 * status of the parent process instead.
 */
#if defined(PR_SET_PDEATHSIG) || defined(PROC_PDEATHSIG_CTL)
#define GUACD_HAVE_PDEATHSIG 1
#else
#define GUACD_HAVE_PDEATHSIG 0
#endif

#if !GUACD_HAVE_PDEATHSIG
/**
 * Thread which polls for the death of the parent guacd process, requesting
 * termination of the current (child) process once the parent has died.
 *
 * @param data
 *     An intptr_t containing the PID of the parent guacd process.
 *
 * @return
 *     Always NULL.
 */
static void* guacd_parent_watch_thread(void* data) {

    guac_thread_name_set("parent-watch");

    /* Parent process has died if the current process has been re-parented (the
     * parent PID has changed) */
    pid_t parent_pid = (pid_t) (intptr_t) data;
    while (getppid() == parent_pid)
        guac_timestamp_msleep(GUACD_TIMEOUT);

    kill(getpid(), SIGTERM);

    return NULL;

}
#endif

int guacd_proc_config_open_pipe(int config_pipe[2]) {

#ifdef HAVE_PIPE2
    if (pipe2(config_pipe, O_CLOEXEC)) {
#else
    if (pipe(config_pipe)) {
#endif
        /* The caller logs this failure */
        return 1;
    }
#ifndef HAVE_PIPE2
    fcntl(config_pipe[0], F_SETFD, FD_CLOEXEC);
    fcntl(config_pipe[1], F_SETFD, FD_CLOEXEC);
#endif

    return 0;

}

int guacd_proc_config_write(int fd, const guacd_proc_config* config) {

    size_t remaining = sizeof(guacd_proc_config);
    const unsigned char* buffer = (const unsigned char*) config;

    do {

        ssize_t written;
        GUAC_RETRY_EINTR(written, write(fd, buffer, remaining));

        if (written <= 0)
            return 1;

        buffer += written;
        remaining -= written;

    } while (remaining > 0);

    return 0;

}

void guacd_connection_process(int argc, char** argv) {

    /* Logging must be reinitialized (fresh process image) */
    openlog(GUACD_LOG_NAME, LOG_PID, LOG_DAEMON);

    if (signal(SIGALRM, SIG_DFL) == SIG_ERR) {
        guacd_log(GUAC_LOG_WARNING, "Could not set handler for SIGALRM to "
                "default. The startup deadline of this connection process "
                "may not be enforced.");
    }

    if (signal(SIGTERM, SIG_DFL) == SIG_ERR) {
        guacd_log(GUAC_LOG_WARNING, "Could not set handler for SIGTERM to "
                "default. This connection process may not terminate when "
                "requested.");
    }

    sigset_t vital_signals;
    sigemptyset(&vital_signals);
    sigaddset(&vital_signals, SIGALRM);
    sigaddset(&vital_signals, SIGTERM);
    sigaddset(&vital_signals, SIGINT);
    sigaddset(&vital_signals, SIGCHLD);
    sigprocmask(SIG_UNBLOCK, &vital_signals, NULL);

    /* Until under the supervision of the watchdog, bound startup via SIGALRM */
    alarm(GUACD_SEC_TIMEOUT);

    /* Do not create a connection process that is not subject to the failsafe */
    if (getpgrp() != getpid() && setpgid(0, 0)) {
        guacd_log(GUAC_LOG_ERROR, "Connection process is not the leader of "
                "its own process group and cannot become one. Refusing to "
                "create a connection process which cannot be forcibly "
                "terminated.");
        exit(EXIT_FAILURE);
    }

    /* Ignore SIGPIPE (writes to a closed socket/pipe should not terminate the
     * process) */
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        guacd_log(GUAC_LOG_INFO, "Could not set handler for SIGPIPE to "
                "ignore. SIGPIPE may cause termination of this connection "
                "process.");
    }

    /* Ignore SIGCHLD (automatically reap children on termination) */
    if (signal(SIGCHLD, SIG_IGN) == SIG_ERR) {
        guacd_log(GUAC_LOG_INFO, "Could not set handler for SIGCHLD to "
                "ignore. Terminated children of this connection process may "
                "remain in the process table.");
    }

    /* Where supported, request that the OS dispatch SIGTERM upon the death of
     * the parent guacd process. Where NOT supported, this is handled later
     * through checking the parent PID (see usage of guacd_parent_watch_thread()
     * below). */

#if defined(PR_SET_PDEATHSIG)
    prctl(PR_SET_PDEATHSIG, SIGTERM);
#elif defined(PROC_PDEATHSIG_CTL)
    procctl(P_PID, getpid(), PROC_PDEATHSIG_CTL, &(int) { SIGTERM });
#endif

    /* Copy the protocol out of the process name before
     * guac_process_title_set() repurposes that memory */
    char* protocol = guac_strdup(argv[0] + strlen(GUACD_PROC_NAME_PREFIX));

    /* Read configuration from dedicated setup pipe */
    guacd_proc_config config;
    size_t config_length = 0;
    ssize_t read_length;
    do {
        GUAC_RETRY_EINTR(read_length, read(GUACD_PROC_CONFIG_FD,
                (char*) &config + config_length,
                sizeof(config) - config_length));
        if (read_length > 0)
            config_length += read_length;
    } while (read_length > 0 && config_length < sizeof(config));

    /* Restore close-on-exec on the socket to the parent */
    fcntl(GUACD_PROC_SOCKET_FD, F_SETFD, FD_CLOEXEC);

    /* Nothing can be served without a complete configuration */
    if (protocol == NULL || read_length < 0 || config_length != sizeof(config)
            || config.connection_id[0] == '\0') {
        guacd_log(GUAC_LOG_ERROR, "Connection process was started without a "
                "usable protocol and configuration.");
        exit(EXIT_FAILURE);
    }

    /* Ensure the received ID is terminated regardless of its contents */
    config.connection_id[sizeof(config.connection_id) - 1] = '\0';

    /* Re-arm the startup timeout alarm with what remains of the deadline. If
     * that deadline has already passed, the parent is no longer waiting. */
    guac_timestamp startup_remaining = config.startup_deadline - guac_timestamp_current();
    if (startup_remaining <= 0) {
        guacd_log(GUAC_LOG_ERROR, "Connection process did not start within "
                "its startup deadline.");
        exit(EXIT_FAILURE);
    }

    /* Round up to nearest whole second (a value of 0 disables the alarm) */
    guac_timestamp startup_seconds = (startup_remaining + 999) / 1000;
    GUAC_ASSERT(startup_seconds <= GUACD_SEC_TIMEOUT);

    alarm((unsigned int) startup_seconds);

    /* Arrival of the per-connection child process (THIS process) is announced to
     * the parent guacd by closing this descriptor */
    close(GUACD_PROC_CONFIG_FD);

    /* The parent may already have died (before the requested signal could take
     * effect), in which case this process has been reparented */
    if (getppid() != config.parent_pid) {
        guacd_log(GUAC_LOG_ERROR, "Parent guacd process terminated before "
                "the connection could be served.");
        exit(EXIT_FAILURE);
    }

#if !GUACD_HAVE_PDEATHSIG
    /* Poll for the death of the parent from here on */
    pthread_t parent_watch_thread;
    if (pthread_create(&parent_watch_thread, NULL, guacd_parent_watch_thread,
                (void*) (intptr_t) config.parent_pid)) {
        guacd_log(GUAC_LOG_ERROR, "Unable to start a thread to watch for "
                "the death of the parent guacd process. Refusing to serve a "
                "connection which could outlive guacd.");
        exit(EXIT_FAILURE);
    }
    pthread_detach(parent_watch_thread);
#endif

    /* Match the log verbosity configured for the parent daemon */
    guacd_log_level = config.log_level;

    /* Build the process entry and its guac_client */
    guacd_proc* proc = guac_mem_zalloc(sizeof(guacd_proc));
    if (proc == NULL) {
        guacd_log(GUAC_LOG_ERROR, "Unable to allocate connection process.");
        exit(EXIT_FAILURE);
    }

    proc->pid = 0;
    proc->fd_socket = GUACD_PROC_SOCKET_FD;

    proc->client = guac_client_alloc();
    if (proc->client == NULL) {
        guacd_log_guac_error(GUAC_LOG_ERROR, "Unable to create client");
        exit(EXIT_FAILURE);
    }

    /* Adopt the connection identifier already assigned by the parent */
    guac_mem_free(proc->client->connection_id);
    proc->client->connection_id = guac_strdup(config.connection_id);
    if (proc->client->connection_id == NULL) {
        guacd_log(GUAC_LOG_ERROR, "Unable to allocate connection identifier.");
        exit(EXIT_FAILURE);
    }

    proc->client->log_handler = guacd_client_log;

    /* Serve the connection */
    guacd_exec_proc(proc, protocol);
    GUAC_ASSERT(0); /* guacd_exec_proc() does not return */

}
