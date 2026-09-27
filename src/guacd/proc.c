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
#include "move-fd.h"
#include "proc.h"
#include "proc-child.h"
#include "proc-map.h"
#include "proc-title.h"

#include <guacamole/client.h>
#include <guacamole/error.h>
#include <guacamole/mem.h>
#include <guacamole/parser.h>
#include <guacamole/plugin.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/string.h>
#include <guacamole/thread.h>
#include <guacamole/timestamp.h>
#include <guacamole/user.h>

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>

#ifdef __FreeBSD__
#include <sys/sysctl.h>
#endif

extern char** environ;

/**
 * Parameters for the user thread.
 */
typedef struct guacd_user_thread_params {

    /**
     * The client of the connection being joined.
     */
    guac_client* client;

    /**
     * The file descriptor of the joining user's socket.
     */
    int fd;

    /**
     * Whether the joining user is the connection owner.
     */
    int owner;

} guacd_user_thread_params;

/**
 * Handles a user's entire connection and socket lifecycle.
 *
 * @param data
 *     A pointer to a guacd_user_thread_params structure describing the user's
 *     associated file descriptor, whether that user is the connection owner
 *     (the first person to join), as well as the process associated with the
 *     connection being joined.
 *
 * @return
 *     Always NULL.
 */
static void* guacd_user_thread(void* data) {

    guacd_user_thread_params* params = (guacd_user_thread_params*) data;
    guac_client* client = params->client;

    /* Bound how long a read from or write to this user may block (a full kernel
     * buffer may otherwise block cleanup) */
    if (setsockopt(params->fd, SOL_SOCKET, SO_SNDTIMEO,
            &GUACD_TIMEOUT_TIMEVAL, sizeof(GUACD_TIMEOUT_TIMEVAL)))
        guacd_log(GUAC_LOG_WARNING, "Unable to set write timeout on user "
                "socket: %s. A user whose connection is blocked may delay "
                "cleanup of this connection until it is otherwise closed.",
                strerror(errno));

    if (setsockopt(params->fd, SOL_SOCKET, SO_RCVTIMEO,
            &GUACD_TIMEOUT_TIMEVAL, sizeof(GUACD_TIMEOUT_TIMEVAL)))
        guacd_log(GUAC_LOG_WARNING, "Unable to set read timeout on user "
                "socket: %s.", strerror(errno));

    /* Get guac_socket for user's file descriptor */
    guac_socket* socket = guac_socket_open(params->fd);
    if (socket == NULL) {

        int owner = params->owner;
        close(params->fd);
        guac_mem_free(params);

        if (owner) {
            guac_client_stop(client);
            guacd_log_guac_error(GUAC_LOG_ERROR, "Unable to create the "
                    "connection owner's socket. Stopping connection.");
        }
        else
            guacd_log_guac_error(GUAC_LOG_ERROR, "Unable to create the "
                    "joining user's socket. The user has been dropped.");

        guac_client_watchdog_notify_user_left(client);
        return NULL;

    }

    /* Create skeleton user */
    guac_user* user = guac_user_alloc();
    user->socket = socket;
    user->client = client;
    user->owner  = params->owner;

    /* Handle user connection from handshake until disconnect/completion */
    guac_user_handle_connection(user, GUACD_USEC_TIMEOUT);

    /* Clean up */
    guac_client_watchdog_notify_operation_start(client);
    guac_socket_free(socket);
    guac_user_free(user);
    guac_mem_free(params);
    guac_client_watchdog_notify_operation_end(client);

    /* NOTE: The user count reaching zero here will begin concurrent teardown of
     * the client, thus no further calls referencing the client may be made after
     * this */
    guac_client_watchdog_notify_user_left(client);

    return NULL;

}

/**
 * Begins a new user connection under a given process, using the given file
 * descriptor. The connection will be managed by a separate and detached thread
 * which is started by this function.
 *
 * @param proc
 *     The process that the user is being added to.
 *
 * @param fd
 *     The file descriptor associated with the user's network connection to
 *     guacd.
 *
 * @param owner
 *     Non-zero if the user is the owner of the connection being joined (they
 *     are the first user to join), or zero otherwise.
 */
static void guacd_proc_add_user(guacd_proc* proc, int fd, int owner) {

    guac_client* client = proc->client;

    guacd_user_thread_params* params = guac_mem_alloc(sizeof(guacd_user_thread_params));
    if (params == NULL) {

        close(fd);

        if (owner) {
            guacd_proc_stop(proc);
            guacd_log(GUAC_LOG_ERROR, "Unable to allocate the parameters of "
                    "the connection owner's thread. Stopping connection.");
        }
        else
            guacd_log(GUAC_LOG_ERROR, "Unable to allocate the parameters of "
                    "a joining user's thread. The user has been dropped.");

        return;

    }

    params->client = client;
    params->fd = fd;
    params->owner = owner;

    /* Notify that a new user is starting, but bail out if this is refused due to
     * all users having already left */
    if (guac_client_watchdog_notify_user_joined(client)) {
        close(fd);
        guac_mem_free(params);
        guacd_log(GUAC_LOG_INFO, "Joining user dropped - connection has already ended.");
        return;
    }

    /* Start user thread */
    guac_thread user_thread;
    if (guac_thread_create(&user_thread, guacd_user_thread, params, "user-conn")) {

        close(fd);
        guac_mem_free(params);

        if (owner) {
            guacd_proc_stop(proc);
            guacd_log_guac_error(GUAC_LOG_ERROR, "Stopping connection: Unable "
                    "to start thread for connection owner");
        }
        else
            guacd_log_guac_error(GUAC_LOG_ERROR, "User dropped: Unable to "
                    "start thread for joining user");

        guac_client_watchdog_notify_user_left(client);
        return;

    }

    guac_thread_detach(&user_thread);

}

/**
 * Forcibly kills all processes within the current process group, including the
 * current process and all child processes. This function is only safe to call
 * if the process group ID has been correctly set. Calling this function within
 * a process which does not have a PGID separate from the main guacd process
 * can result in guacd itself being terminated.
 */
static void guacd_kill_current_proc_group() {

    /* Forcibly kill all children within process group */
    if (kill(0, SIGKILL))
        guacd_log(GUAC_LOG_WARNING, "Unable to forcibly terminate "
                "client process: %s ", strerror(errno));

}

/**
 * The watchdog of the current per-connection process. This value is assigned
 * only once and only within a per-connection process. This reference to the
 * watchdog exists as a means of requesting termination from within a signal
 * handler.
 *
 * @see signal_stop_handler()
 */
static guac_client_watchdog* guacd_proc_watchdog = NULL;

/**
 * Thread which waits for teardown of the given process' connection to begin.
 * Once teardown has begun, this thread shuts down the internal socket of the
 * guacd_proc to unblock that process' main loop and allow cleanup to proceed.
 *
 * @param data
 *     The guacd_proc whose main loop should be unblocked.
 *
 * @return
 *     Always NULL.
 */
static void* guacd_teardown_watch_thread(void* data) {

    guacd_proc* proc = (guacd_proc*) data;

    guac_client_watchdog_await_teardown(proc->watchdog);
    shutdown(proc->fd_socket, SHUT_RDWR);

    return NULL;

}

/**
 * A signal handler that will be invoked when a signal is caught telling this
 * guacd process to immediately exit.
 *
 * @param signal
 *     The signal that was received. Unused in this function since only
 *     signals that should result in stopping the proc should invoke this.
 */
static void signal_stop_handler(int signal) {

    /* NOTE: Requesting the process to stop is done via the watchdog here, as
     * guacd_proc_stop() is not async-signal-safe */
    guac_client_watchdog_stop_connection(guacd_proc_watchdog);

}

/**
 * Client information handler which updates the title of the current process
 * according to the details provided.
 *
 * @param client
 *     The client serving the connection described by the given
 *     guac_client_info.
 *
 * @param info
 *     A guac_client_info structure describing the details of the connection
 *     being served.
 */
static void guacd_client_info(guac_client* client, const guac_client_info* info) {
    guacd_proc_title_set_connection(NULL, info);
}

void guacd_exec_proc(guacd_proc* proc, const char* protocol) {

    /* Label the new child process with at least the protocol */
    guacd_proc_title_set_connection(protocol, NULL);

    /* Thread name is separate from process title (but with insufficient space
     * for connection details) */
    guac_thread_self_set_name(GUACD_PROC_TITLE_PREFIX "%s" GUACD_PROC_TITLE_SUFFIX, protocol);

    /* Relabel the process with additional details once they have been supplied
     * with guac_client_set_info() */
    guac_client* client = proc->client;
    client->info_handler = guacd_client_info;

    /* Start watchdog as early as possible, before loading the plugin */
    proc->watchdog = guacd_proc_watchdog = guac_client_watchdog_start(client,
            GUACD_TIMEOUT, GUACD_CLEANUP_TIMEOUT);
    if (proc->watchdog == NULL) {
        guacd_log(GUAC_LOG_ERROR, "Unable to start the watchdog thread. "
                "Refusing to serve the connection unsupervised.");

        /* No watchdog supervises this path, so the exit must not block */
        _exit(EXIT_FAILURE);
    }

    guac_client_watchdog_notify_operation_start(client);
    int awaiting_first_user = 1;

    /* The startup deadline is no longer needed - the watchdog takes over from
     * here */
    alarm(0);

    /* Unblock recvmsg() in the main loop once teardown starts */
    guac_thread teardown_watch_thread;
    if (guac_thread_create(&teardown_watch_thread, guacd_teardown_watch_thread, proc, "teardown-watch")) {
        guacd_log(GUAC_LOG_ERROR, "Unable to start the teardown watch thread. "
                "Refusing to serve the connection unsupervised.");
        _exit(EXIT_FAILURE);
    }

    /* Init client for selected protocol */
    if (guac_client_load_plugin(client, protocol)) {

        /* Log error */
        if (guac_error == GUAC_STATUS_NOT_FOUND)
            guacd_log(GUAC_LOG_WARNING,
                    "Support for protocol \"%s\" is not installed", protocol);
        else
            guacd_log_guac_error(GUAC_LOG_ERROR,
                    "Unable to load client plugin");

        goto cleanup_client;
    }

    /* The first file descriptor is the owner */
    int owner = 1;

    /* Enable keep alive on the broadcast socket */
    guac_socket_require_keep_alive(client->socket);

    /* Clean up and exit if SIGINT or SIGTERM signals are caught */
    struct sigaction signal_stop_action = {
        .sa_handler = signal_stop_handler,
        .sa_flags = SA_RESTART
    };
    sigaction(SIGINT, &signal_stop_action, NULL);
    sigaction(SIGTERM, &signal_stop_action, NULL);

    /* Add each received file descriptor as a new user */
    for (;;) {

        /* errno must describe this call and no earlier one */
        errno = 0;

        int received_fd = guacd_recv_fd(proc->fd_socket);
        if (received_fd == -1) {

            /* Being out of file descriptors prevents this user from joining but
             * says nothing of those already served */
            if (errno == EMFILE || errno == ENFILE) {
                guacd_log(GUAC_LOG_ERROR, "Unable to accept %s: %s",
                        owner ? "the owner of this connection" : "new user",
                        strerror(errno));
                if (!owner)
                    continue;
            }

            break;

        }

        if (awaiting_first_user) {
            guac_client_watchdog_notify_operation_end(client);
            awaiting_first_user = 0;
        }

        guacd_proc_add_user(proc, received_fd, owner);

        /* Future file descriptors are not owners */
        owner = 0;

    }

cleanup_client:

    if (awaiting_first_user)
        guac_client_watchdog_notify_operation_end(client);

    /* Connection has ended and we are preparing to free the client */
    guac_client_watchdog_notify_teardown_start(proc->watchdog);
    guac_thread_join(&teardown_watch_thread, NULL);

    /* Request client to stop/disconnect */
    guac_client_stop(client);

    /* Cancel outstanding I/O and wait for all users to be disconnected */
    guac_socket_shutdown(client->socket);
    guac_socket_shutdown(client->pending_socket);
    guac_socket_shutdown(client->socket);
    guac_client_watchdog_await_all_users(proc->watchdog);

    /* The client can now safely be freed */
    guacd_log(GUAC_LOG_DEBUG, "Requesting termination of client...");
    guac_client_free(client);
    guacd_log(GUAC_LOG_DEBUG, "Client terminated successfully.");

    /* Verify whether children were all properly reaped */
    pid_t child_pid;
    while (1) {
        GUAC_RETRY_EINTR(child_pid, waitpid(0, NULL, WNOHANG));

        if (child_pid <= 0)
            break;

        guacd_log(GUAC_LOG_DEBUG, "Automatically reaped unreaped "
                "(zombie) child process with PID %i.", child_pid);
    }

    /* If running children remain, warn and forcibly kill */
    if (child_pid == 0) {
        guacd_log(GUAC_LOG_WARNING, "Client reported successful termination, "
                "but child processes remain. Forcibly terminating client and "
                "child processes.");
        guacd_kill_current_proc_group();
    }

    /* Free up all internal resources outside the client */
    close(proc->fd_socket);
    guac_client_watchdog* watchdog = proc->watchdog;
    guac_mem_free(proc);

    guac_client_watchdog_notify_teardown_end(watchdog);

    /* Cannot risk exit() destructors running here, as a malfunctioning plugin
     * may hold locks that prevent destruction from completing */
    _exit(EXIT_SUCCESS);

}

int guacd_open_exe(const char* invoked_path) {

    int fd;

#ifdef __FreeBSD__

    /* FreeBSD provides the path of the running executable via sysctl, without
     * requiring /proc (which is not mounted by default) */
    char path[PATH_MAX];
    size_t size = sizeof(path);
    int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1 };
    if (sysctl(mib, 4, path, &size, NULL, 0) == 0) {
        fd = open(path, O_RDONLY | O_CLOEXEC);
        if (fd >= 0)
            return fd;
    }

#else

    /* Leverage /proc on other platforms */
    fd = open("/proc/self/exe", O_RDONLY | O_CLOEXEC);
    if (fd >= 0)
        return fd;

#endif

    /* Fall back to deriving from the filename used to start guacd */
    if (invoked_path != NULL && strchr(invoked_path, '/') != NULL) {

        char resolved[PATH_MAX];
        if (realpath(invoked_path, resolved) == NULL)
            return -1;

        fd = open(resolved, O_RDONLY | O_CLOEXEC);
        if (fd < 0)
            return -1;

        /* The path must at least name a regular file */
        struct stat exe_stat;
        if (fstat(fd, &exe_stat) || !S_ISREG(exe_stat.st_mode)) {
            close(fd);
            errno = EACCES;
            return -1;
        }

        return fd;

    }

    return -1;

}

void guacd_proc_free(guacd_proc* proc) {

    close(proc->fd_socket);

    if (proc->client != NULL)
        guac_client_free(proc->client);

    /* Reap the process and any remaining members of its process group */
    if (proc->pid > 0) {

        pid_t reaped;
        GUAC_RETRY_EINTR(reaped, waitpid(proc->pid, NULL, 0));
        do {
            GUAC_RETRY_EINTR(reaped, waitpid(-proc->pid, NULL, 0));
        } while (reaped > 0);

    }

    guac_mem_free(proc);

}

guacd_proc* guacd_create_proc(const char* protocol, int exe_fd) {

    int sockets[2];

    /* Open UNIX socket pair, close-on-exec. The child's end survives its
     * re-exec only through its move onto GUACD_PROC_SOCKET_FD, which clears the
     * flag. */
    if (socketpair(AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC, 0, sockets) < 0) {
        guacd_log(GUAC_LOG_ERROR, "Error opening socket pair: %s", strerror(errno));
        return NULL;
    }

    int parent_socket = sockets[0];
    int child_socket = sockets[1];

    /* Bound how long sending a user to the connection process may block (a
     * blocked process does not receive, and the queue of users awaiting it is
     * short) */
    if (setsockopt(child_socket, SOL_SOCKET, SO_SNDTIMEO,
            &GUACD_TIMEOUT_TIMEVAL, sizeof(GUACD_TIMEOUT_TIMEVAL)))
        guacd_log(GUAC_LOG_WARNING, "Unable to set send timeout on internal "
                "socket of connection process: %s. A blocked connection "
                "process may delay users joining it.", strerror(errno));

    /* Allocate process */
    guacd_proc* proc = guac_mem_zalloc(sizeof(guacd_proc));
    if (proc == NULL) {
        close(parent_socket);
        close(child_socket);
        return NULL;
    }

    proc->fd_socket = child_socket;

    /* Associate new client */
    proc->client = guac_client_alloc();
    if (proc->client == NULL) {
        guacd_log_guac_error(GUAC_LOG_ERROR, "Unable to create client");
        close(parent_socket);
        guacd_proc_free(proc);
        return NULL;
    }

    /* Init logging */
    proc->client->log_handler = guacd_client_log;

    int config_pipe[2];
    if (guacd_proc_config_open_pipe(config_pipe)) {
        guacd_log(GUAC_LOG_ERROR, "Error opening configuration pipe: %s",
                strerror(errno));
        close(parent_socket);
        guacd_proc_free(proc);
        return NULL;
    }

    /* Compose arguments before forking, as only async-signal-safe operations are
     * permitted between fork and exec */
    char process_name[GUACD_PROC_NAME_LENGTH];
    int name_length = snprintf(process_name, sizeof(process_name), "%s%s%s",
            GUACD_PROC_TITLE_PREFIX, protocol, GUACD_PROC_TITLE_SUFFIX);

    if (name_length < 0 || (size_t) name_length >= sizeof(process_name)) {
        guacd_log(GUAC_LOG_ERROR, "Invalid protocol name \"%s\": Name exceeds "
                "maximum length.", protocol);
        close(parent_socket);
        close(config_pipe[0]);
        close(config_pipe[1]);
        guacd_proc_free(proc);
        return NULL;
    }

    /* Pad argv[0] with spaces to reserve space for connection details within the
     * process title */
    if ((size_t) name_length < sizeof(process_name) - 1) {
        memset(process_name + name_length, ' ', sizeof(process_name) - 1 - name_length);
        process_name[sizeof(process_name) - 1] = '\0';
    }

    char* const child_argv[] = {
        process_name,
        NULL
    };

    /* Fork */
    proc->pid = fork();
    if (proc->pid < 0) {
        guacd_log(GUAC_LOG_ERROR, "Cannot fork child process: %s", strerror(errno));
        close(parent_socket);
        close(config_pipe[0]);
        close(config_pipe[1]);
        guacd_proc_free(proc);
        return NULL;
    }

    /* Child */
    else if (proc->pid == 0) {

        /* Establish own process group so the parent can signal the whole group
         * by PID. Without one, the teardown timeout's kill(0, SIGKILL) would
         * kill guacd itself. */
        if (setpgid(0, 0))
            _exit(EXIT_FAILURE);

        /* Move the socket and our end of the config pipe onto their known, fixed
         * descriptors (these were reserved at startup within main()) */
        if (dup2(parent_socket, GUACD_PROC_SOCKET_FD) == -1
                || dup2(config_pipe[0], GUACD_PROC_CONFIG_FD) == -1)
            _exit(EXIT_FAILURE);

        /* Re-exec as a per-connection process from the image pinned by the file
         * descriptor during startup */
        fexecve(exe_fd, child_argv, environ);

        /* NOTE: We cannot log any failures here. We can reach here only if
         * fexecve() above fails, which means any syslog-related locks that were
         * inherited are still present and may be locked. */

        _exit(EXIT_FAILURE);

    }

    /* Parent */
    else {

        /* Establish the child's process group from this side too, as a group
         * kill fails until the child runs its own setpgid(). Whichever side
         * runs first succeeds. */
        setpgid(proc->pid, proc->pid);

        /* The child holds its own copies of the pipe and the other end of the
         * socket pair */
        close(config_pipe[0]);
        close(parent_socket);

        /* NOTE: The timed period for the connection process to reach
         * self-supervision under its own watchdog thread begins now. */

        guac_timestamp startup_deadline = guac_timestamp_current() + GUACD_TIMEOUT;

        /* Provide the child with its configuration and identity over the
         * dedicated setup pipe */

        guacd_proc_config config = {
            .log_level = guacd_log_level,
            .parent_pid = getpid(),
            .startup_deadline = startup_deadline
        };

        guac_strlcpy(config.connection_id, proc->client->connection_id,
                sizeof(config.connection_id));

        int send_failed = guacd_proc_config_write(config_pipe[1], &config);

        /* Await the child's arrival, announced by closing its end of the
         * configuration pipe. Nothing within the child can bound this, exec and
         * loader preceding its code. */
        int arrived = 0;
        if (!send_failed) {
            struct pollfd arrival = { .fd = config_pipe[1] };
            for (;;) {

                int remaining = (int) (startup_deadline - guac_timestamp_current());
                if (remaining < 0)
                    remaining = 0;

                arrived = poll(&arrival, 1, remaining);
                if (arrived != -1 || errno != EINTR)
                    break;

            }
        }

        /* The write end is no longer needed, whether or not the child arrived */
        close(config_pipe[1]);

        /* Disambiguate child arrival from child death (both result in the
         * closure of the config pipe) */
        int still_running = guacd_proc_is_running(proc);
        if (arrived != 1 || !still_running) {

            if (still_running) {

                /* Forcibly kill still running process */
                kill(-proc->pid, SIGKILL);
                kill(proc->pid, SIGKILL);

                if (send_failed)
                    guacd_log(GUAC_LOG_ERROR, "Unable to send the "
                            "configuration of a connection to the process "
                            "meant to serve it. That process has been "
                            "killed and the connection dropped.");
                else
                    guacd_log(GUAC_LOG_ERROR, "Connection process did not "
                            "arrive at its own startup and has been "
                            "killed.");

            }

            /* Process never finished starting up */
            else
                guacd_log(GUAC_LOG_ERROR, "Connection process died during "
                        "its own startup.");

            /* Prevent the caller from routing users to a process that cannot
             * serve them */
            guacd_proc_free(proc);

            return NULL;

        }

    }

    return proc;

}

int guacd_proc_is_running(guacd_proc* proc) {

    siginfo_t status;
    memset(&status, 0, sizeof(status));

    int result;
    GUAC_RETRY_EINTR(result, waitid(P_PID, proc->pid, &status,
                WEXITED | WNOHANG | WNOWAIT));

    return !result && status.si_pid == 0;

}

void guacd_proc_await_exit(guacd_proc* proc) {

    siginfo_t status;
    memset(&status, 0, sizeof(status));

    int result;
    GUAC_RETRY_EINTR(result, waitid(P_PID, proc->pid, &status,
                WEXITED | WNOWAIT));

}

/**
 * Requests termination of the provided child guacd process and everything it
 * has spawned, returning as soon as that request has been made. This function
 * must be called by the parent process.
 *
 * @param proc
 *     The child guacd process to terminate.
 */
static void guacd_proc_kill(guacd_proc* proc) {

    /* Request orderly termination of the process group */
    if (guacd_proc_is_running(proc)) {
        if (kill(-proc->pid, SIGTERM))
            guacd_log(GUAC_LOG_DEBUG, "Unable to request termination of "
                    "connection %s: %s", proc->client->connection_id,
                    strerror(errno));
    }

    /* Without a running connection process, there's nothing to pass on the
     * termination signal - we must manually force with SIGKILL */
    else
        kill(-proc->pid, SIGKILL);

}

void guacd_proc_stop(guacd_proc* proc) {

    /* A non-zero PID means that this is the parent process */
    if (proc->pid != 0) {
        guacd_proc_kill(proc);
        return;
    }

    /* Otherwise, this is the child process */

    /* Signal client to stop */
    guac_client_stop(proc->client);

}
