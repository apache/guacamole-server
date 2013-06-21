
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

static void guac_rdpdr_send_client_name_request(guac_rdpdrPlugin* rdpdr, const char* name) {

    int name_bytes = strlen(name) + 1;
    STREAM* output_stream = stream_new(16 + name_bytes);

    /* Write header */
    stream_write_uint16(output_stream, RDPDR_CTYP_CORE);
    stream_write_uint16(output_stream, PAKID_CORE_CLIENT_NAME);

    /* Write content */
    stream_write_uint32(output_stream, 0); /* ASCII */
    stream_write_uint32(output_stream, 0); /* 0 required by RDPDR spec */
    stream_write_uint32(output_stream, name_bytes);
    stream_write(output_stream, name, name_bytes);

    svc_plugin_send((rdpSvcPlugin*) rdpdr, output_stream);

}

static void guac_rdpdr_send_client_capability(guac_rdpdrPlugin* rdpdr) {

    STREAM* output_stream = stream_new(256);
    guac_client_log_info(rdpdr->client, "Sending capabilities...");

    /* Write header */
    stream_write_uint16(output_stream, RDPDR_CTYP_CORE);
    stream_write_uint16(output_stream, PAKID_CORE_CLIENT_CAPABILITY);

    /* Capability count + padding */
    stream_write_uint16(output_stream, 2);
    stream_write_uint16(output_stream, 0); /* Padding */

    /* General capability header */
    stream_write_uint16(output_stream, CAP_GENERAL_TYPE);
    stream_write_uint16(output_stream, 44);
    stream_write_uint32(output_stream, GENERAL_CAPABILITY_VERSION_02);

    /* General capability data */
    stream_write_uint32(output_stream, GUAC_OS_TYPE);          /* osType - required to be ignored */
    stream_write_uint32(output_stream, 0);                     /* osVersion */
    stream_write_uint16(output_stream, RDP_CLIENT_MAJOR_ALL);  /* protocolMajor */
    stream_write_uint16(output_stream, RDP_CLIENT_MINOR_5_2);  /* protocolMinor */
    stream_write_uint32(output_stream, 0xFFFF);                /* ioCode1 */
    stream_write_uint32(output_stream, 0);                     /* ioCode2 */
    stream_write_uint32(output_stream,
                                      RDPDR_DEVICE_REMOVE_PDUS
                                    | RDPDR_CLIENT_DISPLAY_NAME
                                    | RDPDR_USER_LOGGEDON_PDU); /* extendedPDU */
    stream_write_uint32(output_stream, 0);                      /* extraFlags1 */
    stream_write_uint32(output_stream, 0);                      /* extraFlags2 */
    stream_write_uint32(output_stream, 0);                      /* SpecialTypeDeviceCap */

    /* Printer support header */
    stream_write_uint16(output_stream, CAP_PRINTER_TYPE);
    stream_write_uint16(output_stream, 8);
    stream_write_uint32(output_stream, PRINT_CAPABILITY_VERSION_01);

    svc_plugin_send((rdpSvcPlugin*) rdpdr, output_stream);
    guac_client_log_info(rdpdr->client, "Capabilities sent.");

}

static void guac_rdpdr_send_client_device_list_announce_request(guac_rdpdrPlugin* rdpdr) {

    STREAM* output_stream = stream_new(256);

    /* Write header */
    stream_write_uint16(output_stream, RDPDR_CTYP_CORE);
    stream_write_uint16(output_stream, PAKID_CORE_DEVICELIST_ANNOUNCE);

    /* Only one device for now */
    stream_write_uint32(output_stream, 1);

    /* Printer header */
    guac_client_log_info(rdpdr->client, "Sending printer");
    stream_write_uint32(output_stream, RDPDR_DTYP_PRINT);
    stream_write_uint32(output_stream, GUAC_PRINTER_DEVICE_ID);
    stream_write(output_stream, "PRN1\0\0\0\0", 8); /* DOS name */

    /* Printer data */
    stream_write_uint32(output_stream, 24 + GUAC_PRINTER_DRIVER_LENGTH + GUAC_PRINTER_NAME_LENGTH);
    stream_write_uint32(output_stream,
              RDPDR_PRINTER_ANNOUNCE_FLAG_ASCII
            | RDPDR_PRINTER_ANNOUNCE_FLAG_DEFAULTPRINTER
            | RDPDR_PRINTER_ANNOUNCE_FLAG_NETWORKPRINTER);
    stream_write_uint32(output_stream, 0); /* reserved - must be 0 */
    stream_write_uint32(output_stream, 0); /* PnPName length (PnPName is ultimately ignored) */
    stream_write_uint32(output_stream, GUAC_PRINTER_DRIVER_LENGTH); /* DriverName length */
    stream_write_uint32(output_stream, GUAC_PRINTER_NAME_LENGTH);   /* PrinterName length */
    stream_write_uint32(output_stream, 0);                          /* CachedFields length */

    stream_write(output_stream, GUAC_PRINTER_DRIVER, GUAC_PRINTER_DRIVER_LENGTH);
    stream_write(output_stream, GUAC_PRINTER_NAME,   GUAC_PRINTER_NAME_LENGTH);

    svc_plugin_send((rdpSvcPlugin*) rdpdr, output_stream);
    guac_client_log_info(rdpdr->client, "All supported devices sent.");

}


void guac_rdpdr_process_server_announce(guac_rdpdrPlugin* rdpdr,
        STREAM* input_stream) {

    unsigned int major, minor, client_id;

    stream_read_uint16(input_stream, major);
    stream_read_uint16(input_stream, minor);
    stream_read_uint32(input_stream, client_id);

    /* Must choose own client ID if minor not >= 12 */
    if (minor < 12)
        client_id = random() & 0xFFFF;

    guac_client_log_info(rdpdr->client, "Connected to RDPDR %u.%u as client 0x%04x", major, minor, client_id);

    /* Respond to announce */
    guac_rdpdr_send_client_announce_reply(rdpdr, major, minor, client_id);

    /* Name request */
    guac_rdpdr_send_client_name_request(rdpdr, "Guacamole");

}

void guac_rdpdr_process_clientid_confirm(guac_rdpdrPlugin* rdpdr, STREAM* input_stream) {
    guac_client_log_info(rdpdr->client, "Client ID confirmed");
}

void guac_rdpdr_process_device_reply(guac_rdpdrPlugin* rdpdr, STREAM* input_stream) {

    unsigned int device_id, ntstatus;
    int severity, c, n, facility, code;

    stream_read_uint32(input_stream, device_id);
    stream_read_uint32(input_stream, ntstatus);

    severity = ntstatus & 0xC0000000;
    c        = ntstatus & 0x20000000;
    n        = ntstatus & 0x10000000;
    facility = ntstatus & 0x0FFF0000;
    code     = ntstatus & 0x0000FFFF;

    /* Log error / information */
    if (device_id == GUAC_PRINTER_DEVICE_ID) {

        if (severity == 0x0)
            guac_client_log_info(rdpdr->client, "Printer connected successfully");

        else
            guac_client_log_error(rdpdr->client, "Problem connecting printer: "
                    "severity=0x%x, c=0x%x, n=0x%x, facility=0x%x, code=0x%x",
                     severity,      c,      n,      facility,      code);

    }

    else
        guac_client_log_error(rdpdr->client, "Unknown device ID: 0x%08x", device_id);

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

    int count;
    int i;

    /* Read header */
    stream_read_uint16(input_stream, count);
    stream_seek(input_stream, 2);

    /* Parse capabilities */
    for (i=0; i<count; i++) {

        int type;
        int length;

        stream_read_uint16(input_stream, type);
        stream_read_uint16(input_stream, length);

        /* Ignore all for now */
        guac_client_log_info(rdpdr->client, "Ignoring server capability set type=0x%04x, length=%i", type, length);
        stream_seek(input_stream, length - 4);

    }

    /* Send own capabilities */
    guac_rdpdr_send_client_capability(rdpdr);

}

void guac_rdpdr_process_user_loggedon(guac_rdpdrPlugin* rdpdr, STREAM* input_stream) {

    guac_client_log_info(rdpdr->client, "User logged on");
    guac_rdpdr_send_client_device_list_announce_request(rdpdr);

}

void guac_rdpdr_process_prn_cache_data(guac_rdpdrPlugin* rdpdr, STREAM* input_stream) {

    /* STUB */
    guac_client_log_info(rdpdr->client, "STUB: prn_cache_data");

}

void guac_rdpdr_process_prn_using_xps(guac_rdpdrPlugin* rdpdr, STREAM* input_stream) {

    /* STUB */
    guac_client_log_info(rdpdr->client, "STUB: prn_using_xps");

}

