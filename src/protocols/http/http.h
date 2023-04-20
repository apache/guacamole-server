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

#ifndef GUAC_HTTP_H
#define GUAC_HTTP_H

#include "config.h"
#include "settings.h"

#include <guacamole/client.h>
#include <pthread.h>

/**
 * HTTP-specific client data.
 */
typedef struct guac_http_client {

    /**
     * All settings associated with the current or pending HTTP connection.
     */
    guac_http_settings* settings;

    /**
     * The HTTP client thread.
     */
    pthread_t client_thread;

} guac_http_client;

/**
 * HTTP client thread. This thread runs throughout the duration of the client,
 * existing as a single instance, shared by all users.
 *
 * @param data
 *     The guac_client to associate with an HTTP session, once the HTTP
 *     connection succeeds.
 *
 * @return
 *     NULL in all cases. The return value of this thread is expected to be
 *     ignored.
 */
void* guac_http_client_thread(void* data);

#endif
