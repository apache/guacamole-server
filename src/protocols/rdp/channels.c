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

#include "config.h"

#include <freerdp/channels/channels.h>
#include <freerdp/freerdp.h>

int guac_freerdp_channels_load_plugin(rdpChannels* channels,
        rdpSettings* settings, const char* name, void* data) {

    /* Load plugin using "ex" version of the channel plugin entry point, if it exists */
    PVIRTUALCHANNELENTRYEX entry_ex = (PVIRTUALCHANNELENTRYEX) (void*) freerdp_load_channel_addin_entry(name,
            NULL, NULL, FREERDP_ADDIN_CHANNEL_STATIC | FREERDP_ADDIN_CHANNEL_ENTRYEX);

    if (entry_ex != NULL)
        return freerdp_channels_client_load_ex(channels, settings, entry_ex, data);

    /* Lacking the "ex" entry point, attempt to load using the non-ex version */
    PVIRTUALCHANNELENTRY entry = freerdp_load_channel_addin_entry(name,
            NULL, NULL, FREERDP_ADDIN_CHANNEL_STATIC);

    if (entry != NULL)
        return freerdp_channels_client_load(channels, settings, entry, data);

    /* The plugin does not exist / cannot be loaded */
    return 1;

}

