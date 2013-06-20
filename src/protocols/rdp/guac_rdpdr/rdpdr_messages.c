
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

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <freerdp/constants.h>
#include <freerdp/types.h>
#include <freerdp/utils/memory.h>
#include <freerdp/utils/stream.h>
#include <freerdp/utils/svc_plugin.h>

#include <guacamole/client.h>

#include "rdpdr_service.h"
#include "rdpdr_messages.h"
#include "client.h"


static void guac_rdpdr_send_client_announce_reply(guac_rdpdrPlugin* rdpdr,
        unsigned int major, unsigned int minor, unsigned int client_id) {

    STREAM* output_stream = stream_new(12);

    /* Write header */
    stream_write_uint16(output_stream, RDPDR_CTYP_CORE);
    stream_write_uint16(output_stream, PAKID_CORE_CLIENTID_CONFIRM);

    /* Write content */
    stream_write_uint16(output_stream, major);
    stream_write_uint16(output_stream, minor);
    stream_write_uint32(output_stream, client_id);

    svc_plugin_send((rdpSvcPlugin*) rdpdr, output_stream);

}

void guac_rdpdr_process_server_announce(guac_rdpdrPlugin* rdpdr,
        STREAM* input_stream) {

    unsigned int major, minor, client_id;

    stream_read_uint16(input_stream, major);
    stream_read_uint16(input_stream, minor);
    stream_read_uint32(input_stream, client_id);

    guac_client_log_info(rdpdr->client, "Connected to RDPDR %u.%u as client 0x%04x", major, minor, client_id);

    /* Respond to announce */
    guac_rdpdr_send_client_announce_reply(rdpdr, major, minor, client_id);

}

void guac_rdpdr_process_clientid_confirm(guac_rdpdrPlugin* rdpdr, STREAM* input_stream) {

    /* STUB */
    guac_client_log_info(rdpdr->client, "STUB: clientid_confirm");

}

void guac_rdpdr_process_device_reply(guac_rdpdrPlugin* rdpdr, STREAM* input_stream) {

    /* STUB */
    guac_client_log_info(rdpdr->client, "STUB: device_reply");

}

void guac_rdpdr_process_device_iorequest(guac_rdpdrPlugin* rdpdr, STREAM* input_stream) {

    /* STUB */
    guac_client_log_info(rdpdr->client, "STUB: device_iorequest");

}

void guac_rdpdr_process_device_iocompletion(guac_rdpdrPlugin* rdpdr, STREAM* input_stream) {

    /* STUB */
    guac_client_log_info(rdpdr->client, "STUB: device_iocompletion");

}

void guac_rdpdr_process_server_capability(guac_rdpdrPlugin* rdpdr, STREAM* input_stream) {

    /* STUB */
    guac_client_log_info(rdpdr->client, "STUB: server_capability");

}

void guac_rdpdr_process_user_loggedon(guac_rdpdrPlugin* rdpdr, STREAM* input_stream) {

    /* STUB */
    guac_client_log_info(rdpdr->client, "STUB: user_loggedon");

}

void guac_rdpdr_process_prn_cache_data(guac_rdpdrPlugin* rdpdr, STREAM* input_stream) {

    /* STUB */
    guac_client_log_info(rdpdr->client, "STUB: prn_cache_data");

}

void guac_rdpdr_process_prn_using_xps(guac_rdpdrPlugin* rdpdr, STREAM* input_stream) {

    /* STUB */
    guac_client_log_info(rdpdr->client, "STUB: prn_using_xps");

}

