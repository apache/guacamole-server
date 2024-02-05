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

#ifndef _GUAC_CLIENT_INTERNAL_H
#define _GUAC_CLIENT_INTERNAL_H

/**
 * Internal-only members of the guac_client struct.
 *
 * @file client-internal.h
 */

#include "config.h"

#include "guacamole/pool-types.h"
#include "guacamole/rwlock.h"
#include "guacamole/stream-types.h"
#include "guacamole/user-types.h"

#include <pthread.h>
#include <stdarg.h>
#include <time.h>

#ifdef WINDOWS_BUILD
#include <windef.h>
#endif

struct guac_client_internal {

    /**
     * Pool of buffer indices. Buffers are simply layers with negative indices.
     * Note that because guac_pool always gives non-negative indices starting
     * at 0, the output of this guac_pool will be adjusted.
     */
    guac_pool* __buffer_pool;

    /**
     * Pool of layer indices. Note that because guac_pool always gives
     * non-negative indices starting at 0, the output of this guac_pool will
     * be adjusted.
     */
    guac_pool* __layer_pool;

    /**
     * Pool of stream indices.
     */
    guac_pool* __stream_pool;

    /**
     * All available client-level output streams (data going to all connected
     * users).
     */
    guac_stream* __output_streams;

    /**
     * Lock which is acquired when the users list is being manipulated, or when
     * the users list is being iterated.
     */
    guac_rwlock __users_lock;

    /**
     * The first user within the list of all connected users, or NULL if no
     * users are currently connected.
     */
    guac_user* __users;

    /**
     * Lock which is acquired when the pending users list is being manipulated,
     * or when the pending users list is being iterated.
     */
    guac_rwlock __pending_users_lock;

    /**
     * A timer that will periodically synchronize the list of pending users,
     * emptying the list once synchronization is complete. Only for internal
     * use within the client. This will be NULL until the first user joins
     * the connection, as it is lazily instantiated at that time.
     */
#ifdef WINDOWS_BUILD
    HANDLE __pending_users_timer;
#else
    timer_t __pending_users_timer;
#endif

    /**
     * A flag storing the current state of the pending users timer.
     */
    int __pending_users_timer_state;

    /**
     * A mutex that must be acquired before modifying or checking the value of
     * the timer state.
     */
    pthread_mutex_t __pending_users_timer_mutex;

    /**
     * The first user within the list of connected users who have not yet had
     * their connection states synchronized after joining.
     */
    guac_user* __pending_users;

    /**
     * The user that first created this connection. This user will also have
     * their "owner" flag set to a non-zero value. If the owner has left the
     * connection, this will be NULL.
     */
    guac_user* __owner;

    /**
     * Handle to the dlopen()'d plugin, which should be given to dlclose() when
     * this client is freed. This is only assigned if guac_client_load_plugin()
     * is used.
     */
    void* __plugin_handle;

};

#endif

