// rdpr-smartcard.c

#include "channels/rdpdr/rdpdr-smartcard.h"
#include "channels/rdpdr/rdpdr.h"
#include "rdp.h"
#include "unicode.h"

#include <freerdp/channels/rdpdr.h>
#include <freerdp/settings.h>

#include <guacamole/client.h>
#include <guacamole/unicode.h>

#include <winpr/nt.h>
#include <winpr/stream.h>
#include <winpr/smartcard.h>

#include <pthread.h>
#include <stdlib.h>

static BOOL pf_channel_client_write_iostatus(const guac_rdp_scard_operation* op, NTSTATUS ioStatus)
{
	UINT16 component = 0;
	UINT16 packetid = 0;
	UINT32 dID = 0;
	UINT32 cID = 0;
	size_t pos = 0;

    wStream* out = op->out;


	pos = Stream_GetPosition(out);
	Stream_SetPosition(out, 0);
	if (!Stream_CheckAndLogRequiredLength("rdp.scard", out, 16))
		return false;

	Stream_Read_UINT16(out, component);
	Stream_Read_UINT16(out, packetid);

	Stream_Read_UINT32(out, dID);
	Stream_Read_UINT32(out, cID);

	WINPR_ASSERT(component == RDPDR_CTYP_CORE);
	WINPR_ASSERT(packetid == PAKID_CORE_DEVICE_IOCOMPLETION);
	WINPR_ASSERT(dID == op->deviceID);
	WINPR_ASSERT(cID == op->completionID);

    // Prevent unused-but-set-variable warnings in release builds
    (void)component;
    (void)packetid;
    (void)dID;
    (void)cID;

	Stream_Write_INT32(out, ioStatus);
	Stream_SetPosition(out, pos);
	return true;
}

static void* guac_rdpdr_irp_thread(void* data) {
    struct guac_rdpdr_thread_arg* arg = (struct guac_rdpdr_thread_arg*) data;
    guac_rdp_common_svc* svc = arg->svc;
    guac_rdpdr_device* device = arg->device;

    guac_client_log(svc->client, GUAC_LOG_DEBUG, "guac_rdpdr_irp_thread: thread started.");
    printf( "guac_rdpdr_irp_thread: thread started.\n" );

    LONG rc;
    NTSTATUS io_status = 0;

    // Call decoder + business logic
    rc = guac_rdpdr_smartcard_irp_device_control_call(svc, arg->iorequest, &arg->op, &io_status, arg->input_stream);

    if (rc == 0) {
        guac_rdpdr_write_io_completion(arg->op.out, device, arg->iorequest->completion_id, io_status, 0);

        // TODO: write encoded response into `out`
        //       guac_rdpdr_write_response(out, &arg->op);

        guac_rdp_common_svc_write(svc, arg->op.out);
    }

    guac_client_log(svc->client, GUAC_LOG_DEBUG, "guac_rdpdr_irp_thread: thread ending.");

    free(arg->iorequest);
    free(arg);

    return NULL;
}

LONG guac_rdpdr_smartcard_irp_device_control_call(guac_rdp_common_svc* svc,
                                                  guac_rdpdr_iorequest* request,
                                                  guac_rdp_scard_operation* op,
                                                  NTSTATUS* io_status,
                                                  wStream* input_stream) {
    LONG result = 0;
	UINT32 offset = 0;
	size_t objectBufferLength = 0;

	const UINT32 ioControlCode = op->ioControlCode;

	/**
	 * [MS-RDPESC] 3.2.5.1: Sending Outgoing Messages:
	 * the output buffer length SHOULD be set to 2048
	 *
	 * Since it's a SHOULD and not a MUST, we don't care
	 * about it, but we still reserve at least 2048 bytes.
	 */
	const size_t outMaxLen = MAX(2048, op->outputBufferLength);
	if (!Stream_EnsureRemainingCapacity(op->out, outMaxLen)) {
        guac_client_log(svc->client, GUAC_LOG_ERROR, "guac_rdpdr_smartcard_irp_device_control_call: failed to ensure sufficient memory");
		return SCARD_E_NO_MEMORY;
    }

    guac_client_log(svc->client, GUAC_LOG_DEBUG, "guac_rdpdr_smartcard_irp_device_control_call: writing to stream...");

	/* Device Control Response */
	Stream_Write_UINT32(op->out, 0);                            /* OutputBufferLength (4 bytes) */
	Stream_Zero(op->out, SMARTCARD_COMMON_TYPE_HEADER_LENGTH);  /* CommonTypeHeader (8 bytes) */
	Stream_Zero(op->out, SMARTCARD_PRIVATE_TYPE_HEADER_LENGTH); /* PrivateTypeHeader (8 bytes) */
	Stream_Write_UINT32(op->out, 0);                            /* Result (4 bytes) */

    guac_client_log(svc->client, GUAC_LOG_DEBUG, "device_control_call: Smartcard IOCTL request: 0x%08X", op->ioControlCode);

    /* Call */
	switch (ioControlCode)
	{
		// case SCARD_IOCTL_ESTABLISHCONTEXT:
		// 	result = smartcard_EstablishContext_Call(smartcard, out, operation);
		// 	break;

		// case SCARD_IOCTL_RELEASECONTEXT:
		// 	result = smartcard_ReleaseContext_Call(smartcard, out, operation);
		// 	break;

		// case SCARD_IOCTL_ISVALIDCONTEXT:
		// 	result = smartcard_IsValidContext_Call(smartcard, out, operation);
		// 	break;

		// case SCARD_IOCTL_LISTREADERGROUPSA:
		// 	result = smartcard_ListReaderGroupsA_Call(smartcard, out, operation);
		// 	break;

		// case SCARD_IOCTL_LISTREADERGROUPSW:
		// 	result = smartcard_ListReaderGroupsW_Call(smartcard, out, operation);
		// 	break;

		// case SCARD_IOCTL_LISTREADERSA:
		// 	result = smartcard_ListReadersA_Call(smartcard, out, operation);
		// 	break;

		// case SCARD_IOCTL_LISTREADERSW:
		// 	result = smartcard_ListReadersW_Call(smartcard, out, operation);
		// 	break;

		// case SCARD_IOCTL_INTRODUCEREADERGROUPA:
		// 	result = smartcard_IntroduceReaderGroupA_Call(smartcard, out, operation);
		// 	break;

		// case SCARD_IOCTL_INTRODUCEREADERGROUPW:
		// 	result = smartcard_IntroduceReaderGroupW_Call(smartcard, out, operation);
		// 	break;

		// case SCARD_IOCTL_FORGETREADERGROUPA:
		// 	result = smartcard_ForgetReaderA_Call(smartcard, out, operation);
		// 	break;

		// case SCARD_IOCTL_FORGETREADERGROUPW:
		// 	result = smartcard_ForgetReaderW_Call(smartcard, out, operation);
		// 	break;

		// case SCARD_IOCTL_INTRODUCEREADERA:
		// 	result = smartcard_IntroduceReaderA_Call(smartcard, out, operation);
		// 	break;

		// case SCARD_IOCTL_INTRODUCEREADERW:
		// 	result = smartcard_IntroduceReaderW_Call(smartcard, out, operation);
		// 	break;

		// case SCARD_IOCTL_FORGETREADERA:
		// 	result = smartcard_ForgetReaderA_Call(smartcard, out, operation);
		// 	break;

		// case SCARD_IOCTL_FORGETREADERW:
		// 	result = smartcard_ForgetReaderW_Call(smartcard, out, operation);
		// 	break;

		// case SCARD_IOCTL_ADDREADERTOGROUPA:
		// 	result = smartcard_AddReaderToGroupA_Call(smartcard, out, operation);
		// 	break;

		// case SCARD_IOCTL_ADDREADERTOGROUPW:
		// 	result = smartcard_AddReaderToGroupW_Call(smartcard, out, operation);
		// 	break;

		// case SCARD_IOCTL_REMOVEREADERFROMGROUPA:
		// 	result = smartcard_RemoveReaderFromGroupA_Call(smartcard, out, operation);
		// 	break;

		// case SCARD_IOCTL_REMOVEREADERFROMGROUPW:
		// 	result = smartcard_RemoveReaderFromGroupW_Call(smartcard, out, operation);
		// 	break;

		// case SCARD_IOCTL_LOCATECARDSA:
		// 	result = smartcard_LocateCardsA_Call(smartcard, out, operation);
		// 	break;

		// case SCARD_IOCTL_LOCATECARDSW:
		// 	result = smartcard_LocateCardsW_Call(smartcard, out, operation);
		// 	break;

		// case SCARD_IOCTL_GETSTATUSCHANGEA:
		// 	result = smartcard_GetStatusChangeA_Call(smartcard, out, operation);
		// 	break;

		// case SCARD_IOCTL_GETSTATUSCHANGEW:
		// 	result = smartcard_GetStatusChangeW_Call(smartcard, out, operation);
		// 	break;

		// case SCARD_IOCTL_CANCEL:
		// 	result = smartcard_Cancel_Call(smartcard, out, operation);
		// 	break;

		// case SCARD_IOCTL_CONNECTA:
		// 	result = smartcard_ConnectA_Call(smartcard, out, operation);
		// 	break;

		// case SCARD_IOCTL_CONNECTW:
		// 	result = smartcard_ConnectW_Call(smartcard, out, operation);
		// 	break;

		// case SCARD_IOCTL_RECONNECT:
		// 	result = smartcard_Reconnect_Call(smartcard, out, operation);
		// 	break;

		// case SCARD_IOCTL_DISCONNECT:
		// 	result = smartcard_Disconnect_Call(smartcard, out, operation);
		// 	break;

		// case SCARD_IOCTL_BEGINTRANSACTION:
		// 	result = smartcard_BeginTransaction_Call(smartcard, out, operation);
		// 	break;

		// case SCARD_IOCTL_ENDTRANSACTION:
		// 	result = smartcard_EndTransaction_Call(smartcard, out, operation);
		// 	break;

		// case SCARD_IOCTL_STATE:
		// 	result = smartcard_State_Call(smartcard, out, operation);
		// 	break;

		// case SCARD_IOCTL_STATUSA:
		// 	result = smartcard_StatusA_Call(smartcard, out, operation);
		// 	break;

		// case SCARD_IOCTL_STATUSW:
		// 	result = smartcard_StatusW_Call(smartcard, out, operation);
		// 	break;

		// case SCARD_IOCTL_TRANSMIT:
		// 	result = smartcard_Transmit_Call(smartcard, out, operation);
		// 	break;

		// case SCARD_IOCTL_CONTROL:
		// 	result = smartcard_Control_Call(smartcard, out, operation);
		// 	break;

		// case SCARD_IOCTL_GETATTRIB:
		// 	result = smartcard_GetAttrib_Call(smartcard, out, operation);
		// 	break;

		// case SCARD_IOCTL_SETATTRIB:
		// 	result = smartcard_SetAttrib_Call(smartcard, out, operation);
		// 	break;

		// case SCARD_IOCTL_ACCESSSTARTEDEVENT:
		// 	result = smartcard_AccessStartedEvent_Call(smartcard, out, operation);
		// 	break;

		// case SCARD_IOCTL_LOCATECARDSBYATRA:
		// 	result = smartcard_LocateCardsByATRA_Call(smartcard, out, operation);
		// 	break;

		// case SCARD_IOCTL_LOCATECARDSBYATRW:
		// 	result = smartcard_LocateCardsW_Call(smartcard, out, operation);
		// 	break;

		// case SCARD_IOCTL_READCACHEA:
		// 	result = smartcard_ReadCacheA_Call(smartcard, out, operation);
		// 	break;

		// case SCARD_IOCTL_READCACHEW:
		// 	result = smartcard_ReadCacheW_Call(smartcard, out, operation);
		// 	break;

		// case SCARD_IOCTL_WRITECACHEA:
		// 	result = smartcard_WriteCacheA_Call(smartcard, out, operation);
		// 	break;

		// case SCARD_IOCTL_WRITECACHEW:
		// 	result = smartcard_WriteCacheW_Call(smartcard, out, operation);
		// 	break;

		// case SCARD_IOCTL_GETTRANSMITCOUNT:
		// 	result = smartcard_GetTransmitCount_Call(smartcard, out, operation);
		// 	break;

		// case SCARD_IOCTL_RELEASETARTEDEVENT:
		// 	result = smartcard_ReleaseStartedEvent_Call(smartcard, out, operation);
		// 	break;

		// case SCARD_IOCTL_GETREADERICON:
		// 	result = smartcard_GetReaderIcon_Call(smartcard, out, operation);
		// 	break;

		// case SCARD_IOCTL_GETDEVICETYPEID:
		// 	result = smartcard_GetDeviceTypeId_Call(smartcard, out, operation);
		// 	break;

		// default:
		// 	result = STATUS_UNSUCCESSFUL;
		// 	break;
	}

	/**
	 * [MS-RPCE] 2.2.6.3 Primitive Type Serialization
	 * The type MUST be aligned on an 8-byte boundary. If the size of the
	 * primitive type is not a multiple of 8 bytes, the data MUST be padded.
	 */

	if ((ioControlCode != SCARD_IOCTL_ACCESSSTARTEDEVENT) &&
	    (ioControlCode != SCARD_IOCTL_RELEASETARTEDEVENT))
	{
		offset = (RDPDR_DEVICE_IO_RESPONSE_LENGTH + RDPDR_DEVICE_IO_CONTROL_RSP_HDR_LENGTH);
		smartcard_pack_write_size_align(svc, op->out, Stream_GetPosition(op->out) - offset, 8);
	}

	if ((result != SCARD_S_SUCCESS) && (result != SCARD_E_TIMEOUT) &&
	    (result != SCARD_E_NO_READERS_AVAILABLE) && (result != SCARD_E_NO_SERVICE) &&
	    (result != SCARD_W_CACHE_ITEM_NOT_FOUND) && (result != SCARD_W_CACHE_ITEM_STALE))
	{
		guac_client_log(svc->client, GUAC_LOG_WARNING,
		           "IRP failure: %s (0x%08" PRIX32 "), status: %s (0x%08" PRIX32 ")",
		           scard_get_ioctl_string(ioControlCode, TRUE), ioControlCode,
		           SCardGetErrorString(result), result);
	}

	*io_status = STATUS_SUCCESS;

	if ((result & 0xC0000000L) == 0xC0000000L)
	{
		/* NTSTATUS error */
		*io_status = result;
		guac_client_log(svc->client, GUAC_LOG_WARNING,
		           "IRP failure: %s (0x%08" PRIX32 "), ntstatus: 0x%08" PRIX32 "",
		           scard_get_ioctl_string(ioControlCode, TRUE), ioControlCode, result);
	}

	Stream_SealLength(op->out);
	size_t outputBufferLength = Stream_Length(op->out);
	WINPR_ASSERT(outputBufferLength >= RDPDR_DEVICE_IO_RESPONSE_LENGTH + 4U);
	outputBufferLength -= (RDPDR_DEVICE_IO_RESPONSE_LENGTH + 4U);
	WINPR_ASSERT(outputBufferLength >= RDPDR_DEVICE_IO_RESPONSE_LENGTH);
	objectBufferLength = outputBufferLength - RDPDR_DEVICE_IO_RESPONSE_LENGTH;
	WINPR_ASSERT(outputBufferLength <= UINT32_MAX);
	WINPR_ASSERT(objectBufferLength <= UINT32_MAX);
	Stream_SetPosition(op->out, RDPDR_DEVICE_IO_RESPONSE_LENGTH);

	/* [MS-RDPESC] 3.2.5.2 Processing Incoming Replies
	 *
	 * if the output buffer is too small, reply with STATUS_BUFFER_TOO_SMALL
	 * and a outputBufferLength of 0.
	 * The message should then be retransmitted from the server with a doubled
	 * buffer size.
	 */
	if (outputBufferLength > op->outputBufferLength)
	{
		guac_client_log(svc->client, GUAC_LOG_WARNING,
		           "IRP warn: expected outputBufferLength %" PRIu32 ", but current limit %" PRIu32
		           ", respond with STATUS_BUFFER_TOO_SMALL",
		           op->outputBufferLength, outputBufferLength);

		*io_status = STATUS_BUFFER_TOO_SMALL;
		result = *io_status;
		outputBufferLength = 0;
		objectBufferLength = 0;
	}

	/* Device Control Response */
	Stream_Write_UINT32(op->out, (UINT32)outputBufferLength); /* OutputBufferLength (4 bytes) */
	smartcard_pack_common_type_header(op->out);               /* CommonTypeHeader (8 bytes) */
	smartcard_pack_private_type_header(
	    op->out, (UINT32)objectBufferLength); /* PrivateTypeHeader (8 bytes) */
	Stream_Write_INT32(op->out, result);      /* Result (4 bytes) */
	Stream_SetPosition(op->out, Stream_Length(op->out));

	return SCARD_S_SUCCESS;
}

LONG smartcard_pack_write_size_align(guac_rdp_common_svc* svc, wStream* s, size_t size, UINT32 alignment)
{
	size_t pad = 0;

	pad = size;
	size = (size + alignment - 1) & ~(alignment - 1);
	pad = size - pad;

	if (pad)
	{
		if (!Stream_EnsureRemainingCapacity(s, pad))
		{
			guac_client_log(svc->client, GUAC_LOG_ERROR, "Stream_EnsureRemainingCapacity failed!");
			return SCARD_F_INTERNAL_ERROR;
		}

		Stream_Zero(s, pad);
	}

	return SCARD_S_SUCCESS;
}

void smartcard_pack_common_type_header(wStream* s)
{
	Stream_Write_UINT8(s, 1);           /* Version (1 byte) */
	Stream_Write_UINT8(s, 0x10);        /* Endianness (1 byte) */
	Stream_Write_UINT16(s, 8);          /* CommonHeaderLength (2 bytes) */
	Stream_Write_UINT32(s, 0xCCCCCCCC); /* Filler (4 bytes), should be 0xCCCCCCCC */
}

void smartcard_pack_private_type_header(wStream* s, UINT32 objectBufferLength)
{
	Stream_Write_UINT32(s, objectBufferLength); /* ObjectBufferLength (4 bytes) */
	Stream_Write_UINT32(s, 0x00000000);         /* Filler (4 bytes), should be 0x00000000 */
}

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

    guac_client_log(svc->client, GUAC_LOG_DEBUG, "guac_rdpdr_device_smartcard_iorequest_handler: handling smartcard io request.");

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

    switch (op.ioControlCode)
	{
		case SCARD_IOCTL_LISTREADERGROUPSA:
		case SCARD_IOCTL_LISTREADERGROUPSW:
		case SCARD_IOCTL_LISTREADERSA:
		case SCARD_IOCTL_LISTREADERSW:
		case SCARD_IOCTL_LOCATECARDSA:
		case SCARD_IOCTL_LOCATECARDSW:
		case SCARD_IOCTL_LOCATECARDSBYATRA:
		case SCARD_IOCTL_LOCATECARDSBYATRW:
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
			if (!guac_rdpdr_start_irp_thread(svc, device, iorequest, input_stream)) {
                guac_client_log(svc->client, GUAC_LOG_ERROR, "Smartcard IOCTL: failed to guac_rdpdr_start_irp_thread.");
				goto fail;
            }
			return;

		default:
			status = guac_rdpdr_smartcard_irp_device_control_call(svc, iorequest, &op, &ioStatus, input_stream);
			if (status != 0) {
                guac_client_log(svc->client, GUAC_LOG_ERROR, "guac_rdpdr_smartcard_irp_device_control_call: non-zero status.");
				goto fail;
            }
			pf_channel_client_write_iostatus(&op, ioStatus);

            // Commit the io to the rdpc channel
            guac_client_log(svc->client, GUAC_LOG_DEBUG, "guac_rdpdr_smartcard_irp_device_control_call: response sent over channel.");
            guac_rdp_common_svc_write(svc, op.out);
			break;
	}

    guac_client_log(svc->client, GUAC_LOG_DEBUG, "guac_rdpdr_smartcard_irp_device_control_call: completed.");

fail:
	smartcard_operation_free(&op, FALSE);
	return;
}

LONG guac_rdpdr_smartcard_irp_device_control_decode(wStream* input_stream,
                                                    UINT32 CompletionId,
                                                    UINT32 FileId,
                                                    guac_rdp_scard_operation* operation) {
    UINT32 output_len = 0;
    UINT32 input_len = 0;
    UINT32 ioctl_code = 0;
    LONG status = 0;

    if (Stream_GetRemainingLength(input_stream) < 32) {
        guac_client_log(operation->client, GUAC_LOG_ERROR, "Smartcard IOCTL: stream too short.");
        return STATUS_INVALID_PARAMETER;
    }

    Stream_Read_UINT32(input_stream, output_len);
    Stream_Read_UINT32(input_stream, input_len);
    Stream_Read_UINT32(input_stream, ioctl_code);
    Stream_Seek(input_stream, 20); // padding

    operation->ioControlCode = ioctl_code;
    operation->outputBufferLength = output_len;

    if (Stream_GetRemainingLength(input_stream) < input_len) {
        guac_client_log(operation->client, GUAC_LOG_WARNING,
            "Smartcard IOCTL: mismatch in input buffer size. Expected: %u, Remaining: %zu",
            input_len, Stream_GetRemainingLength(input_stream));
        return STATUS_INVALID_PARAMETER;
    }

    guac_client_log(operation->client, GUAC_LOG_DEBUG, "control_decode: Smartcard IOCTL request: 0x%08X", ioctl_code);

    operation->out = Stream_New(NULL, output_len);

    if ((ioctl_code != SCARD_IOCTL_ACCESSSTARTEDEVENT) &&
        (ioctl_code != SCARD_IOCTL_RELEASETARTEDEVENT)) {

        status = guac_rdpdr_scard_unpack_common_type_header(input_stream, operation->client);
        if (status != SCARD_S_SUCCESS)
            return status;

        status = guac_rdpdr_scard_unpack_private_type_header(input_stream, operation->client);
        if (status != SCARD_S_SUCCESS)
            return status;
    }

    // Dispatch decode based on IOCTL
    switch (ioctl_code) {

        case SCARD_IOCTL_ESTABLISHCONTEXT:
            status = smartcard_EstablishContext_Decode(input_stream, operation);
            break;

        case SCARD_IOCTL_RELEASECONTEXT:
            status = smartcard_ReleaseContext_Decode(input_stream, operation);
            break;

        case SCARD_IOCTL_ACCESSSTARTEDEVENT:
            status = smartcard_AccessStartedEvent_Decode(input_stream, operation);
            break;

        default:
            guac_client_log(operation->client, GUAC_LOG_WARNING,
                "Smartcard IOCTL: Unsupported code 0x%08X", ioctl_code);
            status = STATUS_NOT_IMPLEMENTED;
            break;
    }

    return status;
}

// TODO: double check against source code, in `smartcard_unpack.c`
LONG guac_rdpdr_scard_unpack_common_type_header(wStream* s, guac_client* client) {
    UINT8 version = 0;
    UINT8 endianness = 0;
    UINT16 commonHeaderLength = 0;
    UINT32 filler = 0;

    if (Stream_GetRemainingLength(s) < 8) {
        guac_client_log(client, GUAC_LOG_ERROR,
            "CommonTypeHeader too short: need 8 bytes.");
        return STATUS_BUFFER_TOO_SMALL;
    }

    Stream_Read_UINT8(s, version);
    Stream_Read_UINT8(s, endianness);
    Stream_Read_UINT16(s, commonHeaderLength);
    Stream_Read_UINT32(s, filler);

    if (version != 1) {
        guac_client_log(client, GUAC_LOG_WARNING,
            "Unsupported CommonTypeHeader version: %u", version);
        return STATUS_INVALID_PARAMETER;
    }

    if (endianness != 0x10) {
        guac_client_log(client, GUAC_LOG_WARNING,
            "Unsupported CommonTypeHeader endianness: 0x%02X", endianness);
        return STATUS_INVALID_PARAMETER;
    }

    if (commonHeaderLength != 8) {
        guac_client_log(client, GUAC_LOG_WARNING,
            "Unexpected CommonHeaderLength: %u", commonHeaderLength);
        return STATUS_INVALID_PARAMETER;
    }

    if (filler != 0xCCCCCCCC) {
        guac_client_log(client, GUAC_LOG_WARNING,
            "Unexpected filler value: 0x%08" PRIX32, filler);
        return STATUS_INVALID_PARAMETER;
    }

    return SCARD_S_SUCCESS;
}

// TODO: double check against source code, in `smartcard_unpack.c`
LONG guac_rdpdr_scard_unpack_private_type_header(wStream* s, guac_client* client) {
    // In FreeRDP this is typically 4 or 8 bytes (depends on operation)
    if (Stream_GetRemainingLength(s) < 4) {
        guac_client_log(client, GUAC_LOG_ERROR,
            "PrivateTypeHeader too short.");
        return STATUS_BUFFER_TOO_SMALL;
    }

    // You could read fields here if needed later. For now, just skip 4 bytes.
    Stream_Seek(s, 4);
    return SCARD_S_SUCCESS;
}

LONG smartcard_EstablishContext_Decode(wStream* stream, guac_rdp_scard_operation* op) {
    guac_client_log(op->client, GUAC_LOG_INFO,
        "smartcard_EstablishContext_Decode is not implemented yet.");
    return STATUS_NOT_IMPLEMENTED;
}

LONG smartcard_ReleaseContext_Decode(wStream* stream, guac_rdp_scard_operation* op) {
    guac_client_log(op->client, GUAC_LOG_INFO,
        "smartcard_ReleaseContext_Decode is not implemented yet.");
    return STATUS_NOT_IMPLEMENTED;
}

LONG smartcard_AccessStartedEvent_Decode(wStream* stream, guac_rdp_scard_operation* op) {
    guac_client_log(op->client, GUAC_LOG_DEBUG, "smartcard_AccessStartedEvent_Decode");
    INT32 unused = 0;

    if (Stream_GetRemainingLength(stream) < 4) {
        guac_client_log(op->client, GUAC_LOG_ERROR,
            "smartcard_AccessStartedEvent_Decode: stream too short.");
        return SCARD_F_INTERNAL_ERROR;
    }

    Stream_Read_INT32(stream, unused); // This value is unused per FreeRDP

    guac_client_log(op->client, GUAC_LOG_DEBUG,
        "Handled SCARD_IOCTL_ACCESSSTARTEDEVENT (discarded 4-byte INT32: %d)", unused);

    return SCARD_S_SUCCESS;
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

bool guac_rdpdr_start_irp_thread(guac_rdp_common_svc* svc, guac_rdpdr_device* device, const guac_rdpdr_iorequest* request, wStream* input_stream) {
    pthread_t thread_id;
    struct guac_rdpdr_thread_arg* arg = NULL;

    printf( "guac_rdpdr_start_irp_thread: creating a thread.\n" );

    arg = calloc(1, sizeof(struct guac_rdpdr_thread_arg));
    if (!arg) {
        guac_client_log(svc->client, GUAC_LOG_ERROR, "guac_rdpdr_start_irp_thread: failed to calloc guac_rdpdr_thread_arg");
        goto fail;
    }

    arg->svc = svc;
    arg->device = device;
    arg->input_stream = input_stream;

    arg->iorequest = malloc(sizeof(guac_rdpdr_iorequest));
    if (!arg->iorequest) {
        guac_client_log(svc->client, GUAC_LOG_ERROR, "guac_rdpdr_start_irp_thread: failed to malloc guac_rdpdr_iorequest");
        goto fail;
    }

    memcpy(arg->iorequest, request, sizeof(guac_rdpdr_iorequest));
    memset(&arg->op, 0, sizeof(arg->op));

    if (pthread_create(&thread_id, NULL, guac_rdpdr_irp_thread, arg) != 0) {
        guac_client_log(svc->client, GUAC_LOG_ERROR, "guac_rdpdr_start_irp_thread: failed create thread.");
        goto fail;
    }

    pthread_detach(thread_id);
    return true;

fail:
    if (arg) {
        free(arg->iorequest);
        free(arg);
    }
    return false;
}

void smartcard_operation_free(guac_rdp_scard_operation* op, BOOL allocated)
{
	if (!op)
		return;
	switch (op->ioControlCode)
	{
		case SCARD_IOCTL_CANCEL:
		case SCARD_IOCTL_ACCESSSTARTEDEVENT:
		case SCARD_IOCTL_RELEASETARTEDEVENT:
		case SCARD_IOCTL_LISTREADERGROUPSA:
		case SCARD_IOCTL_LISTREADERGROUPSW:
		case SCARD_IOCTL_RECONNECT:
		case SCARD_IOCTL_DISCONNECT:
		case SCARD_IOCTL_BEGINTRANSACTION:
		case SCARD_IOCTL_ENDTRANSACTION:
		case SCARD_IOCTL_STATE:
		case SCARD_IOCTL_STATUSA:
		case SCARD_IOCTL_STATUSW:
		case SCARD_IOCTL_ESTABLISHCONTEXT:
		case SCARD_IOCTL_RELEASECONTEXT:
		case SCARD_IOCTL_ISVALIDCONTEXT:
		case SCARD_IOCTL_GETATTRIB:
		case SCARD_IOCTL_GETTRANSMITCOUNT:
			break;

		case SCARD_IOCTL_LOCATECARDSA:
		break;
		case SCARD_IOCTL_LOCATECARDSW:
		break;

		case SCARD_IOCTL_LOCATECARDSBYATRA:
		break;
		case SCARD_IOCTL_LOCATECARDSBYATRW:
		break;
		case SCARD_IOCTL_FORGETREADERA:
		case SCARD_IOCTL_INTRODUCEREADERGROUPA:
		case SCARD_IOCTL_FORGETREADERGROUPA:
		break;

		case SCARD_IOCTL_FORGETREADERW:
		case SCARD_IOCTL_INTRODUCEREADERGROUPW:
		case SCARD_IOCTL_FORGETREADERGROUPW:
		break;

		case SCARD_IOCTL_INTRODUCEREADERA:
		case SCARD_IOCTL_REMOVEREADERFROMGROUPA:
		case SCARD_IOCTL_ADDREADERTOGROUPA:
		break;

		case SCARD_IOCTL_INTRODUCEREADERW:
		case SCARD_IOCTL_REMOVEREADERFROMGROUPW:
		case SCARD_IOCTL_ADDREADERTOGROUPW:
		break;

		case SCARD_IOCTL_LISTREADERSA:
		case SCARD_IOCTL_LISTREADERSW:
		break;
		case SCARD_IOCTL_GETSTATUSCHANGEA:
		break;

		case SCARD_IOCTL_GETSTATUSCHANGEW:
		break;
		case SCARD_IOCTL_GETREADERICON:
		break;
		case SCARD_IOCTL_GETDEVICETYPEID:
		break;
		case SCARD_IOCTL_CONNECTA:
		break;
		case SCARD_IOCTL_CONNECTW:
		break;
		case SCARD_IOCTL_SETATTRIB:
			break;
		case SCARD_IOCTL_TRANSMIT:
		break;
		case SCARD_IOCTL_CONTROL:
		break;
		case SCARD_IOCTL_READCACHEA:
		break;
		case SCARD_IOCTL_READCACHEW:
		break;
		case SCARD_IOCTL_WRITECACHEA:
		break;
		case SCARD_IOCTL_WRITECACHEW:
		break;
		default:
			break;
	}

	{
		guac_rdp_scard_operation empty = { 0 };
		*op = empty;
	}

	if (allocated)
		free(op);
}
