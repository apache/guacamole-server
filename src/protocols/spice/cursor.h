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

#ifndef GUAC_SPICE_CURSOR_H
#define GUAC_SPICE_CURSOR_H

#include <guacamole/client.h>
#include <spice-client.h>

/**
 * Connects the necessary signal handlers to the given SPICE cursor channel so
 * that the remote cursor shape is mirrored into the cursor layer of the
 * guac_display of the given client.
 *
 * @param client
 *     The guac_client associated with the SPICE connection.
 *
 * @param channel
 *     The SPICE cursor channel to handle.
 */
void guac_spice_cursor_channel_connect(guac_client* client,
        SpiceChannel* channel);

#endif
