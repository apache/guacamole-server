// rdpr-smartcard.c

#include "channels/rdpdr/rdpdr-smartcard.h"
#include "channels/rdpdr/smartcard-pcsc.h"
#include "channels/rdpdr/rdpdr.h"
#include "rdp.h"
#include "unicode.h"

#include <freerdp/channels/rdpdr.h>
#include <freerdp/settings.h>
#include <guacamole/client.h>
#include <guacamole/unicode.h>
#include <winpr/nt.h>
#include <winpr/stream.h>

#include <stdlib.h>

// This is the default value made by the pscd conf
// static const char* VICC_READER_NAME = "Virtual PCD 00 00";

void guac_rdpdr_send_completion(guac_rdp_common_svc* svc,
        guac_rdpdr_iorequest* iorequest, UINT32 status, wStream* output) {

    int length = output ? Stream_GetPosition(output) : 0;
    guac_client_log(svc->client, GUAC_LOG_DEBUG, "length: %d.", length);
    wStream* response = Stream_New(NULL, 20 + length);

    Stream_Write_UINT32(response, RDPDR_CTYP_CORE);
    Stream_Write_UINT32(response, PAKID_CORE_DEVICE_IOCOMPLETION);

    Stream_Write_UINT32(response, iorequest->device_id);
    Stream_Write_UINT32(response, iorequest->file_id);
    Stream_Write_UINT32(response, iorequest->completion_id);
    Stream_Write_UINT32(response, status);
    Stream_Write_UINT32(response, length);

    if (output && length > 0)
        Stream_Write(response, Stream_Buffer(output), length);

    guac_rdp_common_svc_write(svc, response);
}

void guac_rdpdr_device_smartcard_iorequest_handler(
    guac_rdp_common_svc* svc,
    guac_rdpdr_device* device,
    guac_rdpdr_iorequest* iorequest,
    wStream* input_stream) {

    guac_client_log(svc->client, GUAC_LOG_DEBUG,
        "Smartcard IO request received: major_func=0x%02X", iorequest->major_func);

    switch (iorequest->major_func) {

        case IRP_MJ_CREATE:
            guac_client_log(svc->client, GUAC_LOG_DEBUG, "IRP_MJ_CREATE received for smartcard.");
            guac_rdpdr_send_completion(svc, iorequest, STATUS_SUCCESS, NULL);
            return;

        case IRP_MJ_CLOSE:
            guac_client_log(svc->client, GUAC_LOG_DEBUG, "IRP_MJ_CLOSE received for smartcard.");
            guac_rdpdr_send_completion(svc, iorequest, STATUS_SUCCESS, NULL);
            return;

        case IRP_MJ_DEVICE_CONTROL: {
            UINT32 output_len = 0;
            UINT32 input_len = 0;
            UINT32 ioctl_code = 0;

            if (Stream_GetRemainingLength(input_stream) < 12) {
                guac_client_log(svc->client, GUAC_LOG_ERROR,
                    "IOCTL request too short (need at least 12 bytes).");
                guac_rdpdr_send_completion(svc, iorequest, STATUS_INVALID_PARAMETER, NULL);
                return;
            }

            Stream_Read_UINT32(input_stream, output_len); // offset 0
            Stream_Read_UINT32(input_stream, input_len);  // offset 4
            Stream_Read_UINT32(input_stream, ioctl_code); // offset 8
            (void)input_len;
            (void)output_len;

            guac_client_log(svc->client, GUAC_LOG_DEBUG,
                "Smartcard IOCTL received: 0x%08X", ioctl_code);

            wStream* out = Stream_New(NULL, 1024); // Output buffer for IOCTL response
            Stream_SetPosition(out, 0);

            switch (ioctl_code) {
                case SCARD_IOCTL_ACCESSSTARTEDEVENT:
                    guac_client_log(svc->client, GUAC_LOG_DEBUG, "Handling SCARD_IOCTL_ACCESSSTARTEDEVENT");

                    // guac_smartcard_context ctx;
                    // if (guac_smartcard_init(&ctx, VICC_READER_NAME) != 0) {
                    //     guac_client_log(svc->client, GUAC_LOG_ERROR, "smartcard init failed");
                    //     return;
                    // }

                    // UINT32 dummy_event_handle = (UINT32)(ctx.wrappedEventHandle);
                    UINT32 dummy_event_handle = 0x1000;
                    wStream* output = Stream_New(NULL, 4);
                    if (!output) {
                        guac_client_log(svc->client, GUAC_LOG_ERROR, "Failed to allocate stream for ACCESSSTARTEDEVENT output");
                        guac_rdpdr_send_completion(svc, iorequest, STATUS_NO_MEMORY, NULL);
                        return;
                    }

                    Stream_Write_UINT32(output, dummy_event_handle);

                    guac_rdpdr_send_completion(svc, iorequest, STATUS_SUCCESS, output);
                    Stream_Free(output, TRUE);
                    break;

                // --- Stubbed Supported IOCTLs (for testing) ---
                case SCARD_IOCTL_LISTREADERSA:
                case SCARD_IOCTL_LISTREADERSW:
                    guac_client_log(svc->client, GUAC_LOG_DEBUG,
                        "Stub: Handling SCARD_IOCTL_LISTREADERS[AW]");

                    // Return a single fake reader name
                    // For SCARD_IOCTL_LISTREADERSA: ASCII null-separated, double-null terminated
                    Stream_Write(out, "GuacSmartcardReader\0\0", 22);
                    guac_rdpdr_send_completion(svc, iorequest, STATUS_SUCCESS, out);
                    break;

                case SCARD_IOCTL_GETSTATUSCHANGEA:
                case SCARD_IOCTL_GETSTATUSCHANGEW:
                    guac_client_log(svc->client, GUAC_LOG_DEBUG,
                        "Stub: Handling SCARD_IOCTL_GETSTATUSCHANGE[AW]");

                    // This IOCTL expects a full response structure (beyond this stub),
                    // so this is just placeholder code and may not be enough for Windows.
                    // You’d eventually need to parse the input and construct proper PCSC structures.

                    // For now: return STATUS_PENDING or SCARD_STATE_PRESENT
                    Stream_Write_UINT32(out, SCARD_STATE_PRESENT);
                    guac_rdpdr_send_completion(svc, iorequest, STATUS_SUCCESS, out);
                    break;

                case SCARD_IOCTL_ISVALID:
                    guac_client_log(svc->client, GUAC_LOG_DEBUG,
                        "Stub: Handling SCARD_IOCTL_ISVALID");
                    guac_rdpdr_send_completion(svc, iorequest, STATUS_SUCCESS, out);
                    break;

                case SCARD_IOCTL_TRANSMIT:
                    guac_client_log(svc->client, GUAC_LOG_DEBUG,
                        "Stub: Handling SCARD_IOCTL_TRANSMIT");

                    // Example dummy APDU response (just returns "90 00" meaning success)
                    Stream_Write(out, "\x90\x00", 2);
                    guac_rdpdr_send_completion(svc, iorequest, STATUS_SUCCESS, out);
                    break;

                default:
                    guac_client_log(svc->client, GUAC_LOG_WARNING,
                        "Unhandled smartcard IOCTL: 0x%08X", ioctl_code);
                    guac_rdpdr_send_completion(svc, iorequest, STATUS_NOT_SUPPORTED, NULL);
                    break;
            }

            Stream_Free(out, FALSE);
            guac_client_log(svc->client, GUAC_LOG_DEBUG, "Finished handling smartcard IOCTL");
            return;
        }

        default:
            guac_client_log(svc->client, GUAC_LOG_WARNING,
                "Unhandled major_func for smartcard device: 0x%02X",
                iorequest->major_func);
            guac_rdpdr_send_completion(svc, iorequest, STATUS_NOT_SUPPORTED, NULL);
            return;
    }
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

