/*
 * Copyright (C) 2013 Glyptodon LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "config.h"

#include "client.h"
#include "rdp.h"
#include "rdp_rail.h"
#include "rdp_settings.h"

#include <freerdp/channels/channels.h>
#include <freerdp/freerdp.h>
#include <freerdp/utils/event.h>
#include <guacamole/client.h>

#ifdef ENABLE_WINPR
#include <winpr/wtypes.h>
#else
#include "compat/winpr-wtypes.h"
#endif

#ifdef LEGACY_FREERDP
#include "compat/rail.h"
#else
#include <freerdp/rail.h>
#endif

#include <stddef.h>

void guac_rdp_process_rail_event(guac_client* client, wMessage* event) {

#ifdef LEGACY_EVENT
        switch (event->event_type) {
#else
        switch (GetMessageType(event->id)) {
#endif

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
#ifdef LEGACY_EVENT
                guac_client_log(client, GUAC_LOG_INFO,
                        "Unknown rail event type: 0x%x",
                        event->event_type);
#else
                guac_client_log(client, GUAC_LOG_INFO,
                        "Unknown rail event type: 0x%x",
                        GetMessageType(event->id));
#endif

        }

}

void guac_rdp_process_rail_get_sysparam(guac_client* client, wMessage* event) {

    wMessage* response;
    RAIL_SYSPARAM_ORDER* sysparam;

    /* Get channels */
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    rdpChannels* channels = rdp_client->rdp_inst->context->channels;

    /* Get sysparam structure */
#ifdef LEGACY_EVENT
    sysparam = (RAIL_SYSPARAM_ORDER*) event->user_data;
#else
    sysparam = (RAIL_SYSPARAM_ORDER*) event->wParam;
#endif

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

}

