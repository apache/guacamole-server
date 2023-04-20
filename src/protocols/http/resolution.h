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

#ifndef GUAC_HTTP_RESOLUTION_H
#define GUAC_HTTP_RESOLUTION_H

#include <guacamole/user.h>

/**
 * Returns whether the given resolution is reasonable for the given user,
 * based on arbitrary criteria for reasonability.
 *
 * @param user
 *     The guac_user to test the given resolution against.
 *
 * @param resolution
 *     The resolution to test, in DPI.
 *
 * @return
 *     Non-zero if the resolution is reasonable, zero otherwise.
 */
int guac_http_resolution_reasonable(guac_user* user, int resolution);

/**
 * Returns a reasonable resolution for the display, given the size and
 * resolution of a guac_user.
 *
 * @param user
 *     The guac_user whose size and resolution shall be used to determine an
 *     appropriate  display resolution.
 *
 * @return
 *     A reasonable resolution for the display, in DPI.
 */
int guac_http_suggest_resolution(guac_user* user);

#endif
