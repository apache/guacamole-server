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

#ifndef GUAC_HTTP_CLIENT_H
#define GUAC_HTTP_CLIENT_H

#include <guacamole/client.h>

/*
 * The native resolution of most HTTP connections. As Windows and other systems
 * rely heavily on forced 96 DPI, we must assume 96 DPI.
 */
#define GUAC_HTTP_NATIVE_RESOLUTION 96

/**
 * The resolution of an HTTP connection that would be considered high, but is
 * tolerable in the case that the client display would be unreasonably small
 * otherwise.
 */
#define GUAC_HTTP_HIGH_RESOLUTION 120

/**
 * The smallest area, in pixels^2, that would be considered reasonable large
 * screen DPI needs to be adjusted.
 */
#define GUAC_HTTP_REASONABLE_AREA (800*600)

/**
 * Handler which frees all data associated with the guac_client.
 */
guac_client_free_handler guac_http_client_free_handler;

#endif
