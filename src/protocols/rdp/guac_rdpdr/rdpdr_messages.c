
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
#include <freerdp/utils/svc_plugin.h>

#ifdef ENABLE_WINPR
#include <winpr/stream.h>
#include <winpr/wtypes.h>
#else
#include "compat/winpr-stream.h"
#include "compat/winpr-wtypes.h"
#endif

#include <guacamole/client.h>

#include "rdpdr_service.h"
#include "rdpdr_messages.h"
#include "rdpdr_printer.h"
#include "client.h"


static void guac_rdpdr_send_client_announce_reply(guac_rdpdrPlugin* rdpdr,
        unsigned int major, unsigned int minor, unsigned int client_id) {

    wStream* output_stream = Stream_New(NULL, 12);

    /* Write header */
    Stream_Write_UINT16(output_stream, RDPDR_CTYP_CORE);
    Stream_Write_UINT16(output_stream, PAKID_CORE_CLIENTID_CONFIRM);

    /* Write content */
    Stream_Write_UINT16(output_stream, major);
    Stream_Write_UINT16(output_stream, minor);
    Stream_Write_UINT32(output_stream, client_id);

    svc_plugin_send((rdpSvcPlugin*) rdpdr, output_stream);

}

static void guac_rdpdr_send_client_name_request(guac_rdpdrPlugin* rdpdr, const char* name) {

    int name_bytes = strlen(name) + 1;
    wStream* output_stream = Stream_New(NULL, 16 + name_bytes);

    /* Write header */
    Stream_Write_UINT16(output_stream, RDPDR_CTYP_CORE);
    Stream_Write_UINT16(output_stream, PAKID_CORE_CLIENT_NAME);

    /* Write content */
    Stream_Write_UINT32(output_stream, 0); /* ASCII */
    Stream_Write_UINT32(output_stream, 0); /* 0 required by RDPDR spec */
    Stream_Write_UINT32(output_stream, name_bytes);
    Stream_Write(output_stream, name, name_bytes);

    svc_plugin_send((rdpSvcPlugin*) rdpdr, output_stream);

}

static void guac_rdpdr_send_client_capability(guac_rdpdrPlugin* rdpdr) {

    wStream* output_stream = Stream_New(NULL, 256);
    guac_client_log_info(rdpdr->client, "Sending capabilities...");

    /* Write header */
    Stream_Write_UINT16(output_stream, RDPDR_CTYP_CORE);
    Stream_Write_UINT16(output_stream, PAKID_CORE_CLIENT_CAPABILITY);

    /* Capability count + padding */
    Stream_Write_UINT16(output_stream, 2);
    Stream_Write_UINT16(output_stream, 0); /* Padding */

    /* General capability header */
    Stream_Write_UINT16(output_stream, CAP_GENERAL_TYPE);
    Stream_Write_UINT16(output_stream, 44);
    Stream_Write_UINT32(output_stream, GENERAL_CAPABILITY_VERSION_02);

    /* General capability data */
    Stream_Write_UINT32(output_stream, GUAC_OS_TYPE);          /* osType - required to be ignored */
    Stream_Write_UINT32(output_stream, 0);                     /* osVersion */
    Stream_Write_UINT16(output_stream, RDP_CLIENT_MAJOR_ALL);  /* protocolMajor */
    Stream_Write_UINT16(output_stream, RDP_CLIENT_MINOR_5_2);  /* protocolMinor */
    Stream_Write_UINT32(output_stream, 0xFFFF);                /* ioCode1 */
    Stream_Write_UINT32(output_stream, 0);                     /* ioCode2 */
    Stream_Write_UINT32(output_stream,
                                      RDPDR_DEVICE_REMOVE_PDUS
                                    | RDPDR_CLIENT_DISPLAY_NAME
                                    | RDPDR_USER_LOGGEDON_PDU); /* extendedPDU */
    Stream_Write_UINT32(output_stream, 0);                      /* extraFlags1 */
    Stream_Write_UINT32(output_stream, 0);                      /* extraFlags2 */
    Stream_Write_UINT32(output_stream, 0);                      /* SpecialTypeDeviceCap */

    /* Printer support header */
    Stream_Write_UINT16(output_stream, CAP_PRINTER_TYPE);
    Stream_Write_UINT16(output_stream, 8);
    Stream_Write_UINT32(output_stream, PRINT_CAPABILITY_VERSION_01);

    svc_plugin_send((rdpSvcPlugin*) rdpdr, output_stream);
    guac_client_log_info(rdpdr->client, "Capabilities sent.");

}

static void guac_rdpdr_send_client_device_list_announce_request(guac_rdpdrPlugin* rdpdr) {

    int i;
    wStream* output_stream = Stream_New(NULL, 256);

    /* Write header */
    Stream_Write_UINT16(output_stream, RDPDR_CTYP_CORE);
    Stream_Write_UINT16(output_stream, PAKID_CORE_DEVICELIST_ANNOUNCE);

    /* List devices */
    Stream_Write_UINT32(output_stream, rdpdr->devices_registered);
    for (i=0; i<rdpdr->devices_registered; i++) {
        guac_rdpdr_device* device = &(rdpdr->devices[i]);
        device->announce_handler(device, output_stream, i);
        guac_client_log_info(rdpdr->client, "Registered device %i (%s)",
                device->device_id, device->device_name);
    }

    svc_plugin_send((rdpSvcPlugin*) rdpdr, output_stream);
    guac_client_log_info(rdpdr->client, "All supported devices sent.");

}


void guac_rdpdr_process_server_announce(guac_rdpdrPlugin* rdpdr,
        wStream* input_stream) {

    unsigned int major, minor, client_id;

    Stream_Read_UINT16(input_stream, major);
    Stream_Read_UINT16(input_stream, minor);
    Stream_Read_UINT32(input_stream, client_id);

    /* Must choose own client ID if minor not >= 12 */
    if (minor < 12)
        client_id = random() & 0xFFFF;

    guac_client_log_info(rdpdr->client, "Connected to RDPDR %u.%u as client 0x%04x", major, minor, client_id);

    /* Respond to announce */
    guac_rdpdr_send_client_announce_reply(rdpdr, major, minor, client_id);

    /* Name request */
    guac_rdpdr_send_client_name_request(rdpdr, "Guacamole RDP");

}

void guac_rdpdr_process_clientid_confirm(guac_rdpdrPlugin* rdpdr, wStream* input_stream) {
    guac_client_log_info(rdpdr->client, "Client ID confirmed");
}

void guac_rdpdr_process_device_reply(guac_rdpdrPlugin* rdpdr, wStream* input_stream) {

    unsigned int device_id, ntstatus;
    int severity, c, n, facility, code;

    Stream_Read_UINT32(input_stream, device_id);
    Stream_Read_UINT32(input_stream, ntstatus);

    severity = (ntstatus & 0xC0000000) >> 30;
    c        = (ntstatus & 0x20000000) >> 29;
    n        = (ntstatus & 0x10000000) >> 28;
    facility = (ntstatus & 0x0FFF0000) >> 16;
    code     =  ntstatus & 0x0000FFFF;

    /* Log error / information */
    if (device_id >= 0 && device_id < rdpdr->devices_registered) {

        if (severity == 0x0)
            guac_client_log_info(rdpdr->client, "Device %i (%s) connected successfully",
                    device_id, rdpdr->devices[device_id].device_name);

        else
            guac_client_log_error(rdpdr->client, "Problem connecting device %i (%s): "
                    "severity=0x%x, c=0x%x, n=0x%x, facility=0x%x, code=0x%x",
                     device_id, rdpdr->devices[device_id].device_name,
                     severity,      c,      n,      facility,      code);

    }

    else
        guac_client_log_error(rdpdr->client, "Unknown device ID: 0x%08x", device_id);

}

void guac_rdpdr_process_device_iorequest(guac_rdpdrPlugin* rdpdr, wStream* input_stream) {

    int device_id, file_id, completion_id, major_func, minor_func;

    /* Read header */
    Stream_Read_UINT32(input_stream, device_id);
    Stream_Read_UINT32(input_stream, file_id);
    Stream_Read_UINT32(input_stream, completion_id);
    Stream_Read_UINT32(input_stream, major_func);
    Stream_Read_UINT32(input_stream, minor_func);

    /* If printer, run printer handlers */
    if (device_id >= 0 && device_id < rdpdr->devices_registered) {

        /* Call handler on device */
        guac_rdpdr_device* device = &(rdpdr->devices[device_id]);
        device->iorequest_handler(device, input_stream,
                file_id, completion_id, major_func, minor_func);

    }

    else
        guac_client_log_error(rdpdr->client, "Unknown device ID: 0x%08x", device_id);

}

void guac_rdpdr_process_server_capability(guac_rdpdrPlugin* rdpdr, wStream* input_stream) {

    int count;
    int i;

    /* Read header */
    Stream_Read_UINT16(input_stream, count);
    Stream_Seek(input_stream, 2);

    /* Parse capabilities */
    for (i=0; i<count; i++) {

        int type;
        int length;

        Stream_Read_UINT16(input_stream, type);
        Stream_Read_UINT16(input_stream, length);

        /* Ignore all for now */
        guac_client_log_info(rdpdr->client, "Ignoring server capability set type=0x%04x, length=%i", type, length);
        Stream_Seek(input_stream, length - 4);

    }

    /* Send own capabilities */
    guac_rdpdr_send_client_capability(rdpdr);

}

void guac_rdpdr_process_user_loggedon(guac_rdpdrPlugin* rdpdr, wStream* input_stream) {

    guac_client_log_info(rdpdr->client, "User logged on");
    guac_rdpdr_send_client_device_list_announce_request(rdpdr);

}

void guac_rdpdr_process_prn_cache_data(guac_rdpdrPlugin* rdpdr, wStream* input_stream) {
    guac_client_log_info(rdpdr->client, "Ignoring printer cached configuration data");
}

void guac_rdpdr_process_prn_using_xps(guac_rdpdrPlugin* rdpdr, wStream* input_stream) {
    guac_client_log_info(rdpdr->client, "Printer unexpectedly switched to XPS mode");
}

