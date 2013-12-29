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


#include <freerdp/freerdp.h>
#include <freerdp/channels/channels.h>
#include <freerdp/utils/event.h>

#ifdef HAVE_FREERDP_CLIENT_CLIPRDR_H
#include <freerdp/client/cliprdr.h>
#else
#include "compat/client-cliprdr.h"
#endif

#ifdef ENABLE_WINPR
#include <winpr/wtypes.h>
#else
#include "compat/winpr-wtypes.h"
#endif

#include <guacamole/client.h>
#include <guacamole/protocol.h>

#include "client.h"
#include "rdp_cliprdr.h"

void guac_rdp_process_cliprdr_event(guac_client* client, wMessage* event) {

#ifdef LEGACY_EVENT
        switch (event->event_type) {
#else
        switch (GetMessageType(event->id)) {
#endif

            case CliprdrChannel_MonitorReady:
                guac_rdp_process_cb_monitor_ready(client, event);
                break;

            case CliprdrChannel_FormatList:
                guac_rdp_process_cb_format_list(client,
                        (RDP_CB_FORMAT_LIST_EVENT*) event);
                break;

            case CliprdrChannel_DataRequest:
                guac_rdp_process_cb_data_request(client,
                        (RDP_CB_DATA_REQUEST_EVENT*) event);
                break;

            case CliprdrChannel_DataResponse:
                guac_rdp_process_cb_data_response(client,
                        (RDP_CB_DATA_RESPONSE_EVENT*) event);
                break;

            default:
#ifdef LEGACY_EVENT
                guac_client_log_info(client,
                        "Unknown cliprdr event type: 0x%x",
                        event->event_type);
#else
                guac_client_log_info(client,
                        "Unknown cliprdr event type: 0x%x",
                        GetMessageType(event->id));
#endif

        }

}

void guac_rdp_process_cb_monitor_ready(guac_client* client, wMessage* event) {

    rdpChannels* channels = 
        ((rdp_guac_client_data*) client->data)->rdp_inst->context->channels;

    RDP_CB_FORMAT_LIST_EVENT* format_list =
        (RDP_CB_FORMAT_LIST_EVENT*) freerdp_event_new(
            CliprdrChannel_Class,
            CliprdrChannel_FormatList,
            NULL, NULL);

    /* Received notification of clipboard support. */

    /* Respond with supported format list */
    format_list->formats = (UINT32*) malloc(sizeof(UINT32));
    format_list->formats[0] = CB_FORMAT_TEXT;
    format_list->num_formats = 1;

    freerdp_channels_send_event(channels, (wMessage*) format_list);

}

void guac_rdp_process_cb_format_list(guac_client* client,
        RDP_CB_FORMAT_LIST_EVENT* event) {

    rdpChannels* channels = 
        ((rdp_guac_client_data*) client->data)->rdp_inst->context->channels;

    /* Received notification of available data */

    int i;
    for (i=0; i<event->num_formats; i++) {

        /* If plain text available, request it */
        if (event->formats[i] == CB_FORMAT_TEXT) {

            /* Create new data request */
            RDP_CB_DATA_REQUEST_EVENT* data_request =
                (RDP_CB_DATA_REQUEST_EVENT*) freerdp_event_new(
                        CliprdrChannel_Class,
                        CliprdrChannel_DataRequest,
                        NULL, NULL);

            /* We want plain text */
            data_request->format = CB_FORMAT_TEXT;

            /* Send request */
            freerdp_channels_send_event(channels, (wMessage*) data_request);
            return;

        }

    }

    /* Otherwise, no supported data available */
    guac_client_log_info(client, "Ignoring unsupported clipboard data");

}

void guac_rdp_process_cb_data_request(guac_client* client,
        RDP_CB_DATA_REQUEST_EVENT* event) {

    rdpChannels* channels = 
        ((rdp_guac_client_data*) client->data)->rdp_inst->context->channels;

    /* If text requested, send clipboard text contents */
    if (event->format == CB_FORMAT_TEXT) {

        /* Get clipboard data */
        const char* clipboard =
            ((rdp_guac_client_data*) client->data)->clipboard;

        /* Create new data response */
        RDP_CB_DATA_RESPONSE_EVENT* data_response =
            (RDP_CB_DATA_RESPONSE_EVENT*) freerdp_event_new(
                    CliprdrChannel_Class,
                    CliprdrChannel_DataResponse,
                    NULL, NULL);

        /* Set data and length */
        if (clipboard != NULL) {
            data_response->data = (UINT8*) strdup(clipboard);
            data_response->size = strlen(clipboard) + 1;
        }
        else {
            data_response->data = (UINT8*) strdup("");
            data_response->size = 1;
        }

        /* Send response */
        freerdp_channels_send_event(channels, (wMessage*) data_response);

    }

    /* Otherwise ... failure */
    else
        guac_client_log_error(client, 
                "Server requested unsupported clipboard data type");

}

void guac_rdp_process_cb_data_response(guac_client* client,
        RDP_CB_DATA_RESPONSE_EVENT* event) {

    /* Received clipboard data */
    if (event->data[event->size - 1] == '\0') {

        /* Free existing data */
        free(((rdp_guac_client_data*) client->data)->clipboard);

        /* Store clipboard data */
        ((rdp_guac_client_data*) client->data)->clipboard =
            strdup((char*) event->data);

        /* Send clipboard data */
        guac_protocol_send_clipboard(client->socket, (char*) event->data);

    }
    else
        guac_client_log_error(client,
                "Clipboard data missing null terminator");

}

