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
#include "channels.h"
#include "rdp.h"

#include <freerdp/freerdp.h>
#include <guacamole/client.h>

void guac_rdpsnd_load_plugin(rdpContext* context) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;

    /* Load RDPSND plugin */
    if (guac_freerdp_channels_load_plugin(context->channels, context->settings, "guacsnd", client)) {
        guac_client_log(client, GUAC_LOG_WARNING, "Support for the RDPSND "
                "channel (audio output) could not be loaded. Sound will not "
                "work. Drive redirection and printing MAY not work.");
        return;
    }

    guac_client_log(client, GUAC_LOG_DEBUG, "Support for RDPSND (audio "
        "output) registered. Awaiting channel connection.");

}

