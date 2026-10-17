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

#include "guacamole/client.h"
#include "guacamole/error.h"
#include "guacamole/flag.h"
#include "guacamole/mem.h"
#include "guacamole/thread.h"
#include "guacamole/timestamp.h"

#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

/**
 * Flag set within the state of a watchdog once teardown of the connection has
 * begun. This flag is never cleared. Once set, the connection being monitored
 * must finish cleaning up within the cleanup timeout given to
 * guac_client_watchdog_start(). If the connection does not finish cleaning up
 * within this time, it will be forcibly killed.
 */
#define GUAC_CLIENT_WATCHDOG_TEARDOWN_STARTED 1

/**
 * Flag set within the state of a watchdog once teardown of the connection has
 * completed. This flag is never cleared.
 */
#define GUAC_CLIENT_WATCHDOG_TEARDOWN_ENDED 2

/**
 * Flag set within the state of a watchdog once all users have left the
 * connection. Once set, this flag is never cleared, and no further users are
 * permitted to join.
 */
#define GUAC_CLIENT_WATCHDOG_USERS_GONE 4

/**
 * The polling interval of the watchdog's state checks, in milliseconds.
 */
#define GUAC_CLIENT_WATCHDOG_POLLING_INTERVAL 1000

struct guac_client_watchdog {

    /**
     * The state of the watchdog. This guac_flag also functions as the lock for
     * other state-related members of this structure.
     *
     * @see GUAC_CLIENT_WATCHDOG_TEARDOWN_STARTED
     * @see GUAC_CLIENT_WATCHDOG_TEARDOWN_ENDED
     * @see GUAC_CLIENT_WATCHDOG_USERS_GONE
     */
    guac_flag state;

    /**
     * The client whose connection this watchdog observes.
     *
     * @important
     *     This pointer MUST NOT be dereferenced once
     *     GUAC_CLIENT_WATCHDOG_TEARDOWN_STARTED has been set.
     *
     * @important
     *     This value must only be read or modified while holding the state
     *     lock.
     */
    guac_client* client;

    /**
     * The number of milliseconds allowed for a timed operation before the
     * connection process is considered unresponsive. Within the scope of client
     * plugins, and with the sole exception of the free_handler covered by
     * cleanup_timeout, the handlers of guac_client and guac_user are considered
     * timed operations. This value may safely be read without acquiring the
     * state lock.
     */
    unsigned const int op_timeout;

    /**
     * The maximum number of milliseconds the process may take to clean up
     * before it is forcibly killed. This value may safely be read without
     * acquiring the state lock.
     */
    unsigned const int cleanup_timeout;

    /**
     * Whether the connection being monitored has been requested to stop. This
     * value may safely be modified or checked without acquiring the state lock.
     *
     * @see guac_client_watchdog_stop_connection().
     */
    volatile sig_atomic_t stop_requested;

    /**
     * Whether users are currently present on the connection.
     *
     * @note
     *     This value is maintained as a consistent rough count. It increases
     *     as users join, decreases as users leave, is non-zero when at least
     *     one user is present, and is exactly zero once all users have left.
     *     An individual user may be counted multiple times so long as that
     *     same number is decremented from this value when that user leaves.
     *
     * @important
     *     This value must only be read or modified while holding the state
     *     lock.
     */
    unsigned int users;

    /**
     * The number of timed operations currently in progress across all users of
     * the connection.
     *
     * @important
     *     This value must only be read or modified while holding the state
     *     lock.
     */
    unsigned int active_operations;

    /**
     * The number of times any timed operation has started or ended, across all
     * users of the connection.
     *
     * @important
     *     This value must only be read or modified while holding the state
     *     lock.
     */
    unsigned long total_operation_events;

};

int guac_client_watchdog_notify_user_joined(guac_client* client) {

    guac_client_watchdog* watchdog = client->__watchdog;
    if (watchdog == NULL)
        return 0;

    guac_flag_lock(&watchdog->state);

    /* No further users are accepted after the last user has left */
    if (watchdog->state.value & GUAC_CLIENT_WATCHDOG_USERS_GONE) {
        guac_flag_unlock(&watchdog->state);
        return 1;
    }

    watchdog->users++;
    guac_flag_unlock(&watchdog->state);

    return 0;

}

void guac_client_watchdog_notify_user_left(guac_client* client) {

    guac_client_watchdog* watchdog = client->__watchdog;
    if (watchdog == NULL)
        return;

    guac_flag_lock(&watchdog->state);

    if (watchdog->users && --watchdog->users == 0)
        guac_flag_set(&watchdog->state, GUAC_CLIENT_WATCHDOG_USERS_GONE);

    guac_flag_unlock(&watchdog->state);

}

void guac_client_watchdog_notify_operation_start(guac_client* client) {

    guac_client_watchdog* watchdog = client->__watchdog;
    if (watchdog == NULL)
        return;

    guac_flag_lock(&watchdog->state);
    watchdog->active_operations++;
    watchdog->total_operation_events++;
    guac_flag_unlock(&watchdog->state);

}

void guac_client_watchdog_notify_operation_end(guac_client* client) {

    guac_client_watchdog* watchdog = client->__watchdog;
    if (watchdog == NULL)
        return;

    guac_flag_lock(&watchdog->state);

    if (watchdog->active_operations)
        watchdog->active_operations--;

    watchdog->total_operation_events++;
    guac_flag_unlock(&watchdog->state);

}

void guac_client_watchdog_stop_connection(guac_client_watchdog* watchdog) {
    watchdog->stop_requested = 1;
}

void guac_client_watchdog_notify_teardown_start(guac_client_watchdog* watchdog) {

    guac_flag_set_and_lock(&watchdog->state, GUAC_CLIENT_WATCHDOG_TEARDOWN_STARTED);

    if (watchdog->users == 0)
        guac_flag_set(&watchdog->state, GUAC_CLIENT_WATCHDOG_USERS_GONE);

    guac_flag_unlock(&watchdog->state);

}

void guac_client_watchdog_await_teardown(guac_client_watchdog* watchdog) {
    guac_flag_wait_and_lock(&watchdog->state, GUAC_CLIENT_WATCHDOG_TEARDOWN_STARTED);
    guac_flag_unlock(&watchdog->state);
}

void guac_client_watchdog_notify_teardown_end(guac_client_watchdog* watchdog) {

    guac_flag_lock(&watchdog->state);

    if (watchdog->state.value & GUAC_CLIENT_WATCHDOG_TEARDOWN_STARTED)
        guac_flag_set(&watchdog->state, GUAC_CLIENT_WATCHDOG_TEARDOWN_ENDED);

    guac_flag_unlock(&watchdog->state);

}

void guac_client_watchdog_await_all_users(guac_client_watchdog* watchdog) {
    guac_flag_wait_and_lock(&watchdog->state, GUAC_CLIENT_WATCHDOG_USERS_GONE);
    guac_flag_unlock(&watchdog->state);
}

/**
 * Thread which guarantees the process serving a client does not continue
 * past that client's connection.
 *
 * @param data
 *     The guac_client_watchdog describing the watchdog.
 *
 * @return
 *     Always NULL.
 */
static void* guac_client_watchdog_thread(void* data) {

    guac_client_watchdog* watchdog = (guac_client_watchdog*) data;

    /* Do not allow the signal handlers of any client plugin (or its libraries)
     * to run on this thread. A signal handler that preempts this thread and
     * blocks could prevent the cleanup deadline from being enforced. */
    sigset_t all_signals;
    sigfillset(&all_signals);
    pthread_sigmask(SIG_BLOCK, &all_signals, NULL);

    unsigned long last_operation_events = 0;
    guac_timestamp last_progress = guac_timestamp_current();

    /* Watch connection state until teardown begins */
    for (;;) {

        if (!guac_flag_timedwait_and_lock(&watchdog->state,
                    GUAC_CLIENT_WATCHDOG_TEARDOWN_STARTED | GUAC_CLIENT_WATCHDOG_USERS_GONE,
                    GUAC_CLIENT_WATCHDOG_POLLING_INTERVAL)) {
            guac_flag_lock(&watchdog->state);
        }

        /* Teardown may have begun while the lock was being acquired. If it has,
         * it's no longer safe to dereference the client, and we must stop
         * monitoring here */
        if (watchdog->state.value & GUAC_CLIENT_WATCHDOG_TEARDOWN_STARTED) {
            guac_flag_unlock(&watchdog->state);
            break;
        }

        guac_timestamp current = guac_timestamp_current();

        /* Track general operation progress, stopping the client if at least one
         * operation must have exceeded the timeout */
        if (watchdog->active_operations == 0
                || watchdog->total_operation_events != last_operation_events) {
            last_operation_events = watchdog->total_operation_events;
            last_progress = current;
        }
        else if (current - last_progress >= watchdog->op_timeout) {
            guac_client_watchdog_stop_connection(watchdog);
        }

        /* Switch from watching connection behavior to watching termination
         * progress if any termination condition is met (stop forced/requested
         * or all users have left) */
        if (watchdog->stop_requested
                || watchdog->client->state != GUAC_CLIENT_RUNNING
                || (watchdog->state.value & GUAC_CLIENT_WATCHDOG_USERS_GONE)) {
            guac_client_watchdog_notify_teardown_start(watchdog);
            guac_flag_unlock(&watchdog->state);
            break;
        }

        guac_flag_unlock(&watchdog->state);

    }

    /* Teardown must complete within the cleanup timeout */
    if (guac_flag_timedwait_and_lock(&watchdog->state,
                GUAC_CLIENT_WATCHDOG_TEARDOWN_ENDED,
                watchdog->cleanup_timeout)) {
        guac_flag_unlock(&watchdog->state);
        return NULL;
    }

    /* If we reach here, teardown did not complete in time. NOTE: We do not log
     * here for the reason noted above - logging would mean invoking
     * guac_client_log() and there's no guarantee that the client has not yet
     * been freed. */
    kill(0, SIGKILL);

    /* We should never reach here so long as SIGKILL delivery is possible */
    _exit(EXIT_FAILURE);

}

guac_client_watchdog* guac_client_watchdog_start(guac_client* client,
        unsigned int op_timeout, unsigned int cleanup_timeout) {

    /* NOTE: Once started successfully, the watchdog allocated here is never
     * freed. Its struct continues to be available and its thread continues to
     * run until the process exits. */

    guac_client_watchdog* watchdog = guac_mem_zalloc(sizeof(guac_client_watchdog));
    if (watchdog == NULL) {
        guac_error = GUAC_STATUS_NO_MEMORY;
        guac_error_message = "Insufficient memory to allocate client watchdog";
        return NULL;
    }

    guac_flag_init(&watchdog->state);
    watchdog->client = client;
    *((unsigned int*) &watchdog->op_timeout) = op_timeout;
    *((unsigned int*) &watchdog->cleanup_timeout) = cleanup_timeout;

    guac_thread watchdog_thread;
    if (guac_thread_create(&watchdog_thread, guac_client_watchdog_thread, watchdog, "conn-watchdog")) {
        guac_flag_destroy(&watchdog->state);
        guac_mem_free(watchdog);
        return NULL;
    }

    guac_thread_detach(&watchdog_thread);

    client->__watchdog = watchdog;
    return watchdog;

}
