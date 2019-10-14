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
#include "client.h"
#include "rail.h"
#include "rdp.h"
#include "rdp_settings.h"

#include <freerdp/client/rail.h>
#include <freerdp/freerdp.h>
#include <guacamole/client.h>
#include <winpr/wtypes.h>

#include <stddef.h>

/**
 * Callback which is invoked when a Server System Parameters Update PDU is
 * received from the RDP server. The Server System Parameters Update PDU, also
 * referred to as a "sysparam order", is used by the server to update system
 * parameters for RemoteApp.
 *
 * @param rail
 *     The RailClientContext structure used by FreeRDP to handle the RAIL
 *     channel for the current RDP session.
 *
 * @param sysparam
 *     The RAIL_SYSPARAM_ORDER structure representing the Server System
 *     Parameters Update PDU that was received.
 *
 * @return
 *     CHANNEL_RC_OK (zero) if the PDU was handled successfully, an error code
 *     (non-zero) otherwise.
 */
static UINT guac_rdp_rail_sysparam(RailClientContext* rail,
        const RAIL_SYSPARAM_ORDER* sysparam) {

    guac_client* client = (guac_client*) rail->custom;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    RAIL_SYSPARAM_ORDER response = {
        .workArea = {
            .left   = 0,
            .top    = 0,
            .right  = rdp_client->settings->width,
            .bottom = rdp_client->settings->height
        },
        .dragFullWindows = FALSE
    };

    return rail->ClientSystemParam(rail, &response);

}

/**
 * Callback which associates handlers specific to Guacamole with the
 * RailClientContext instance allocated by FreeRDP to deal with received
 * RAIL (RemoteApp) messages.
 *
 * This function is called whenever a channel connects via the PubSub event
 * system within FreeRDP, but only has any effect if the connected channel is
 * the RAIL channel. This specific callback is registered with the PubSub
 * system of the relevant rdpContext when guac_rdp_rail_load_plugin() is
 * called.
 *
 * @param context
 *     The rdpContext associated with the active RDP session.
 *
 * @param e
 *     Event-specific arguments, mainly the name of the channel, and a
 *     reference to the associated plugin loaded for that channel by FreeRDP.
 */
static void guac_rdp_rail_channel_connected(rdpContext* context,
        ChannelConnectedEventArgs* e) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;

    /* Ignore connection event if it's not for the RAIL channel */
    if (strcmp(e->name, RAIL_SVC_CHANNEL_NAME) != 0)
        return;

    /* The structure pointed to by pInterface is guaranteed to be a
     * RailClientContext if the channel is RAIL */
    RailClientContext* rail = (RailClientContext*) e->pInterface;

    /* Init FreeRDP RAIL context, ensuring the guac_client can be accessed from
     * within any RAIL-specific callbacks */
    rail->custom = client;
    rail->ServerSystemParam = guac_rdp_rail_sysparam;

    guac_client_log(client, GUAC_LOG_DEBUG, "RAIL (RemoteApp) channel "
            "connected.");

}

void guac_rdp_rail_load_plugin(rdpContext* context) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;

    /* Attempt to load FreeRDP support for the RAIL channel */
    if (guac_freerdp_channels_load_plugin(context->channels, context->settings, "rail", context->settings)) {
        guac_client_log(client, GUAC_LOG_WARNING,
                "Support for the RAIL channel (RemoteApp) could not be "
                "loaded. This support normally takes the form of a plugin "
                "which is built into FreeRDP. Lacking this support, "
                "RemoteApp will not work.");
        return;
    }

    /* Complete RDP side of initialization when channel is connected */
    PubSub_SubscribeChannelConnected(context->pubSub,
            (pChannelConnectedEventHandler) guac_rdp_rail_channel_connected);

    guac_client_log(client, GUAC_LOG_DEBUG, "Support for RAIL (RemoteApp) "
            "registered. Awaiting channel connection.");

}

