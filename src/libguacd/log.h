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

#ifndef LIBGUACD_LOG_H
#define LIBGUACD_LOG_H

#include "config.h"

#include <guacamole/client.h>

/**
 * Prints an error message using the logging facilities of the given client,
 * automatically including any information present in guac_error. This function
 * accepts parameters identically to printf.
 */
void guacd_client_log_guac_error(guac_client* client,
        guac_client_log_level level, const char* message);

/**
 * Logs a reasonable explanatory message regarding handshake failure based on
 * the current value of guac_error.
 */
void guacd_client_log_handshake_failure(guac_client* client);

#endif

