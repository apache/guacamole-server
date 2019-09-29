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

#include "client.h"
#include "rdp.h"
#include "rdp_rail.h"
#include "rdp_settings.h"

#include <freerdp/channels/channels.h>
#include <freerdp/freerdp.h>
#include <freerdp/rail.h>
#include <guacamole/client.h>
#include <winpr/wtypes.h>

#include <stddef.h>

void guac_rdp_process_rail_event(guac_client* client, wMessage* event) {

        switch (GetMessageType(event->id)) {

            /* Get system parameters */
            case RailChannel_GetSystemParam:
                guac_rdp_process_rail_get_sysparam(client, event);
                break;

            /* Currently ignored events */
            case RailChannel_ServerSystemParam:
            case RailChannel_ServerExecuteResult:
            case RailChannel_ServerMinMaxInfo:
            case RailChannel_ServerLocalMoveSize:
            case RailChannel_ServerGetAppIdResponse:
            case RailChannel_ServerLanguageBarInfo:
                break;

            default:
                guac_client_log(client, GUAC_LOG_INFO,
                        "Unknown rail event type: 0x%x",
                        GetMessageType(event->id));

        }

}

void guac_rdp_process_rail_get_sysparam(guac_client* client, wMessage* event) {
#if 0
    wMessage* response;
    RAIL_SYSPARAM_ORDER* sysparam;

    /* Get channels */
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    rdpChannels* channels = rdp_client->rdp_inst->context->channels;

    /* Get sysparam structure */
    sysparam = (RAIL_SYSPARAM_ORDER*) event->wParam;

    response = freerdp_event_new(RailChannel_Class,
                                 RailChannel_ClientSystemParam,
                                 NULL,
                                 sysparam);

    /* Work area */
    sysparam->workArea.left   = 0;
    sysparam->workArea.top    = 0;
    sysparam->workArea.right  = rdp_client->settings->width;
    sysparam->workArea.bottom = rdp_client->settings->height;

    /* Taskbar */
    sysparam->taskbarPos.left   = 0;
    sysparam->taskbarPos.top    = 0;
    sysparam->taskbarPos.right  = 0;
    sysparam->taskbarPos.bottom = 0;

    sysparam->dragFullWindows = FALSE;

    /* Send response */
    freerdp_channels_send_event(channels, response);
#endif
}

