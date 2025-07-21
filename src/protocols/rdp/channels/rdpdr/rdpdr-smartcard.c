// rdpr-smartcard.c

#include "channels/rdpdr/rdpdr-smartcard.h"
#include "channels/rdpdr/rdpdr.h"
#include "channels/rdpdr/remote-smartcard.h"
#include "channels/rdpdr/scard.h"
#include "channels/rdpdr/smartcard-call.h"
#include "channels/rdpdr/smartcard-pack.h"
#include "channels/rdpdr/smartcard-operations.h"
#include "rdp.h"
#include "unicode.h"

#include <freerdp/channels/rdpdr.h>
#include <freerdp/settings.h>

#include <guacamole/client.h>
#include <guacamole/unicode.h>

#include <winpr/nt.h>
#include <winpr/collections.h>
#include <winpr/stream.h>
#include <winpr/smartcard.h>

#include <pthread.h>
#include <stdlib.h>

BOOL rdpdr_write_iocompletion_header(wStream* out, UINT32 DeviceId, UINT32 CompletionId,
                                     NTSTATUS ioStatus)
{
	WINPR_ASSERT(out);
	Stream_SetPosition(out, 0);
	if (!Stream_EnsureRemainingCapacity(out, 16))
		return FALSE;
	Stream_Write_UINT16(out, RDPDR_CTYP_CORE);                /* Component (2 bytes) */
	Stream_Write_UINT16(out, PAKID_CORE_DEVICE_IOCOMPLETION); /* PacketId (2 bytes) */
	Stream_Write_UINT32(out, DeviceId);                       /* DeviceId (4 bytes) */
	Stream_Write_UINT32(out, CompletionId);                   /* CompletionId (4 bytes) */
	Stream_Write_INT32(out, ioStatus);                        /* IoStatus (4 bytes) */

	return TRUE;
}

/*
Code mostly ported from pf_channel_smartcard.c in FreeRDP
*/
void guac_rdpdr_device_smartcard_iorequest_handler(
    guac_rdp_common_svc* svc,
    guac_rdpdr_device* device,
    guac_rdpdr_iorequest* iorequest,
    wStream* input_stream) {

    SMARTCARD_CONTEXT* context = (SMARTCARD_CONTEXT*)device->data;
    WINPR_ASSERT(context);

	UINT32 FileId = 0;
	UINT32 CompletionId = 0;
    NTSTATUS ioStatus = 0;

    FileId = iorequest->file_id;
    CompletionId = iorequest->completion_id;
    const uint32_t MajorFunction = iorequest->major_func;

    if (MajorFunction != IRP_MJ_DEVICE_CONTROL)
    {
        guac_client_log(svc->client, GUAC_LOG_WARNING, "Invalid major device recieved, expected %s, got %s.",
            rdpdr_irp_string(IRP_MJ_DEVICE_CONTROL), rdpdr_irp_string(MajorFunction));
        return;
    }

    wStream* output_stream = Stream_New(NULL, 16);

    // size_t start_pos = Stream_GetPosition(output_stream);

    if (!rdpdr_write_iocompletion_header(output_stream, device->device_id, CompletionId, 0)) {
        guac_client_log(svc->client, GUAC_LOG_ERROR, "guac_rdpdr_device_smartcard_iorequest_handler: failed to write iocompletion header.");
    }

    if (Stream_GetRemainingLength(input_stream) < 12) {
        guac_client_log(svc->client, GUAC_LOG_ERROR,
            "IOCTL request too short (need at least 12 bytes).");
        guac_rdpdr_write_io_completion(output_stream, device, CompletionId, STATUS_INVALID_PARAMETER, 0);
        guac_rdp_common_svc_write(svc, output_stream);
        return;
    }

    guac_rdp_scard_operation op;
    op.client = svc->client;
    op.out = output_stream;
    LONG status = guac_rdpdr_smartcard_irp_device_control_decode(input_stream, CompletionId, FileId, &op);

    if (status != 0) {
        guac_client_log(svc->client, GUAC_LOG_ERROR, "IOCTL decode status != 0.");
        guac_rdpdr_write_io_completion(output_stream, device, CompletionId, STATUS_INVALID_PARAMETER, 0);
        guac_rdp_common_svc_write(svc, output_stream);
        return;
    }

    guac_client_log(svc->client, GUAC_LOG_DEBUG, "iorequest_handler: Smartcard IOCTL request: 0x%08X, %s", op.ioControlCode, scard_get_ioctl_string(op.ioControlCode, true));

    BOOL asyncIrp = TRUE;

    switch (op.ioControlCode)
    {
        case SCARD_IOCTL_ESTABLISHCONTEXT:
        case SCARD_IOCTL_RELEASECONTEXT:
        case SCARD_IOCTL_ISVALIDCONTEXT:
        case SCARD_IOCTL_CANCEL:
        case SCARD_IOCTL_ACCESSSTARTEDEVENT:
        case SCARD_IOCTL_RELEASETARTEDEVENT:
            asyncIrp = FALSE;
            break;

        case SCARD_IOCTL_LISTREADERGROUPSA:
        case SCARD_IOCTL_LISTREADERGROUPSW:
        case SCARD_IOCTL_LISTREADERSA:
        case SCARD_IOCTL_LISTREADERSW:
        case SCARD_IOCTL_INTRODUCEREADERGROUPA:
        case SCARD_IOCTL_INTRODUCEREADERGROUPW:
        case SCARD_IOCTL_FORGETREADERGROUPA:
        case SCARD_IOCTL_FORGETREADERGROUPW:
        case SCARD_IOCTL_INTRODUCEREADERA:
        case SCARD_IOCTL_INTRODUCEREADERW:
        case SCARD_IOCTL_FORGETREADERA:
        case SCARD_IOCTL_FORGETREADERW:
        case SCARD_IOCTL_ADDREADERTOGROUPA:
        case SCARD_IOCTL_ADDREADERTOGROUPW:
        case SCARD_IOCTL_REMOVEREADERFROMGROUPA:
        case SCARD_IOCTL_REMOVEREADERFROMGROUPW:
        case SCARD_IOCTL_LOCATECARDSA:
        case SCARD_IOCTL_LOCATECARDSW:
        case SCARD_IOCTL_LOCATECARDSBYATRA:
        case SCARD_IOCTL_LOCATECARDSBYATRW:
        case SCARD_IOCTL_READCACHEA:
        case SCARD_IOCTL_READCACHEW:
        case SCARD_IOCTL_WRITECACHEA:
        case SCARD_IOCTL_WRITECACHEW:
        case SCARD_IOCTL_GETREADERICON:
        case SCARD_IOCTL_GETDEVICETYPEID:
        case SCARD_IOCTL_GETSTATUSCHANGEA:
        case SCARD_IOCTL_GETSTATUSCHANGEW:
        case SCARD_IOCTL_CONNECTA:
        case SCARD_IOCTL_CONNECTW:
        case SCARD_IOCTL_RECONNECT:
        case SCARD_IOCTL_DISCONNECT:
        case SCARD_IOCTL_BEGINTRANSACTION:
        case SCARD_IOCTL_ENDTRANSACTION:
        case SCARD_IOCTL_STATE:
        case SCARD_IOCTL_STATUSA:
        case SCARD_IOCTL_STATUSW:
        case SCARD_IOCTL_TRANSMIT:
        case SCARD_IOCTL_CONTROL:
        case SCARD_IOCTL_GETATTRIB:
        case SCARD_IOCTL_SETATTRIB:
        case SCARD_IOCTL_GETTRANSMITCOUNT:
            asyncIrp = TRUE;
            break;
        default:
            break;
    }

    if (!asyncIrp)
    {
        status = guac_rdpdr_smartcard_irp_device_control_call(svc, context->call_context, iorequest, &op, &ioStatus);

        smartcard_operation_free(&op, FALSE);

        if (status)
        {
            guac_client_log(svc->client, GUAC_LOG_ERROR, "smartcard_irp_device_control_call failed with error %" PRId32 "!",
                        status);
            return;
        }
    } else {
        guac_client_log(svc->client, GUAC_LOG_ERROR, "Async IOCTL not supported yet; handling syncronously.");
        status = guac_rdpdr_smartcard_irp_device_control_call(svc, context->call_context, iorequest, &op, &ioStatus);

        smartcard_operation_free(&op, FALSE);

        if (status)
        {
            guac_client_log(svc->client, GUAC_LOG_ERROR, "smartcard_irp_device_control_call failed with error %" PRId32 "!",
                        status);
            return;
        }
    }

    // guac_client_log(svc->client, GUAC_LOG_DEBUG, "Writing stream to service...");
    guac_rdp_common_svc_write(svc, output_stream);
    guac_client_log(svc->client, GUAC_LOG_DEBUG, "\tCompleted IOCTL request.");

	return;
}

void guac_rdpdr_device_smartcard_free_handler(guac_rdp_common_svc* svc,
        guac_rdpdr_device* device) {
    guac_client_log(svc->client, GUAC_LOG_WARNING, "Freeing smartcard...");

    if (!device || !device->data)
        return;

    SMARTCARD_CONTEXT* smartcard_ctx = (SMARTCARD_CONTEXT*)device->data;

    if (smartcard_ctx->call_context) {
        if (smartcard_ctx->call_context->smartcard) {
            RemoteSmartcard_free(smartcard_ctx->call_context->smartcard);
            free(smartcard_ctx->call_context->smartcard);
        }

        if (smartcard_ctx->call_context->names) {
            LinkedList_Free(smartcard_ctx->call_context->names);
        }

        free(smartcard_ctx->call_context);
    }

    free(smartcard_ctx);
    device->data = NULL;

    Stream_Free(device->device_announce, 1);

    guac_client_log(svc->client, GUAC_LOG_WARNING, "Smartcard freed.");
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

    Stream_Write_UINT32(device->device_announce, RDPDR_DTYP_SMARTCARD); /* deviceType */
	Stream_Write_UINT32(
	    device->device_announce, device->device_id); /* deviceID */
	Stream_Write(device->device_announce, "SCARD\0\0\0", 8);
	Stream_Write_UINT32(device->device_announce, 6);
	Stream_Write(device->device_announce, "SCARD\0", 6);

    /* Set handlers */
    device->iorequest_handler = guac_rdpdr_device_smartcard_iorequest_handler;
    device->free_handler      = guac_rdpdr_device_smartcard_free_handler;

    if (!assign_smartcard_context_to_device(device, svc->client)) {
        guac_client_log(svc->client, GUAC_LOG_ERROR, "Failed to create smartcard context.");
    }
}

// Function to create and initialize a SMARTCARD_CONTEXT and assign it to the device
int assign_smartcard_context_to_device(guac_rdpdr_device* device, guac_client* client) {
    if (!device)
        return -1;

    // Allocate SMARTCARD_CONTEXT
    SMARTCARD_CONTEXT* smartcard_ctx = (SMARTCARD_CONTEXT*)calloc(1, sizeof(SMARTCARD_CONTEXT));
    if (!smartcard_ctx)
        return -1;

    // Allocate scard_call_context
    scard_call_context* call_context = (scard_call_context*)calloc(1, sizeof(scard_call_context));
    if (!call_context) {
        free(smartcard_ctx);
        return -1;
    }

    call_context->names = LinkedList_New();

    // Allocate RemoteSmartcard
    RemoteSmartcard* remote_scard = (RemoteSmartcard*)calloc(1, sizeof(RemoteSmartcard));
    if (!remote_scard) {
        free(call_context);
        free(smartcard_ctx);
        return -1;
    }

    // Store allocations
    call_context->smartcard = remote_scard;
    smartcard_ctx->call_context = call_context;
    device->data = (void*)smartcard_ctx;

    remote_scard->log_client = client;

    return 0;
}
