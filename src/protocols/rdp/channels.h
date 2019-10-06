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

#ifndef GUAC_RDP_CHANNELS_H
#define GUAC_RDP_CHANNELS_H

#include "config.h"

#include <freerdp/channels/channels.h>
#include <freerdp/freerdp.h>

/**
 * Loads the FreeRDP plugin having the given name. This function is a drop-in
 * replacement for freerdp_channels_load_plugin() which additionally loads
 * plugins implementing the PVIRTUALCHANNELENTRYEX version of the channel
 * plugin entry point. The freerdp_channels_load_plugin() function which is
 * part of FreeRDP can load only plugins which implement the
 * PVIRTUALCHANNELENTRY version of the entry point.
 *
 * @param channels
 *     The rdpChannels structure with which the plugin should be registered
 *     once loaded. This structure should be retrieved directly from the
 *     relevant FreeRDP instance.
 *
 * @param settings
 *     The rdpSettings structure associated with the FreeRDP instance, already
 *     populated with any settings applicable to the plugin being loaded.
 *
 * @param name
 *     The name of the plugin to load. If the plugin is not statically built
 *     into FreeRDP, this name will determine the filename of the library to be
 *     loaded dynamically. For a plugin named "NAME", the library called
 *     "libNAME-client" will be loaded from the "freerdp2" subdirectory of the
 *     main directory containing the FreeRDP libraries.
 *
 * @param data
 *     Arbitrary data to be passed to the plugin entry point. For most plugins
 *     which are built into FreeRDP, this will be another reference to the
 *     rdpSettings struct. The source of the relevant plugin must be consulted
 *     to determine the proper value to pass here.
 *
 * @return
 *     Zero if the plugin was loaded successfully, non-zero if the plugin could
 *     not be loaded.
 */
int guac_freerdp_channels_load_plugin(rdpChannels* channels,
        rdpSettings* settings, const char* name, void* data);

#endif

