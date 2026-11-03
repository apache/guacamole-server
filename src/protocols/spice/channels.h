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

#ifndef GUAC_SPICE_CHANNELS_H
#define GUAC_SPICE_CHANNELS_H

#include <guacamole/client.h>
#include <spice-client.h>

/**
 * Signal handler for the SpiceSession "channel-new" signal. Inspects the type
 * of each newly-created channel, wires up the appropriate Guacamole-specific
 * signal handlers, and opens the channel.
 *
 * @param session
 *     The SPICE session which created the channel.
 *
 * @param channel
 *     The newly-created SPICE channel.
 *
 * @param data
 *     The guac_client associated with the SPICE connection.
 */
void guac_spice_channel_new(SpiceSession* session, SpiceChannel* channel,
        gpointer data);

/**
 * Signal handler for the SpiceSession "channel-destroy" signal. Releases any
 * references held to the destroyed channel.
 *
 * @param session
 *     The SPICE session which destroyed the channel.
 *
 * @param channel
 *     The SPICE channel being destroyed.
 *
 * @param data
 *     The guac_client associated with the SPICE connection.
 */
void guac_spice_channel_destroy(SpiceSession* session, SpiceChannel* channel,
        gpointer data);

#endif
