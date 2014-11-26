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
#include <guacamole/timestamp.h>

guac_rdp_disp* guac_rdp_disp_alloc() {

    guac_rdp_disp* disp = malloc(sizeof(guac_rdp_disp));

    /* Not yet connected */
    disp->disp = NULL;

    /* No requests have been made */
    disp->last_request = 0;
    disp->requested_width  = 0;
    disp->requested_height = 0;

    return disp;

}

void guac_rdp_disp_free(guac_rdp_disp* disp) {
    free(disp);
}

void guac_rdp_disp_load_plugin(rdpContext* context) {

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

void guac_rdp_disp_connect(guac_rdp_disp* guac_disp, DispClientContext* disp) {
    guac_disp->disp = disp;
}

void guac_rdp_disp_set_size(guac_rdp_disp* disp, rdpContext* context,
        int width, int height) {

    /* Width must be at least 200 pixels */
    if (width < 200)
        width = 200;

    /* Width may be no more than 8192 pixels */
    else if (width > 8192)
        width = 8192;

    /* Width must be even */
    else if (width % 2 == 1)
        width -= 1;

    /* Height must be at least 200 pixels */
    if (height < 200)
        height = 200;

    /* Height may be no more than 8192 pixels */
    else if (height > 8192)
        height = 8192;

    /* Store deferred size */
    disp->requested_width = width;
    disp->requested_height = height;

    /* Send display update notification if possible */
    guac_rdp_disp_update_size(disp, context);

}

void guac_rdp_disp_update_size(guac_rdp_disp* disp, rdpContext* context) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;

    /* Send display update notification if display channel is connected */
    if (disp->disp == NULL)
        return;

    int width = disp->requested_width;
    int height = disp->requested_height;

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

    guac_timestamp now = guac_timestamp_current();

    /* Limit display update frequency */
    if (disp->last_request != 0
            && now - disp->last_request <= GUAC_RDP_DISP_UPDATE_INTERVAL)
        return;

    /* Do NOT send requests unless the size will change */
    if (width == guac_rdp_get_width(context->instance)
            && height == guac_rdp_get_height(context->instance))
        return;

    guac_client_log(client, GUAC_LOG_DEBUG,
            "Resizing remote display to %ix%i",
            width, height);

    disp->last_request = now;
    disp->disp->SendMonitorLayout(disp->disp, 1, monitors);

}

