
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

#include <stdlib.h>
#include <string.h>

#include <freerdp/constants.h>
#include <freerdp/types.h>
#include <freerdp/utils/stream.h>
#include <freerdp/utils/svc_plugin.h>

#include <guacamole/client.h>

#include "rdpdr_service.h"
#include "rdpdr_messages.h"


/* Define service, associate with "rdpdr" channel */

DEFINE_SVC_PLUGIN(guac_rdpdr, "rdpdr",
    CHANNEL_OPTION_INITIALIZED | CHANNEL_OPTION_ENCRYPT_RDP | CHANNEL_OPTION_COMPRESS_RDP)


/* 
 * Service Handlers
 */

void guac_rdpdr_process_connect(rdpSvcPlugin* plugin) {

    /* Get RDPDR plugin */
    guac_rdpdrPlugin* rdpdr = (guac_rdpdrPlugin*) plugin;

    /* Get client from plugin */
    guac_client* client = (guac_client*)
        plugin->channel_entry_points.pExtendedData;

    /* Init plugin */
    rdpdr->client = client;

    /* Log that printing, etc. has been loaded */
    guac_client_log_info(client, "guac_rdpdr connected.");

}

void guac_rdpdr_process_terminate(rdpSvcPlugin* plugin) {
    xfree(plugin);
}

void guac_rdpdr_process_event(rdpSvcPlugin* plugin, RDP_EVENT* event) {
    freerdp_event_free(event);
}

void guac_rdpdr_process_receive(rdpSvcPlugin* plugin,
        STREAM* input_stream) {

    guac_rdpdrPlugin* rdpdr = (guac_rdpdrPlugin*) plugin;

    int component;
    int packet_id;

    /* Read header */
    stream_read_uint16(input_stream, component);
    stream_read_uint16(input_stream, packet_id);

    /* Core component */
    if (component == RDPDR_CTYP_CORE) {

        /* Dispatch handlers based on packet ID */
        switch (packet_id) {

            case PAKID_CORE_SERVER_ANNOUNCE:
                guac_rdpdr_process_server_announce(rdpdr, input_stream);
                break;

            case PAKID_CORE_CLIENTID_CONFIRM:
                guac_rdpdr_process_clientid_confirm(rdpdr, input_stream);
                break;

            case PAKID_CORE_DEVICE_REPLY:
                guac_rdpdr_process_device_reply(rdpdr, input_stream);
                break;

            case PAKID_CORE_DEVICE_IOREQUEST:
                guac_rdpdr_process_device_iorequest(rdpdr, input_stream);
                break;

            case PAKID_CORE_SERVER_CAPABILITY:
                guac_rdpdr_process_server_capability(rdpdr, input_stream);
                break;

            case PAKID_CORE_USER_LOGGEDON:
                guac_rdpdr_process_user_loggedon(rdpdr, input_stream);
                break;

            default:
                guac_client_log_info(rdpdr->client, "Ignoring RDPDR core packet with unexpected ID: 0x%04x", packet_id);

        }

    } /* end if core */

    /* Printer component */
    else if (component == RDPDR_CTYP_PRN) {

        /* Dispatch handlers based on packet ID */
        switch (packet_id) {

            case PAKID_PRN_CACHE_DATA:
                guac_rdpdr_process_prn_cache_data(rdpdr, input_stream);
                break;

            case PAKID_PRN_USING_XPS:
                guac_rdpdr_process_prn_using_xps(rdpdr, input_stream);
                break;

            default:
                guac_client_log_info(rdpdr->client, "Ignoring RDPDR printer packet with unexpected ID: 0x%04x", packet_id);

        }

    } /* end if printer */

    else
        guac_client_log_info(rdpdr->client, "Ignoring packet for unknown RDPDR component: 0x%04x", component);

}

