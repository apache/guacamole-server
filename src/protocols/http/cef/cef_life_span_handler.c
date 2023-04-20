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

#include "cef_life_span_handler.h"

#include <stdlib.h>

custom_life_span_handler_t *create_life_span_handler() {
    custom_life_span_handler_t *handler =
        (custom_life_span_handler_t *)calloc(1, sizeof(custom_life_span_handler_t));

    /* Set the size of the custom structure. */
    handler->base.base.size = sizeof(cef_life_span_handler_t);

    return handler;
}
