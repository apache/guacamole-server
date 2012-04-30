
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is libguac-client-rdp.
 *
 * The Initial Developer of the Original Code is
 * Michael Jumper.
 * Portions created by the Initial Developer are Copyright (C) 2011
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */


#include <freerdp/freerdp.h>
#include <freerdp/channels/channels.h>
#include <freerdp/utils/event.h>
#include <freerdp/plugins/cliprdr.h>

#include <guacamole/client.h>

#include "client.h"
#include "rdp_cliprdr.h"

void guac_rdp_process_cliprdr_event(guac_client* client, RDP_EVENT* event) {

        switch (event->event_type) {

            case RDP_EVENT_TYPE_CB_MONITOR_READY:
                guac_rdp_process_cb_monitor_ready(client, event);
                break;

            case RDP_EVENT_TYPE_CB_FORMAT_LIST:
                guac_rdp_process_cb_format_list(client,
                        (RDP_CB_FORMAT_LIST_EVENT*) event);
                break;

            case RDP_EVENT_TYPE_CB_DATA_REQUEST:
                guac_rdp_process_cb_data_request(client,
                        (RDP_CB_DATA_REQUEST_EVENT*) event);
                break;

            case RDP_EVENT_TYPE_CB_DATA_RESPONSE:
                guac_rdp_process_cb_data_response(client,
                        (RDP_CB_DATA_RESPONSE_EVENT*) event);
                break;

            default:
                guac_client_log_info(client,
                        "Unknown cliprdr event type: 0x%x",
                        event->event_type);
        }

}

void guac_rdp_process_cb_monitor_ready(guac_client* client, RDP_EVENT* event) {

    rdpChannels* channels = 
        ((rdp_guac_client_data*) client->data)->rdp_inst->context->channels;

    RDP_CB_FORMAT_LIST_EVENT* format_list =
        (RDP_CB_FORMAT_LIST_EVENT*) freerdp_event_new(
            RDP_EVENT_CLASS_CLIPRDR,
            RDP_EVENT_TYPE_CB_FORMAT_LIST,
            NULL, NULL);

    /* Received notification of clipboard support. */

    /* Respond with supported format list */
    format_list->formats = (uint32*) malloc(sizeof(uint32));
    format_list->formats[0] = CB_FORMAT_TEXT;
    format_list->num_formats = 1;

    freerdp_channels_send_event(channels, (RDP_EVENT*) format_list);

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
                        RDP_EVENT_CLASS_CLIPRDR,
                        RDP_EVENT_TYPE_CB_DATA_REQUEST,
                        NULL, NULL);

            /* We want plain text */
            data_request->format = CB_FORMAT_TEXT;

            /* Send request */
            freerdp_channels_send_event(channels, (RDP_EVENT*) data_request);
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
                    RDP_EVENT_CLASS_CLIPRDR,
                    RDP_EVENT_TYPE_CB_DATA_RESPONSE,
                    NULL, NULL);

        /* Set data and length */
        data_response->data = (uint8*) strdup(clipboard);
        data_response->size = strlen(clipboard) + 1;

        /* Send response */
        freerdp_channels_send_event(channels, (RDP_EVENT*) data_response);

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

