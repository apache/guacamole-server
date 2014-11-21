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

#include <freerdp/freerdp.h>
#include <freerdp/client/disp.h>
#include <guacamole/client.h>

/**
 * Called whenever a channel connects. If that channel happens to be the
 * display update channel, a reference to that channel will be stored within
 * the guac_client data.
 */
static void guac_rdp_disp_channel_connected(rdpContext* context,
        ChannelConnectedEventArgs* e) {

    /* Store reference to the display update plugin once it's connected */
    if (strcmp(e->name, DISP_DVC_CHANNEL_NAME) == 0) {

        DispClientContext* disp = (DispClientContext*) e->pInterface;

        guac_client* client = ((rdp_freerdp_context*) context)->client;
        rdp_guac_client_data* guac_client_data =
            (rdp_guac_client_data*) client->data;

        guac_client_data->disp = disp;

        guac_client_log(client, GUAC_LOG_DEBUG,
                "Display update channel connected.");

    }

}

void guac_rdp_disp_load_plugin(rdpContext* context) {

    /* Subscribe to and handle channel connected events */
    PubSub_SubscribeChannelConnected(context->pubSub,
            (pChannelConnectedEventHandler) guac_rdp_disp_channel_connected);

#ifdef HAVE_RDPSETTINGS_SUPPORTDISPLAYCONTROL
    context->settings->SupportDisplayControl = TRUE;
#endif

    /* Add "disp" channel */
    ADDIN_ARGV* args = malloc(sizeof(ADDIN_ARGV));
    args->argc = 1;
    args->argv = malloc(sizeof(char**) * 1);
    args->argv[0] = strdup("disp");
    freerdp_dynamic_channel_collection_add(context->settings, args);

}

void guac_rdp_disp_send_size(rdpContext* context, int width, int height) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;

    rdp_guac_client_data* guac_client_data =
        (rdp_guac_client_data*) client->data;

    /* Send display update notification if display channel is connected */
    if (guac_client_data->disp != NULL) {

        guac_client_log(client, GUAC_LOG_DEBUG,
                "Resizing remote display to %ix%i",
                width, height);

        DISPLAY_CONTROL_MONITOR_LAYOUT monitors[1] = {{
            .Flags  = 0x1, /* DISPLAYCONTROL_MONITOR_PRIMARY */
            .Left = 0,
            .Top = 0,
            .Width  = width,
            .Height = height,
            .PhysicalWidth = 0,
            .PhysicalHeight = 0,
            .Orientation = 0,
            .DesktopScaleFactor = 0,
            .DeviceScaleFactor = 0
        }};

        guac_client_data->disp->SendMonitorLayout(guac_client_data->disp, 1,
                monitors);

    }

}

