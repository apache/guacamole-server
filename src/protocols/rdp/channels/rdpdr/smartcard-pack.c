#include "channels/rdpdr/smartcard-pack.h"
#include "channels/rdpdr/rdpdr.h"
#include "channels/rdpdr/scard.h"
#include "channels/rdpdr/msz-unicode.h"

#include <guacamole/client.h>
#include <winpr/stream.h>

#include <winpr/smartcard.h>

#define SCARD_TAG FREERDP_TAG("scard.pack")

static const DWORD g_LogLevel = WLOG_ERROR;

typedef enum
{
	NDR_PTR_FULL,
	NDR_PTR_SIMPLE,
	NDR_PTR_FIXED
} ndr_ptr_t;

static wLog* scard_log(void)
{
	static wLog* log = NULL;
	if (!log)
		log = WLog_Get(SCARD_TAG);
	return log;
}

static BOOL Stream_CheckAndLogRequiredLengthWLog(void* log, wStream* s, int size)
{
	if (!s)
	{
		WLog_Print((wLog*)log, WLOG_ERROR, "Stream is NULL");
		return FALSE;
	}

	if (Stream_GetRemainingLength(s) < (size_t)size)
	{
		WLog_Print((wLog*)log, WLOG_ERROR,
		           "Stream too short: needed %d bytes, but only %zu available",
		           size, Stream_GetRemainingLength(s));
		return FALSE;
	}

	return TRUE;
}

static BOOL Stream_CheckAndLogRequiredLengthOfSizeWLog(wLog* log, wStream* s, size_t len, size_t elementSize) {
	return TRUE;
}

#define smartcard_unpack_redir_scard_context(log, s, context, index, ndr)                  \
	smartcard_unpack_redir_scard_context_((log), (s), (context), (index), (ndr), __FILE__, \
	                                      __func__, __LINE__)

static LONG smartcard_unpack_redir_scard_context_(wLog* log, wStream* s,
                                                  REDIR_SCARDCONTEXT* context, UINT32* index,
                                                  UINT32* ppbContextNdrPtr, const char* file,
                                                  const char* function, size_t line);

#define smartcard_context_supported(log, size) \
	smartcard_context_supported_((log), (size), __FILE__, __func__, __LINE__)
static LONG smartcard_context_supported_(wLog* log, uint32_t size, const char* file,
                                         const char* fkt, size_t line)
{
	switch (size)
	{
		case 0:
		case 4:
		case 8:
			return SCARD_S_SUCCESS;
		default:
		{
			const uint32_t level = WLOG_WARN;
			if (WLog_IsLevelActive(log, level))
			{
				WLog_PrintMessage(log, WLOG_MESSAGE_TEXT, level, line, file, fkt,
				                  "REDIR_SCARDCONTEXT length is not 0, 4 or 8: %" PRIu32 "", size);
			}
			return STATUS_INVALID_PARAMETER;
		}
	}
}

/* Reads a NDR pointer and checks if the value read has the expected relative
 * addressing */
#define smartcard_ndr_pointer_read(log, s, index, ptr) \
	smartcard_ndr_pointer_read_((log), (s), (index), (ptr), __FILE__, __func__, __LINE__)
static BOOL smartcard_ndr_pointer_read_(wLog* log, wStream* s, UINT32* index, UINT32* ptr,
                                        const char* file, const char* fkt, size_t line)
{
	const UINT32 expect = 0x20000 + (*index) * 4;
	UINT32 ndrPtr = 0;
	WINPR_UNUSED(file);
	if (!s)
		return FALSE;
	if (!Stream_CheckAndLogRequiredLengthWLog(log, s, 4))
		return FALSE;

	Stream_Read_UINT32(s, ndrPtr); /* mszGroupsNdrPtr (4 bytes) */

	if (ptr)
		*ptr = ndrPtr;
	if (expect != ndrPtr)
	{
		/* Allow NULL pointer if we read the result */
		if (ptr && (ndrPtr == 0))
			return TRUE;
		WLog_Print(log, WLOG_WARN,
		           "[%s:%" PRIuz "] Read context pointer 0x%08" PRIx32 ", expected 0x%08" PRIx32,
		           fkt, line, ndrPtr, expect);
		return FALSE;
	}

	(*index) = (*index) + 1;
	return TRUE;
}

static BOOL smartcard_ndr_pointer_write(wStream* s, UINT32* index, DWORD length)
{
	const UINT32 ndrPtr = 0x20000 + (*index) * 4;

	if (!s)
		return FALSE;
	if (!Stream_EnsureRemainingCapacity(s, 4))
		return FALSE;

	if (length > 0)
	{
		Stream_Write_UINT32(s, ndrPtr); /* mszGroupsNdrPtr (4 bytes) */
		(*index) = (*index) + 1;
	}
	else
		Stream_Write_UINT32(s, 0);
	return TRUE;
}

static LONG smartcard_ndr_write(wStream* s, const BYTE* data, UINT32 size, UINT32 elementSize,
                                ndr_ptr_t type)
{
	const UINT32 offset = 0;
	const UINT32 len = size;
	const UINT32 dataLen = size * elementSize;
	size_t required = 0;

	if (size == 0)
		return SCARD_S_SUCCESS;

	switch (type)
	{
		case NDR_PTR_FULL:
			required = 12;
			break;
		case NDR_PTR_SIMPLE:
			required = 4;
			break;
		case NDR_PTR_FIXED:
			required = 0;
			break;
		default:
			return SCARD_E_INVALID_PARAMETER;
	}

	if (!Stream_EnsureRemainingCapacity(s, required + dataLen + 4))
		return STATUS_BUFFER_TOO_SMALL;

	switch (type)
	{
		case NDR_PTR_FULL:
			Stream_Write_UINT32(s, len);
			Stream_Write_UINT32(s, offset);
			Stream_Write_UINT32(s, len);
			break;
		case NDR_PTR_SIMPLE:
			Stream_Write_UINT32(s, len);
			break;
		case NDR_PTR_FIXED:
			break;
		default:
			return SCARD_E_INVALID_PARAMETER;
	}

	if (data)
		Stream_Write(s, data, dataLen);
	else
		Stream_Zero(s, dataLen);
	return smartcard_pack_write_size_align(s, len, 4);
}

static LONG smartcard_ndr_read(wLog* log, wStream* s, BYTE** data, size_t min, size_t elementSize,
                               ndr_ptr_t type)
{
	size_t len = 0;
	size_t offset = 0;
	size_t len2 = 0;
	void* r = NULL;
	size_t required = 0;

	*data = NULL;
	switch (type)
	{
		case NDR_PTR_FULL:
			required = 12;
			break;
		case NDR_PTR_SIMPLE:
			required = 4;
			break;
		case NDR_PTR_FIXED:
			required = min;
			break;
		default:
			return STATUS_INVALID_PARAMETER;
	}

	if (!Stream_CheckAndLogRequiredLengthWLog(log, s, required))
		return STATUS_BUFFER_TOO_SMALL;

	switch (type)
	{
		case NDR_PTR_FULL:
			Stream_Read_UINT32(s, len);
			Stream_Read_UINT32(s, offset);
			Stream_Read_UINT32(s, len2);
			if (len != offset + len2)
			{
				WLog_Print(log, WLOG_ERROR,
				           "Invalid data when reading full NDR pointer: total=%" PRIu32
				           ", offset=%" PRIu32 ", remaining=%" PRIu32,
				           len, offset, len2);
				return STATUS_BUFFER_TOO_SMALL;
			}
			break;
		case NDR_PTR_SIMPLE:
			Stream_Read_UINT32(s, len);

			if ((len != min) && (min > 0))
			{
				WLog_Print(log, WLOG_ERROR,
				           "Invalid data when reading simple NDR pointer: total=%" PRIu32
				           ", expected=%" PRIu32,
				           len, min);
				return STATUS_BUFFER_TOO_SMALL;
			}
			break;
		case NDR_PTR_FIXED:
			len = (UINT32)min;
			break;
		default:
			return STATUS_INVALID_PARAMETER;
	}

	if (min > len)
	{
		WLog_Print(log, WLOG_ERROR,
		           "Invalid length read from NDR pointer, minimum %" PRIu32 ", got %" PRIu32, min,
		           len);
		return STATUS_DATA_ERROR;
	}

	if (len > SIZE_MAX / 2)
		return STATUS_BUFFER_TOO_SMALL;

	if (!Stream_CheckAndLogRequiredLengthOfSizeWLog(log, s, len, elementSize))
		return STATUS_BUFFER_TOO_SMALL;

	len *= elementSize;

	/* Ensure proper '\0' termination for all kinds of unicode strings
	 * as we do not know if the data from the wire contains one.
	 */
	r = calloc(len + sizeof(WCHAR), sizeof(CHAR));
	if (!r)
		return SCARD_E_NO_MEMORY;
	Stream_Read(s, r, len);
	smartcard_unpack_read_size_align(s, len, 4);
	*data = r;
	return STATUS_SUCCESS;
}

static LONG smartcard_ndr_read_w(wLog* log, wStream* s, WCHAR** data, ndr_ptr_t type)
{
	union
	{
		WCHAR** ppc;
		BYTE** ppv;
	} u;
	u.ppc = data;
	return smartcard_ndr_read(log, s, u.ppv, 0, sizeof(WCHAR), type);
}

static char* smartcard_convert_string_list(const void* in, size_t bytes, BOOL unicode)
{
	size_t length = 0;
	union
	{
		const void* pv;
		const char* sz;
		const WCHAR* wz;
	} string;
	char* mszA = NULL;

	string.pv = in;

	if (bytes < 1)
		return NULL;

	if (in == NULL)
		return NULL;

	if (unicode)
	{
		mszA = ConvertMszWCharNToUtf8Alloc(string.wz, bytes / sizeof(WCHAR), &length);
		if (!mszA)
			return NULL;
	}
	else
	{
		mszA = (char*)calloc(bytes, sizeof(char));
		if (!mszA)
			return NULL;
		CopyMemory(mszA, string.sz, bytes - 1);
		length = bytes;
	}

	if (length < 1)
	{
		free(mszA);
		return NULL;
	}
	for (size_t index = 0; index < length - 1; index++)
	{
		if (mszA[index] == '\0')
			mszA[index] = ',';
	}

	return mszA;
}

static void smartcard_trace_list_readers_call(wLog* log, const ListReaders_Call* call, BOOL unicode)
{
	WINPR_ASSERT(call);
	return;

	// if (!WLog_IsLevelActive(log, g_LogLevel))
	// 	return;

	// char* mszGroupsA = smartcard_convert_string_list(call->mszGroups, call->cBytes, unicode);

	// WLog_Print(log, g_LogLevel, "ListReaders%s_Call {", unicode ? "W" : "A");
	// smartcard_log_context(log, &call->handles.hContext);

	// WLog_Print(log, g_LogLevel,
	//            "cBytes: %" PRIu32 " mszGroups: %s fmszReadersIsNULL: %" PRId32
	//            " cchReaders: 0x%08" PRIX32 "",
	//            call->cBytes, mszGroupsA, call->fmszReadersIsNULL, call->cchReaders);
	// WLog_Print(log, g_LogLevel, "}");

	// free(mszGroupsA);
}

static void smartcard_trace_list_readers_return(wLog* log, const ListReaders_Return* ret,
                                                BOOL unicode)
{
	WINPR_ASSERT(ret);

	WLog_Print(log, g_LogLevel, "ListReaders%s_Return {", unicode ? "W" : "A");
	// WLog_Print(log, g_LogLevel, "  ReturnCode: %s (0x%08" PRIX32 ")",
	//            SCardGetErrorString(ret->ReturnCode), ret->ReturnCode);

	if (ret->ReturnCode != SCARD_S_SUCCESS)
	{
		WLog_Print(log, g_LogLevel, "}");
		return;
	}

	WLog_Print(log, g_LogLevel, "Converting string...");
	char* mszA = smartcard_convert_string_list(ret->msz, ret->cBytes, unicode);
	WLog_Print(log, g_LogLevel, "Converted!");

	WLog_Print(log, g_LogLevel, "  cBytes: %" PRIu32 " msz: %s", ret->cBytes, mszA);
	WLog_Print(log, g_LogLevel, "}");
	free(mszA);
}

static void smartcard_trace_context_and_string_call_w(wLog* log, const char* name,
                                                      const REDIR_SCARDCONTEXT* phContext,
                                                      const WCHAR* sz)
{
	char tmp[1024] = { 0 };

	// if (!WLog_IsLevelActive(log, g_LogLevel))
	// 	return;

	if (sz)
		(void)ConvertWCharToUtf8(sz, tmp, ARRAYSIZE(tmp));

	WLog_Print(log, WLOG_WARN, "%s {", name);
	// smartcard_log_context(log, phContext);
	WLog_Print(log, WLOG_WARN, "  sz=%s", tmp);
	WLog_Print(log, WLOG_WARN, "}");
}

static void smartcard_trace_device_type_id_return(wLog* log, const GetDeviceTypeId_Return* ret)
{
	WINPR_ASSERT(ret);

	// if (!WLog_IsLevelActive(log, g_LogLevel))
	// 	return;

	WLog_Print(log, WLOG_WARN, "GetDeviceTypeId_Return {");
	WLog_Print(log, WLOG_WARN, "  ReturnCode: %s (0x%08" PRIX32 ")",
	           SCardGetErrorString(ret->ReturnCode), ret->ReturnCode);
	WLog_Print(log, WLOG_WARN, "  dwDeviceId=%08" PRIx32, ret->dwDeviceId);

	WLog_Print(log, WLOG_WARN, "}");
}

static LONG smartcard_unpack_common_context_and_string_w(wLog* log, wStream* s,
                                                         REDIR_SCARDCONTEXT* phContext,
                                                         WCHAR** pszReaderName)
{
	UINT32 index = 0;
	UINT32 pbContextNdrPtr = 0;

	LONG status = smartcard_unpack_redir_scard_context(log, s, phContext, &index, &pbContextNdrPtr);
	if (status != SCARD_S_SUCCESS)
		return status;

	if (!smartcard_ndr_pointer_read(log, s, &index, NULL))
		return ERROR_INVALID_DATA;

	status = smartcard_unpack_redir_scard_context_ref(log, s, pbContextNdrPtr, phContext);
	if (status != SCARD_S_SUCCESS)
		return status;

	status = smartcard_ndr_read_w(log, s, pszReaderName, NDR_PTR_FULL);
	if (status != SCARD_S_SUCCESS)
		return status;

	smartcard_trace_context_and_string_call_w(log, __func__, phContext, *pszReaderName);
	return SCARD_S_SUCCESS;
}

static LONG smartcard_unpack_reader_state_w(wLog* log, wStream* s, LPSCARD_READERSTATEW* ppcReaders,
                                            UINT32 cReaders, UINT32* ptrIndex)
{
	LONG status = SCARD_E_NO_MEMORY;

	WINPR_ASSERT(ppcReaders || (cReaders == 0));
	if (!Stream_CheckAndLogRequiredLengthWLog(log, s, 4))
		return status;

	UINT32 len;
	Stream_Read_UINT32(s, len);
	if (len != cReaders)
	{
		WLog_Print(log, WLOG_ERROR, "Count mismatch when reading LPSCARD_READERSTATEW");
		return status;
	}

	LPSCARD_READERSTATEW rgReaderStates =
	    (LPSCARD_READERSTATEW)calloc(cReaders, sizeof(SCARD_READERSTATEW));
	BOOL* states = calloc(cReaders, sizeof(BOOL));

	if (!rgReaderStates || !states)
		goto fail;

	status = ERROR_INVALID_DATA;
	for (UINT32 index = 0; index < cReaders; index++)
	{
		UINT32 ptr = UINT32_MAX;
		LPSCARD_READERSTATEW readerState = &rgReaderStates[index];

		if (!Stream_CheckAndLogRequiredLengthWLog(log, s, 52))
			goto fail;

		if (!smartcard_ndr_pointer_read(log, s, ptrIndex, &ptr))
		{
			if (ptr != 0)
				goto fail;
		}
		/* Ignore NULL length strings */
		states[index] = ptr != 0;
		Stream_Read_UINT32(s, readerState->dwCurrentState); /* dwCurrentState (4 bytes) */
		Stream_Read_UINT32(s, readerState->dwEventState);   /* dwEventState (4 bytes) */
		Stream_Read_UINT32(s, readerState->cbAtr);          /* cbAtr (4 bytes) */
		Stream_Read(s, readerState->rgbAtr, 36);            /* rgbAtr [0..36] (36 bytes) */
	}

	for (UINT32 index = 0; index < cReaders; index++)
	{
		LPSCARD_READERSTATEW readerState = &rgReaderStates[index];

		/* Skip NULL pointers */
		if (!states[index])
			continue;

		status = smartcard_ndr_read_w(log, s, &readerState->szReader, NDR_PTR_FULL);
		if (status != SCARD_S_SUCCESS)
			goto fail;
	}

	*ppcReaders = rgReaderStates;
	free(states);
	return SCARD_S_SUCCESS;
fail:
	if (rgReaderStates)
	{
		for (UINT32 index = 0; index < cReaders; index++)
		{
			LPSCARD_READERSTATEW readerState = &rgReaderStates[index];
			free(readerState->szReader);
		}
	}
	free(rgReaderStates);
	free(states);
	return status;
}

static LONG smartcard_ndr_write_state(wStream* s, const ReaderState_Return* data, UINT32 size,
                                      ndr_ptr_t type)
{
	union
	{
		const ReaderState_Return* reader;
		const BYTE* data;
	} cnv;

	WINPR_ASSERT(data || (size == 0));
	cnv.reader = data;
	return smartcard_ndr_write(s, cnv.data, size, sizeof(ReaderState_Return), type);
}

// static void smartcard_trace_get_status_change_return(wLog* log, const GetStatusChange_Return* ret,
//                                                      BOOL unicode)
// {
// 	WINPR_ASSERT(ret);

// 	WLog_Print(log, g_LogLevel, "GetStatusChange%s_Return {", unicode ? "W" : "A");
// 	WLog_Print(log, g_LogLevel, "  ReturnCode: %s (0x%08" PRIX32 ")",
// 	           SCardGetErrorString(ret->ReturnCode), ret->ReturnCode);
// 	WLog_Print(log, g_LogLevel, "  cReaders: %" PRIu32 "", ret->cReaders);

// 	dump_reader_states_return(log, ret->rgReaderStates, ret->cReaders);

// 	if (!ret->rgReaderStates && (ret->cReaders > 0))
// 	{
// 		WLog_Print(log, g_LogLevel, "    [INVALID STATE] rgReaderStates=NULL, cReaders=%" PRIu32,
// 		           ret->cReaders);
// 	}
// 	else if (ret->ReturnCode != SCARD_S_SUCCESS)
// 	{
// 		WLog_Print(log, g_LogLevel, "    [INVALID RETURN] rgReaderStates, cReaders=%" PRIu32,
// 		           ret->cReaders);
// 	}
// 	else
// 	{
// 		for (UINT32 index = 0; index < ret->cReaders; index++)
// 		{
// 			char buffer[1024] = { 0 };
// 			const ReaderState_Return* rgReaderState = &(ret->rgReaderStates[index]);
// 			char* szCurrentState = SCardGetReaderStateString(rgReaderState->dwCurrentState);
// 			char* szEventState = SCardGetReaderStateString(rgReaderState->dwEventState);
// 			WLog_Print(log, g_LogLevel, "    [%" PRIu32 "]: dwCurrentState: %s (0x%08" PRIX32 ")",
// 			           index, szCurrentState, rgReaderState->dwCurrentState);
// 			WLog_Print(log, g_LogLevel, "    [%" PRIu32 "]: dwEventState: %s (0x%08" PRIX32 ")",
// 			           index, szEventState, rgReaderState->dwEventState);
// 			WLog_Print(log, g_LogLevel, "    [%" PRIu32 "]: cbAtr: %" PRIu32 " rgbAtr: %s", index,
// 			           rgReaderState->cbAtr,
// 			           smartcard_array_dump(rgReaderState->rgbAtr, rgReaderState->cbAtr, buffer,
// 			                                sizeof(buffer)));
// 			free(szCurrentState);
// 			free(szEventState);
// 		}
// 	}

// 	WLog_Print(log, g_LogLevel, "}");
// }

LONG smartcard_unpack_read_size_align(wStream* s, size_t size, UINT32 alignment)
{
	size_t pad = 0;

	pad = size;
	size = (size + alignment - 1) & ~(alignment - 1);
	pad = size - pad;

	if (pad)
		Stream_Seek(s, pad);

	return (LONG)pad;
}

LONG smartcard_unpack_redir_scard_context_(wLog* log, wStream* s, REDIR_SCARDCONTEXT* context,
                                           UINT32* index, UINT32* ppbContextNdrPtr,
                                           const char* file, const char* function, size_t line)
{
	UINT32 pbContextNdrPtr = 0;

	// WLog_Print(log, WLOG_WARN, "smartcard_unpack_redir_scard_context_: start");

	WINPR_UNUSED(file);
	WINPR_ASSERT(context);

	ZeroMemory(context, sizeof(REDIR_SCARDCONTEXT));

	const LONG status = smartcard_context_supported_(log, context->cbContext, file, function, line);
	if (status != SCARD_S_SUCCESS) {
		WLog_Print(log, WLOG_ERROR, "smartcard context not supported.");
		return status;
	}

	Stream_Read_UINT32(s, context->cbContext); /* cbContext (4 bytes) */
	// size_t position = Stream_GetPosition(s);
    // size_t length = Stream_GetRemainingLength(s);
    // size_t remaining = Stream_GetRemainingLength(s);
	// WLog_Print(log, WLOG_WARN, "smartcard_unpack_redir_scard_context_: context: %lu Position: %zu, Length: %zu, Remaining: %zu", context->cbContext, position, length, remaining);

	if (!smartcard_ndr_pointer_read_(log, s, index, &pbContextNdrPtr, file, function, line)) {
		WLog_Print(log, WLOG_ERROR, "smartcard_ndr_pointer_read_: invalid data.");
		return ERROR_INVALID_DATA;
	}

	if (((context->cbContext == 0) && pbContextNdrPtr) ||
	    ((context->cbContext != 0) && !pbContextNdrPtr))
	{
		WLog_Print(log, WLOG_WARN,
		           "REDIR_SCARDCONTEXT cbContext (%" PRIu32 ") pbContextNdrPtr (%" PRIu32
		           ") inconsistency",
		           context->cbContext, pbContextNdrPtr);
		return STATUS_INVALID_PARAMETER;
	}

	*ppbContextNdrPtr = pbContextNdrPtr;

	// WLog_Print(log, WLOG_WARN, "smartcard_unpack_redir_scard_context_: end");
	return SCARD_S_SUCCESS;
}

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

LONG guac_rdpdr_scard_unpack_private_type_header(wStream* s, guac_client* client) {
	wLog* log = scard_log();
	UINT32 filler = 0;
	UINT32 objectBufferLength = 0;

	if (!Stream_CheckAndLogRequiredLengthWLog(log, s, 8))
		return STATUS_BUFFER_TOO_SMALL;

	Stream_Read_UINT32(s, objectBufferLength); /* ObjectBufferLength (4 bytes) */
	Stream_Read_UINT32(s, filler);             /* Filler (4 bytes), should be 0x00000000 */

	if (filler != 0x00000000)
	{
		WLog_Print(log, WLOG_WARN, "Unexpected PrivateTypeHeader Filler 0x%08" PRIX32 "", filler);
		return STATUS_INVALID_PARAMETER;
	}

	if (!Stream_CheckAndLogRequiredLengthWLog(log, s, objectBufferLength))
		return STATUS_INVALID_PARAMETER;

	return SCARD_S_SUCCESS;
}


LONG smartcard_pack_write_size_align( wStream* s, size_t size, UINT32 alignment)
{
	size_t pad = 0;

	pad = size;
	size = (size + alignment - 1) & ~(alignment - 1);
	pad = size - pad;

	if (pad)
	{
		if (!Stream_EnsureRemainingCapacity(s, pad))
		{
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

LONG smartcard_unpack_establish_context_call(wStream* s, EstablishContext_Call* call)
{
	wLog* log = scard_log();
	size_t position = Stream_GetPosition(s);
    size_t length = Stream_GetRemainingLength(s);
    size_t remaining = Stream_GetRemainingLength(s);
	WLog_Print(log, WLOG_WARN, "smartcard_unpack_establish_context_call: Position: %zu, Length: %zu, Remaining: %zu", position, length, remaining);

	if (Stream_GetRemainingLength(s) < 4)
		return STATUS_BUFFER_TOO_SMALL;

	Stream_Read_UINT32(s, call->dwScope); /* dwScope (4 bytes) */
	return SCARD_S_SUCCESS;
}

LONG smartcard_pack_redir_scard_context(wStream* s, RemoteSmartcard* smartcard, DWORD* index)
{
	wLog* log = scard_log();

	const UINT32 pbContextNdrPtr = 0x00020000 + *index * 4;

	WINPR_ASSERT(smartcard->context);

	if (smartcard->context->cbContext != 0)
	{
		Stream_Write_UINT32(s, smartcard->context->cbContext); /* cbContext (4 bytes) */
		WLog_Print(log, WLOG_WARN, "packed cbContext: %lu", smartcard->context->cbContext);
		Stream_Write_UINT32(s, pbContextNdrPtr);    /* pbContextNdrPtr (4 bytes) */
		WLog_Print(log, WLOG_WARN, "packed pbContextNdrPtr: %zu", pbContextNdrPtr);
		*index = *index + 1;
	}
	else {
		Stream_Zero(s, 8);
	}

	return SCARD_S_SUCCESS;
}

LONG smartcard_pack_redir_scard_context_ref(wStream* s, RemoteSmartcard* smartcard)
{
	wLog* log = scard_log();
	WINPR_ASSERT(smartcard->context);
	WLog_Print(log, WLOG_WARN, "smartcard_pack_redir_scard_context_ref: packed cbContext: %lu", smartcard->context->cbContext);
	Stream_Write_UINT32(s, smartcard->context->cbContext); /* Length (4 bytes) */

	if (smartcard->context->cbContext)
	{
		WLog_Print(log, WLOG_WARN, "smartcard_pack_redir_scard_context_ref: writing pbContext");
		Stream_Write(s, &(smartcard->context->pbContext), smartcard->context->cbContext);
	}

	return SCARD_S_SUCCESS;
}

LONG smartcard_unpack_redir_scard_context_ref(wLog* log, wStream* s, UINT32 pbContextNdrPtr, REDIR_SCARDCONTEXT* context)
{
	UINT32 length = 0;

	WINPR_ASSERT(context);
	if (context->cbContext == 0)
		return SCARD_S_SUCCESS;

	if (!Stream_CheckAndLogRequiredLengthWLog(log, s, 4))
		return STATUS_BUFFER_TOO_SMALL;

	Stream_Read_UINT32(s, length); /* Length (4 bytes) */

	if (length != context->cbContext)
	{
		WLog_Print(log, WLOG_WARN,
		           "REDIR_SCARDCONTEXT length (%" PRIu32 ") cbContext (%" PRIu32 ") mismatch",
		           length, context->cbContext);
		return STATUS_INVALID_PARAMETER;
	}

	if (!Stream_CheckAndLogRequiredLengthWLog(log, s, context->cbContext))
		return STATUS_BUFFER_TOO_SMALL;

	if (context->cbContext)
		Stream_Read(s, &(context->pbContext), context->cbContext);
	else
		ZeroMemory(&(context->pbContext), sizeof(context->pbContext));

	return SCARD_S_SUCCESS;
}

LONG smartcard_pack_establish_context_return(wStream* s, RemoteSmartcard* smartcard)
{
	LONG status = 0;
	DWORD index = 0;

	wLog* log = scard_log();

	status = smartcard_pack_redir_scard_context(s, smartcard, &index);
	if (status != SCARD_S_SUCCESS) {
		WLog_Print(log, WLOG_ERROR, "smartcard_pack_redir_scard_context: failed to pack context!");
		return status;
	}

	return smartcard_pack_redir_scard_context_ref(s, smartcard);
}

LONG smartcard_unpack_list_readers_call(wStream* s, ListReaders_Call* call, BOOL unicode)
{
	UINT32 index = 0;
	UINT32 mszGroupsNdrPtr = 0;
	UINT32 pbContextNdrPtr = 0;
	wLog* log = scard_log();

	WINPR_ASSERT(call);
	call->mszGroups = NULL;

	LONG status = smartcard_unpack_redir_scard_context(log, s, &(call->handles.hContext), &index,
	                                                   &pbContextNdrPtr);

	if (status != SCARD_S_SUCCESS) {
		WLog_Print(log, WLOG_ERROR, "smartcard_unpack_redir_scard_context failed!");
		return status;
	}

	if (!Stream_CheckAndLogRequiredLengthWLog(log, s, 4))
		return STATUS_BUFFER_TOO_SMALL;

	Stream_Read_UINT32(s, call->cBytes); /* cBytes (4 bytes) */
	if (!smartcard_ndr_pointer_read(log, s, &index, &mszGroupsNdrPtr)) {
		WLog_Print(log, WLOG_ERROR, "smartcard_ndr_pointer_read failed!");
		return ERROR_INVALID_DATA;
	}

	if (!Stream_CheckAndLogRequiredLengthWLog(log, s, 8))
		return STATUS_BUFFER_TOO_SMALL;
	Stream_Read_INT32(s, call->fmszReadersIsNULL); /* fmszReadersIsNULL (4 bytes) */
	Stream_Read_UINT32(s, call->cchReaders);       /* cchReaders (4 bytes) */

	status = smartcard_unpack_redir_scard_context_ref(log, s, pbContextNdrPtr,
	                                                  &(call->handles.hContext));
	if (status != SCARD_S_SUCCESS) {
		WLog_Print(log, WLOG_ERROR, "smartcard_unpack_redir_scard_context_ref failed!");
		return status;
	}

	if (mszGroupsNdrPtr)
	{
		status = smartcard_ndr_read(log, s, &call->mszGroups, call->cBytes, 1, NDR_PTR_SIMPLE);
		if (status != SCARD_S_SUCCESS) {
			WLog_Print(log, WLOG_ERROR, "smartcard_ndr_read failed!");
			return status;
		}
	}

	smartcard_trace_list_readers_call(log, call, unicode);
	return SCARD_S_SUCCESS;
}

LONG smartcard_pack_list_readers_return(wStream* s, const ListReaders_Return* ret, BOOL unicode)
{
	WINPR_ASSERT(ret);
	wLog* log = scard_log();
	LONG status = 0;
	UINT32 index = 0;
	UINT32 size = ret->cBytes;

	smartcard_trace_list_readers_return(log, ret, unicode);
	if (ret->ReturnCode != SCARD_S_SUCCESS) {
		WLog_Print(log, WLOG_ERROR, "smartcard_trace_list_readers_return returned non 0");
		size = 0;
	}

	if (!Stream_EnsureRemainingCapacity(s, 4))
	{
		WLog_Print(log, WLOG_ERROR, "Stream_EnsureRemainingCapacity failed!");
		return SCARD_F_INTERNAL_ERROR;
	}

	WLog_Print(log, WLOG_ERROR, "smartcard_pack_list_readers_return: Size: %d", size);

	Stream_Write_UINT32(s, size); /* cBytes (4 bytes) */
	if (!smartcard_ndr_pointer_write(s, &index, size))
		return SCARD_E_NO_MEMORY;

	if (ret->msz) {
		WLog_Print(log, WLOG_ERROR, "smartcard_pack_list_readers_return:MSZ has data :(");
	}
	else {
		WLog_Print(log, WLOG_ERROR, "smartcard_pack_list_readers_return:MSZ has no data :)");
	}

	status = smartcard_ndr_write(s, ret->msz, size, 1, NDR_PTR_SIMPLE);
	if (status != SCARD_S_SUCCESS)
		return status;
	return ret->ReturnCode;
}

LONG smartcard_unpack_get_device_type_id_call(wStream* s, GetDeviceTypeId_Call* call)
{
	WINPR_ASSERT(call);
	wLog* log = scard_log();
	return smartcard_unpack_common_context_and_string_w(log, s, &call->handles.hContext,
	                                                    &call->szReaderName);
}

LONG smartcard_pack_device_type_id_return(wStream* s, const GetDeviceTypeId_Return* ret)
{
	WINPR_ASSERT(ret);
	wLog* log = scard_log();
	smartcard_trace_device_type_id_return(log, ret);

	if (!Stream_EnsureRemainingCapacity(s, 4))
	{
		WLog_Print(log, WLOG_ERROR, "Stream_EnsureRemainingCapacity failed!");
		return SCARD_F_INTERNAL_ERROR;
	}

	Stream_Write_UINT32(s, ret->dwDeviceId); /* cBytes (4 bytes) */

	return ret->ReturnCode;
}

LONG smartcard_unpack_get_status_change_w_call(wStream* s, GetStatusChangeW_Call* call)
{
	UINT32 ndrPtr = 0;
	UINT32 index = 0;
	UINT32 pbContextNdrPtr = 0;

	WINPR_ASSERT(call);
	wLog* log = scard_log();
	call->rgReaderStates = NULL;

	LONG status = smartcard_unpack_redir_scard_context(log, s, &(call->handles.hContext), &index,
	                                                   &pbContextNdrPtr);
	if (status != SCARD_S_SUCCESS)
		return status;

	if (!Stream_CheckAndLogRequiredLengthWLog(log, s, 8))
		return STATUS_BUFFER_TOO_SMALL;

	Stream_Read_UINT32(s, call->dwTimeOut); /* dwTimeOut (4 bytes) */
	Stream_Read_UINT32(s, call->cReaders);  /* cReaders (4 bytes) */
	if (!smartcard_ndr_pointer_read(log, s, &index, &ndrPtr))
		return ERROR_INVALID_DATA;

	status = smartcard_unpack_redir_scard_context_ref(log, s, pbContextNdrPtr,
	                                                  &(call->handles.hContext));
	if (status != SCARD_S_SUCCESS)
		return status;

	if (ndrPtr)
	{
		status =
		    smartcard_unpack_reader_state_w(log, s, &call->rgReaderStates, call->cReaders, &index);
		if (status != SCARD_S_SUCCESS)
			return status;
	}
	else
	{
		WLog_Print(log, WLOG_WARN, "ndrPtr=0x%08" PRIx32 ", can not read rgReaderStates", ndrPtr);
		return SCARD_E_UNEXPECTED;
	}

	// smartcard_trace_get_status_change_w_call(log, call);
	return SCARD_S_SUCCESS;
}

LONG smartcard_pack_get_status_change_return(wStream* s, const GetStatusChange_Return* ret,
                                             BOOL unicode)
{
	WINPR_ASSERT(ret);

	LONG status = 0;
	DWORD cReaders = ret->cReaders;
	UINT32 index = 0;

	// smartcard_trace_get_status_change_return(log, ret, unicode);
	if (ret->ReturnCode != SCARD_S_SUCCESS)
		cReaders = 0;
	if (cReaders == SCARD_AUTOALLOCATE)
		cReaders = 0;

	if (!Stream_EnsureRemainingCapacity(s, 4))
		return SCARD_E_NO_MEMORY;

	Stream_Write_UINT32(s, cReaders); /* cReaders (4 bytes) */
	if (!smartcard_ndr_pointer_write(s, &index, cReaders))
		return SCARD_E_NO_MEMORY;
	status = smartcard_ndr_write_state(s, ret->rgReaderStates, cReaders, NDR_PTR_SIMPLE);
	if (status != SCARD_S_SUCCESS)
		return status;
	return ret->ReturnCode;
}

LONG smartcard_unpack_context_call(wStream* s, Context_Call* call, const char* name)
{
	UINT32 index = 0;
	UINT32 pbContextNdrPtr = 0;
	wLog* log = scard_log();

	WINPR_ASSERT(call);
	LONG status = smartcard_unpack_redir_scard_context(log, s, &(call->handles.hContext), &index,
	                                                   &pbContextNdrPtr);
	if (status != SCARD_S_SUCCESS)
		return status;

	status = smartcard_unpack_redir_scard_context_ref(log, s, pbContextNdrPtr,
	                                                  &(call->handles.hContext));
	if (status != SCARD_S_SUCCESS)
		WLog_Print(log, WLOG_ERROR,
		           "smartcard_unpack_redir_scard_context_ref failed with error %" PRId32 "",
		           status);

	// smartcard_trace_context_call(log, call, name);
	return status;
}
