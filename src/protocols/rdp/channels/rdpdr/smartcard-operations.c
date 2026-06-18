#include "channels/rdpdr/smartcard-operations.h"
#include "channels/rdpdr/scard.h"
#include "channels/rdpdr/smartcard-pack.h"

#include <guacamole/client.h>
#include "channels/rdpdr/rdpdr.h"

#include <winpr/stream.h>
#include <winpr/smartcard.h>

static void free_reader_states_a(LPSCARD_READERSTATEA rgReaderStates, UINT32 cReaders)
{
	for (UINT32 x = 0; x < cReaders; x++)
	{
		SCARD_READERSTATEA* state = &rgReaderStates[x];
		free(state->szReader);
	}

	free(rgReaderStates);
}

static void free_reader_states_w(LPSCARD_READERSTATEW rgReaderStates, UINT32 cReaders)
{
	for (UINT32 x = 0; x < cReaders; x++)
	{
		SCARD_READERSTATEW* state = &rgReaderStates[x];
		free(state->szReader);
	}

	free(rgReaderStates);
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
		{
			LocateCardsA_Call* call = &op->call.locateCardsA;
			free(call->mszCards);

			free_reader_states_a(call->rgReaderStates, call->cReaders);
		}
		break;
		case SCARD_IOCTL_LOCATECARDSW:
		{
			LocateCardsW_Call* call = &op->call.locateCardsW;
			free(call->mszCards);

			free_reader_states_w(call->rgReaderStates, call->cReaders);
		}
		break;

		case SCARD_IOCTL_LOCATECARDSBYATRA:
		{
			LocateCardsByATRA_Call* call = &op->call.locateCardsByATRA;

			free_reader_states_a(call->rgReaderStates, call->cReaders);
		}
		break;
		case SCARD_IOCTL_LOCATECARDSBYATRW:
		{
			LocateCardsByATRW_Call* call = &op->call.locateCardsByATRW;
			free_reader_states_w(call->rgReaderStates, call->cReaders);
		}
		break;
		case SCARD_IOCTL_FORGETREADERA:
		case SCARD_IOCTL_INTRODUCEREADERGROUPA:
		case SCARD_IOCTL_FORGETREADERGROUPA:
		{
			ContextAndStringA_Call* call = &op->call.contextAndStringA;
			free(call->sz);
		}
		break;

		case SCARD_IOCTL_FORGETREADERW:
		case SCARD_IOCTL_INTRODUCEREADERGROUPW:
		case SCARD_IOCTL_FORGETREADERGROUPW:
		{
			ContextAndStringW_Call* call = &op->call.contextAndStringW;
			free(call->sz);
		}
		break;

		case SCARD_IOCTL_INTRODUCEREADERA:
		case SCARD_IOCTL_REMOVEREADERFROMGROUPA:
		case SCARD_IOCTL_ADDREADERTOGROUPA:

		{
			ContextAndTwoStringA_Call* call = &op->call.contextAndTwoStringA;
			free(call->sz1);
			free(call->sz2);
		}
		break;

		case SCARD_IOCTL_INTRODUCEREADERW:
		case SCARD_IOCTL_REMOVEREADERFROMGROUPW:
		case SCARD_IOCTL_ADDREADERTOGROUPW:

		{
			ContextAndTwoStringW_Call* call = &op->call.contextAndTwoStringW;
			free(call->sz1);
			free(call->sz2);
		}
		break;

		case SCARD_IOCTL_LISTREADERSA:
		case SCARD_IOCTL_LISTREADERSW:
		{
			ListReaders_Call* call = &op->call.listReaders;
			free(call->mszGroups);
		}
		break;
		case SCARD_IOCTL_GETSTATUSCHANGEA:
		{
			GetStatusChangeA_Call* call = &op->call.getStatusChangeA;
			free_reader_states_a(call->rgReaderStates, call->cReaders);
		}
		break;

		case SCARD_IOCTL_GETSTATUSCHANGEW:
		{
			GetStatusChangeW_Call* call = &op->call.getStatusChangeW;
			free_reader_states_w(call->rgReaderStates, call->cReaders);
		}
		break;
		case SCARD_IOCTL_GETREADERICON:
		{
			GetReaderIcon_Call* call = &op->call.getReaderIcon;
			free(call->szReaderName);
		}
		break;
		case SCARD_IOCTL_GETDEVICETYPEID:
		{
			GetDeviceTypeId_Call* call = &op->call.getDeviceTypeId;
			free(call->szReaderName);
		}
		break;
		case SCARD_IOCTL_CONNECTA:
		{
			ConnectA_Call* call = &op->call.connectA;
			free(call->szReader);
		}
		break;
		case SCARD_IOCTL_CONNECTW:
		{
			ConnectW_Call* call = &op->call.connectW;
			free(call->szReader);
		}
		break;
		case SCARD_IOCTL_SETATTRIB:
			free(op->call.setAttrib.pbAttr);
			break;
		case SCARD_IOCTL_TRANSMIT:
		{
			Transmit_Call* call = &op->call.transmit;
			free(call->pbSendBuffer);
			free(call->pioSendPci);
			free(call->pioRecvPci);
		}
		break;
		case SCARD_IOCTL_CONTROL:
		{
			Control_Call* call = &op->call.control;
			free(call->pvInBuffer);
		}
		break;
		case SCARD_IOCTL_READCACHEA:
		{
			ReadCacheA_Call* call = &op->call.readCacheA;
			free(call->szLookupName);
			free(call->Common.CardIdentifier);
		}
		break;
		case SCARD_IOCTL_READCACHEW:
		{
			ReadCacheW_Call* call = &op->call.readCacheW;
			free(call->szLookupName);
			free(call->Common.CardIdentifier);
		}
		break;
		case SCARD_IOCTL_WRITECACHEA:
		{
			WriteCacheA_Call* call = &op->call.writeCacheA;
			free(call->szLookupName);
			free(call->Common.CardIdentifier);
			free(call->Common.pbData);
		}
		break;
		case SCARD_IOCTL_WRITECACHEW:
		{
			WriteCacheW_Call* call = &op->call.writeCacheW;
			free(call->szLookupName);
			free(call->Common.CardIdentifier);
			free(call->Common.pbData);
		}
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

static LONG smartcard_EstablishContext_Decode(wStream* stream, guac_rdp_scard_operation* op) {
	LONG status = 0;

	status = smartcard_unpack_establish_context_call(stream, &op->call.establishContext);
	if (status != SCARD_S_SUCCESS)
	{
        guac_client_log(op->client, GUAC_LOG_ERROR, "smartcard_EstablishContext_Decode: error.");
		return status;
	}

	return SCARD_S_SUCCESS;
}

static LONG smartcard_AccessStartedEvent_Decode(wStream* stream, guac_rdp_scard_operation* op) {
    INT32 unused = 0;

    if (Stream_GetRemainingLength(stream) < 4) {
        guac_client_log(op->client, GUAC_LOG_ERROR,
            "smartcard_AccessStartedEvent_Decode: stream too short.");
        return SCARD_F_INTERNAL_ERROR;
    }

    Stream_Read_INT32(stream, unused); // This value is unused per FreeRDP
    (void)unused;

    return SCARD_S_SUCCESS;
}

static LONG smartcard_ListReadersW_Decode(wStream* stream, guac_rdp_scard_operation* op)
{
	LONG status = 0;

	WINPR_ASSERT(stream);
	WINPR_ASSERT(op);

	status = smartcard_unpack_list_readers_call(stream, &op->call.listReaders, TRUE);

	return status;
}

static LONG smartcard_GetDeviceTypeId_Decode(wStream* s, guac_rdp_scard_operation* op)
{
	LONG status = 0;

	WINPR_ASSERT(s);
	WINPR_ASSERT(op);

	status = smartcard_unpack_get_device_type_id_call(s, &op->call.getDeviceTypeId);

	return status;
}

static LONG smartcard_GetStatusChangeW_Decode(wStream* s, guac_rdp_scard_operation* operation)
{
	WINPR_ASSERT(s);
	WINPR_ASSERT(operation);

	return smartcard_unpack_get_status_change_w_call(s, &operation->call.getStatusChangeW);
}

static LONG smartcard_ReleaseContext_Decode(wStream* s, guac_rdp_scard_operation* operation)
{
	LONG status = 0;

	WINPR_ASSERT(s);
	WINPR_ASSERT(operation);

	status = smartcard_unpack_context_call(s, &operation->call.context, "ReleaseContext");
	if (status != SCARD_S_SUCCESS) {
		guac_client_log(operation->client, GUAC_LOG_ERROR, "smartcard_ReleaseContext_Decode-unpack: invalid status.");
    }

	return status;
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

    if (Stream_Length(input_stream) != (Stream_GetPosition(input_stream) + input_len)) {
        guac_client_log(operation->client, GUAC_LOG_WARNING,
            "InputBufferLength mismatch: Actual: %" PRIuz " Expected: %" PRIuz "", Stream_Length(input_stream), Stream_GetPosition(input_stream) + input_len);
        return STATUS_INVALID_PARAMETER;
    }

    // guac_client_log(operation->client, GUAC_LOG_DEBUG, "control_decode: Smartcard IOCTL request: 0x%08X", ioctl_code);

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
            guac_client_log(operation->client, GUAC_LOG_INFO, "smartcard_EstablishContext_Decode");
            status = smartcard_EstablishContext_Decode(input_stream, operation);
            break;

        case SCARD_IOCTL_ACCESSSTARTEDEVENT:
            guac_client_log(operation->client, GUAC_LOG_INFO, "smartcard_AccessStartedEvent_Decode");
            status = smartcard_AccessStartedEvent_Decode(input_stream, operation);
            break;

        case SCARD_IOCTL_LISTREADERSW:
            guac_client_log(operation->client, GUAC_LOG_INFO, "smartcard_ListReadersW_Decode");
            status = smartcard_ListReadersW_Decode(input_stream, operation);
            break;

        case SCARD_IOCTL_GETDEVICETYPEID:
			status = smartcard_GetDeviceTypeId_Decode(input_stream, operation);
			break;

        case SCARD_IOCTL_GETSTATUSCHANGEW:
			status = smartcard_GetStatusChangeW_Decode(input_stream, operation);
			break;

        case SCARD_IOCTL_RELEASECONTEXT:
			status = smartcard_ReleaseContext_Decode(input_stream, operation);
			break;

        default:
            guac_client_log(operation->client, GUAC_LOG_WARNING,
                "Smartcard IOCTL: Unsupported code 0x%08X, %s", ioctl_code, scard_get_ioctl_string(operation->ioControlCode, true));
            status = STATUS_NOT_IMPLEMENTED;
            break;
    }

    return status;
}
