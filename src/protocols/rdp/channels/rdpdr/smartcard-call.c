#include "channels/rdpdr/smartcard-call.h"
#include "channels/rdpdr/scard.h"
#include "channels/rdpdr/smartcard-pack.h"
#include "channels/rdpdr/remote-smartcard.h"
#include "channels/rdpdr/msz-unicode.h"

#include <guacamole/client.h>
#include "channels/rdpdr/rdpdr.h"

#include <freerdp/channels/rdpdr.h>

#include <winpr/crt.h>
#include <winpr/stream.h>
#include <winpr/smartcard.h>

static LONG scard_log_status_error_wlog(wLog* log, const char* what, LONG status)
{
	if (status != SCARD_S_SUCCESS)
	{
		DWORD level = WLOG_ERROR;
		switch (status)
		{
			case SCARD_E_TIMEOUT:
				level = WLOG_DEBUG;
				break;
			case SCARD_E_NO_READERS_AVAILABLE:
				level = WLOG_INFO;
				break;
			default:
				break;
		}
		WLog_Print(log, level, "%s failed with error %s [%" PRId32 "]", what,
		           SCardGetErrorString(status), status);
	}
	return status;
}

static LONG smartcard_AccessStartedEvent_Call(scard_call_context* smartcard, guac_rdp_scard_operation* op)
{
	WINPR_UNUSED(smartcard);
	WINPR_UNUSED(op);

	return SCARD_S_SUCCESS;
}

static LONG smartcard_EstablishContext_Call(guac_rdp_common_svc* svc, scard_call_context* smartcard_ctx, guac_rdp_scard_operation* op)
{
	LONG status = 0;
	EstablishContext_Call* call = &op->call.establishContext;
	status = Emulate_SCardEstablishContext(smartcard_ctx->smartcard, call->dwScope);

	if (status != SCARD_S_SUCCESS)
	{
		guac_client_log(svc->client, GUAC_LOG_ERROR, "smartcard_EstablishContext_Call failed with error %" PRId32 "!", status);
		return status;
	}

	status = smartcard_pack_establish_context_return(op->out, smartcard_ctx->smartcard);
	if (status != SCARD_S_SUCCESS)
	{
		guac_client_log(svc->client, GUAC_LOG_ERROR, "smartcard_EstablishContext_Call:smartcard_pack_establish_context_return failed with error %" PRId32 "!", status);
		return status;
	}

	return status;
}

static BOOL filter_match(wLinkedList* list, LPCSTR reader, size_t readerLen)
{
	if (readerLen < 1)
		return FALSE;

	LinkedList_Enumerator_Reset(list);

	while (LinkedList_Enumerator_MoveNext(list))
	{
		const char* filter = LinkedList_Enumerator_Current(list);

		if (filter)
		{
			if (strstr(reader, filter) != NULL)
				return TRUE;
		}
	}

	return FALSE;
}

static DWORD filter_device_by_name_a(wLinkedList* list, LPSTR* mszReaders, DWORD cchReaders)
{
	size_t rpos = 0;
	size_t wpos = 0;

	if (*mszReaders == NULL || LinkedList_Count(list) < 1)
		return cchReaders;

	do
	{
		LPCSTR rreader = &(*mszReaders)[rpos];
		LPSTR wreader = &(*mszReaders)[wpos];
		size_t readerLen = strnlen(rreader, cchReaders - rpos);

		rpos += readerLen + 1;

		if (filter_match(list, rreader, readerLen))
		{
			if (rreader != wreader)
				memmove(wreader, rreader, readerLen + 1);

			wpos += readerLen + 1;
		}
	} while (rpos < cchReaders);

	/* this string must be double 0 terminated */
	if (rpos != wpos)
	{
		if (wpos >= cchReaders)
			return 0;

		(*mszReaders)[wpos++] = '\0';
	}

	return (DWORD)wpos;
}

#define SCARD_TAG FREERDP_TAG("scard.pack")

static wLog* scard_log(void)
{
	static wLog* log = NULL;
	if (!log)
		log = WLog_Get(SCARD_TAG);
	return log;
}

static DWORD filter_device_by_name_w(wLinkedList* list, LPWSTR* mszReaders, DWORD cchReaders)
{
	wLog* log = scard_log();
	WLog_Print(log, WLOG_ERROR, "filter_device_by_name_w: start");
	DWORD rc = 0;
	LPSTR readers = NULL;

	if (LinkedList_Count(list) < 1)
		return cchReaders;

	WLog_Print(log, WLOG_ERROR, "filter_device_by_name_w: ConvertMszWCharNToUtf8Alloc");
	readers = ConvertMszWCharNToUtf8Alloc(*mszReaders, cchReaders, NULL);

	if (!readers)
	{
		free(readers);
		return 0;
	}

	free(*mszReaders);
	*mszReaders = NULL;

	WLog_Print(log, WLOG_ERROR, "filter_device_by_name_w: filter_device_by_name_a");
	rc = filter_device_by_name_a(list, &readers, cchReaders);

	WLog_Print(log, WLOG_ERROR, "filter_device_by_name_w: ConvertMszUtf8NToWCharAlloc");
	*mszReaders = ConvertMszUtf8NToWCharAlloc(readers, rc, NULL);
	if (!*mszReaders)
		rc = 0;

	free(readers);
	return rc;
}

static size_t wcslenW(const WCHAR* str)
{
    const WCHAR* s = str;
    while (s && *s)
        s++;
    return (size_t)(s - str);
}

static LONG smartcard_ListReadersW_Call(guac_rdp_common_svc* svc, scard_call_context* smartcard, guac_rdp_scard_operation* op)
{
	LONG status = 0;
	ListReaders_Return ret = { 0 };
	DWORD cchReaders = 0;
	ListReaders_Call* call = NULL;
	union
	{
		const BYTE* bp;
		const char* sz;
		const WCHAR* wz;
	} string;
	union
	{
		WCHAR** ppw;
		WCHAR* pw;
		CHAR* pc;
		BYTE* pb;
	} mszReaders;

	WINPR_ASSERT(smartcard);
	WINPR_ASSERT(op);

	call = &op->call.listReaders;

	string.bp = call->mszGroups;
	cchReaders = SCARD_AUTOALLOCATE;

	char* mszGroupsUtf = NULL;
	char* mszReadersUtf = NULL;
	size_t utf8Len = 0;

	status = Emulate_SCardListReadersW(smartcard->smartcard, string.wz, &mszReaders.pw, &cchReaders);

	mszGroupsUtf = ConvertMszWCharNToUtf8Alloc(string.wz, wcslenW(string.wz), &utf8Len);
	mszReadersUtf = ConvertMszWCharNToUtf8Alloc(mszReaders.pw, wcslenW(mszReaders.pw), &utf8Len);
	guac_client_log(svc->client, GUAC_LOG_ERROR, "RemoteSmartcard: Emulate_SCardListReadersW. string.wz: %s, &mszReaders.pw: %s, cchReaders: %lu", mszGroupsUtf, mszReadersUtf, cchReaders);

	if (status == SCARD_S_SUCCESS)
	{
		if (cchReaders == SCARD_AUTOALLOCATE) {
			guac_client_log(svc->client, GUAC_LOG_ERROR, "Emulate_SCardListReadersW: cchReaders SCARD_AUTOALLOCATE, unknown error.");
			status = SCARD_F_UNKNOWN_ERROR;
		}
	}

	if (status != SCARD_S_SUCCESS)
	{
		guac_client_log(svc->client, GUAC_LOG_ERROR, "SCardListReadersW failed with error %" PRId32 "!", status);
		return smartcard_pack_list_readers_return(op->out, &ret, TRUE);
	}

	guac_client_log(svc->client, GUAC_LOG_ERROR, "filtering by name.");
	cchReaders = filter_device_by_name_w(smartcard->names, &mszReaders.pw, cchReaders);
	ret.msz = mszReaders.pb;
	ret.cBytes = cchReaders * sizeof(WCHAR);
	ret.ReturnCode = status;

	guac_client_log(svc->client, GUAC_LOG_ERROR, "packing the return");
	status = smartcard_pack_list_readers_return(op->out, &ret, TRUE);
	guac_client_log(svc->client, GUAC_LOG_ERROR, "return is packed");

	// if (mszReaders.pb)
	// 	wrap(smartcard, SCardFreeMemory, operation->hContext, mszReaders.pb);

	if (status != SCARD_S_SUCCESS)
		return status;

	return ret.ReturnCode;
}

static LONG smartcard_GetDeviceTypeId_Call(scard_call_context* smartcard, guac_rdp_scard_operation* operation)
{
	wLog* log = scard_log();
	LONG status = 0;
	GetDeviceTypeId_Return ret = { 0 };
	GetDeviceTypeId_Call* call = NULL;

	WINPR_ASSERT(smartcard);
	WINPR_ASSERT(out);
	WINPR_ASSERT(operation);

	call = &operation->call.getDeviceTypeId;

	ret.ReturnCode = Emulate_SCardGetDeviceTypeIdW(smartcard->smartcard, call->szReaderName, &ret.dwDeviceId);
	scard_log_status_error_wlog(log, "SCardGetDeviceTypeIdW", ret.ReturnCode);

	status = smartcard_pack_device_type_id_return(operation->out, &ret);
	if (status != SCARD_S_SUCCESS)
		return status;

	return ret.ReturnCode;
}

static LONG smartcard_GetStatusChangeW_Call(scard_call_context* smartcard, guac_rdp_scard_operation* operation)
{
	wLog* log = scard_log();

	LONG status = STATUS_NO_MEMORY;
	DWORD dwTimeOut = 0;
	const DWORD dwTimeStep = 100;
	GetStatusChange_Return ret = { 0 };
	LPSCARD_READERSTATEW rgReaderStates = NULL;

	WINPR_ASSERT(smartcard);
	WINPR_ASSERT(operation);

	GetStatusChangeW_Call* call = &operation->call.getStatusChangeW;
	dwTimeOut = call->dwTimeOut;

	if (call->cReaders > 0)
	{
		ret.cReaders = call->cReaders;
		rgReaderStates = calloc(ret.cReaders, sizeof(SCARD_READERSTATEW));
		ret.rgReaderStates = (ReaderState_Return*)calloc(ret.cReaders, sizeof(ReaderState_Return));
		if (!rgReaderStates || !ret.rgReaderStates)
			goto fail;
	}

	for (UINT32 x = 0; x < MAX(1, dwTimeOut);)
	{
		if (call->cReaders > 0)
			memcpy(rgReaderStates, call->rgReaderStates,
			       call->cReaders * sizeof(SCARD_READERSTATEW));
		{
			ret.ReturnCode = Emulate_SCardGetStatusChangeW(smartcard->smartcard, MIN(dwTimeOut, dwTimeStep), rgReaderStates, call->cReaders);
		}
		if (ret.ReturnCode != SCARD_E_TIMEOUT) {
			break;
		} else {
			scard_log_status_error_wlog(log, "Invalid return code", ret.ReturnCode);
			return ret.ReturnCode;
		}
	}
	scard_log_status_error_wlog(log, "SCardGetStatusChangeW", ret.ReturnCode);

	for (UINT32 index = 0; index < ret.cReaders; index++)
	{
		const SCARD_READERSTATEW* cur = &rgReaderStates[index];
		ReaderState_Return* rout = &ret.rgReaderStates[index];

		rout->dwCurrentState = cur->dwCurrentState;
		rout->dwEventState = cur->dwEventState;
		rout->cbAtr = cur->cbAtr;
		CopyMemory(&(rout->rgbAtr), cur->rgbAtr, sizeof(rout->rgbAtr));
	}

	status = smartcard_pack_get_status_change_return(operation->out, &ret, TRUE);
fail:
	free(ret.rgReaderStates);
	free(rgReaderStates);
	if (status != SCARD_S_SUCCESS)
		return status;
	return ret.ReturnCode;
}

static LONG smartcard_ReleaseContext_Call(scard_call_context* smartcard, guac_rdp_scard_operation* operation)
{
	return 0;
}

LONG guac_rdpdr_smartcard_irp_device_control_call(guac_rdp_common_svc* svc,
												  scard_call_context* ctx,
                                                  guac_rdpdr_iorequest* request,
                                                  guac_rdp_scard_operation* op,
                                                  NTSTATUS* io_status) {
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

	/* Device Control Response */
	Stream_Write_UINT32(op->out, 0);                            /* OutputBufferLength (4 bytes) */
	Stream_Zero(op->out, SMARTCARD_COMMON_TYPE_HEADER_LENGTH);  /* CommonTypeHeader (8 bytes) */
	Stream_Zero(op->out, SMARTCARD_PRIVATE_TYPE_HEADER_LENGTH); /* PrivateTypeHeader (8 bytes) */
	Stream_Write_UINT32(op->out, 0);                            /* Result (4 bytes) */

    /* Call */
	switch (ioControlCode)
	{
		case SCARD_IOCTL_ESTABLISHCONTEXT:
			result = smartcard_EstablishContext_Call(svc, ctx, op);
			break;

		case SCARD_IOCTL_RELEASECONTEXT:
			result = smartcard_ReleaseContext_Call(ctx, op);
			break;

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

		case SCARD_IOCTL_LISTREADERSW:
			result = smartcard_ListReadersW_Call(svc, ctx, op);
			break;

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

		case SCARD_IOCTL_GETSTATUSCHANGEW:
			result = smartcard_GetStatusChangeW_Call(ctx, op);
			break;

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

		case SCARD_IOCTL_ACCESSSTARTEDEVENT:
			result = smartcard_AccessStartedEvent_Call(ctx, op);
			break;

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

		case SCARD_IOCTL_GETDEVICETYPEID:
			result = smartcard_GetDeviceTypeId_Call(ctx, op);
			break;

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
		smartcard_pack_write_size_align(op->out, Stream_GetPosition(op->out) - offset, 8);
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
