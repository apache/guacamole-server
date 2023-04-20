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

#include "cef_client.h"

#include <stdlib.h>

/**
 * Function called to get the life span handler associated with the provided
 * client instance.
 *
 * @param self Pointer to the client instance.
 * 
 * @return Pointer to the life_span_handler_t field of the client instance.
 */
cef_life_span_handler_t* get_life_span_handler(cef_client_t* self) {
    custom_client_t* custom_client = (custom_client_t*)self;
    return &custom_client->life_span_handler->base;
}

/**
 * Function called to get the render handler associated with the provided
 * client instance.
 *
 * @param self Pointer to the client instance.
 * 
 * @return Pointer to the render_handler_t field of the client instance.
 */
cef_render_handler_t* get_render_handler(cef_client_t* self) {
    custom_client_t* custom_client = (custom_client_t*)self;
    return &custom_client->render_handler->base;
}

cef_client_t *create_client(custom_render_handler_t *render_handler) {
    custom_client_t *custom_client = (custom_client_t *)calloc(1, sizeof(custom_client_t));
    cef_client_t *client = &custom_client->base;

    /* Initialize base structure */
    client->base.size = sizeof(cef_client_t);
    /* Assign our custom render handler to the client's render_handler */
    custom_client->render_handler = render_handler;
    client->get_render_handler = get_render_handler;

    custom_life_span_handler_t *life_span_handler = create_life_span_handler();
    custom_client->life_span_handler = life_span_handler;
    client->get_life_span_handler = get_life_span_handler;

    return client;
}
