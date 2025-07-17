// rdpr-smartcard.c

#include "channels/rdpdr/rdpdr-smartcard.h"
#include "channels/rdpdr/rdpdr.h"
#include "channels/rdpdr/rdpdr-utils.h"
#include "rdp.h"
#include "unicode.h"

#include <freerdp/channels/rdpdr.h>
#include <freerdp/settings.h>
#include <guacamole/client.h>
#include <guacamole/unicode.h>
#include <winpr/nt.h>
#include <winpr/stream.h>

#include <stdlib.h>

/*
Code mostly ported from pf_channel_smartcard.c in FreeRDP
*/
void guac_rdpdr_device_smartcard_iorequest_handler(
    guac_rdp_common_svc* svc,
    guac_rdpdr_device* device,
    guac_rdpdr_iorequest* iorequest,
    wStream* input_stream) {

	// UINT32 FileId = 0;
	// UINT32 CompletionId = 0;
	// NTSTATUS ioStatus = 0;

    // const uint32_t DeviceId = device->device_id;
    // FileId = iorequest->file_id;
    // CompletionId = iorequest->completion_id;
    // const uint32_t MajorFunction = iorequest->major_func;
    // const uint32_t MinorFunction = iorequest->minor_func;

    // if (MajorFunction != IRP_MJ_DEVICE_CONTROL)
    // {
    //     guac_client_log(svc->client, GUAC_LOG_WARNING, "Invalid major device recieved, expected %s, got %s.",
    //         rdpdr_irp_string(IRP_MJ_DEVICE_CONTROL), rdpdr_irp_string(MajorFunction));
    //     return;
    // }
}

void guac_rdpdr_device_smartcard_free_handler(guac_rdp_common_svc* svc,
        guac_rdpdr_device* device) {

    guac_client_log(svc->client, GUAC_LOG_WARNING, "Smartcard IOTCL freed.");

    Stream_Free(device->device_announce, 1);

}

/* See the spec here https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-rdpefs/32e34332-774b-4ead-8c9d-5d64720d6bf9 */
void guac_rdpdr_register_smartcard(guac_rdp_common_svc* svc, char* smartcard_name) {

    guac_rdpdr* rdpdr = (guac_rdpdr*) svc->data;
    int id = rdpdr->devices_registered++;

    /* Get new device */
    guac_rdpdr_device* device = &(rdpdr->devices[id]);

    /* Init device */
    device->device_id   = id;
    device->device_name = smartcard_name;
    int device_name_len = guac_utf8_strlen(device->device_name);
    device->device_type = RDPDR_DTYP_SMARTCARD;
    device->dos_name = "SCARD\0\0\0";

    int scard_name_len = (device_name_len + 1) * 2;
    /* DeviceData = 12 + driver_name + device_name */
    int device_data_len = 12 + GUAC_SMARTCARD_DRIVER_LENGTH + scard_name_len;

    /* Set up device announce stream */
    device->device_announce_len = 20 + device_data_len;
    device->device_announce = Stream_New(NULL, device->device_announce_len);

    // /* Write common information. */
    // /* DeviceType (4 bytes) */
    // Stream_Write_UINT32(device->device_announce, device->device_type);
    // /* DeviceId (4 bytes) */
    // Stream_Write_UINT32(device->device_announce, device->device_id);
    // /* PreferredDosName (8 bytes) */
    // Stream_Write(device->device_announce, device->dos_name, 8);

    // /* DeviceDataLength (4 bytes): specifies the number of bytes in the DeviceData field. */
    // Stream_Write_UINT32(device->device_announce, device_data_len);

    // /* DeviceData */
    // Stream_Write_UINT32(device->device_announce, GUAC_SMARTCARD_DRIVER_LENGTH);
    // Stream_Write_UINT32(device->device_announce, scard_name_len);
    // Stream_Write_UINT32(device->device_announce, 0); /* CachedFields length. */
    // Stream_Write(device->device_announce, GUAC_SMARTCARD_DRIVER, GUAC_SMARTCARD_DRIVER_LENGTH);
    // guac_rdp_utf8_to_utf16((const unsigned char*) device->device_name,
    //         device_name_len + 1, (char*) Stream_Pointer(device->device_announce),
    //         scard_name_len);
    // Stream_Seek(device->device_announce, scard_name_len);
    Stream_Write_UINT32(device->device_announce, RDPDR_DTYP_SMARTCARD); /* deviceType */
	Stream_Write_UINT32(
	    device->device_announce, device->device_id); /* deviceID */
	Stream_Write(device->device_announce, "SCARD\0\0\0", 8);
	Stream_Write_UINT32(device->device_announce, 6);
	Stream_Write(device->device_announce, "SCARD\0", 6);

    /* Set handlers */
    device->iorequest_handler = guac_rdpdr_device_smartcard_iorequest_handler;
    device->free_handler      = guac_rdpdr_device_smartcard_free_handler;
}

