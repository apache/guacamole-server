// remote-smartcard.c

#include "channels/rdpdr/remote-smartcard.h"
#include "channels/rdpdr/msz-unicode.h"
#include "channels/rdpdr/scard.h"
#include "channels/rdpdr/rdpdr.h"

#include <guacamole/client.h>

#include <stdio.h>
#include <stdint.h>
#include <winpr/crt.h>
#include <winpr/stream.h>
#include <winpr/smartcard.h>


#define RCARD_TAG FREERDP_TAG("remote.smartcard")

static CHAR g_ReaderNameA[] = { 'F', 'r', 'e', 'e', 'R', 'D', 'P', ' ',  'E',
	                            'm', 'u', 'l', 'a', 't', 'o', 'r', '\0', '\0' };
static INIT_ONCE g_ReaderNameWGuard = INIT_ONCE_STATIC_INIT;
static WCHAR g_ReaderNameW[32] = { 0 };
static size_t g_ReaderNameWLen = 0;

static wLog* scard_log(void)
{
	static wLog* log = NULL;
	if (!log)
		log = WLog_Get(RCARD_TAG);
	return log;
}

void RemoteSmartcard_free(RemoteSmartcard* smartcard) {
	free(smartcard->context);
}

// static size_t wcslenW(const WCHAR* str)
// {
//     const WCHAR* s = str;
//     while (s && *s)
//         s++;
//     return (size_t)(s - str);
// }

static BOOL CALLBACK g_ReaderNameWInit(PINIT_ONCE InitOnce, PVOID Parameter, PVOID* Context)
{
    WINPR_UNUSED(InitOnce);
    WINPR_UNUSED(Parameter);
    WINPR_UNUSED(Context);

    InitializeConstWCharFromUtf8(g_ReaderNameA, g_ReaderNameW, ARRAYSIZE(g_ReaderNameW));
    g_ReaderNameWLen = _wcsnlen(g_ReaderNameW, ARRAYSIZE(g_ReaderNameW) - 2) + 2;
    return TRUE;
}

void Emulate_SCardAccessStartedEvent(RemoteSmartcard* smartcard) {
	guac_client_log(smartcard->log_client, GUAC_LOG_INFO, "RemoteSmartcard: Emulate_SCardAccessStartedEvent.");
	return;
}

LONG Emulate_SCardEstablishContext(RemoteSmartcard* smartcard, DWORD dwScope) {
	wLog* log = scard_log();
	WLog_Print(log, WLOG_INFO, "RemoteSmartcard: Emulate_SCardEstablishContext. Scope: %u", dwScope);

	if (smartcard->context != NULL) {
		guac_client_log(smartcard->log_client, GUAC_LOG_INFO, "RemoteSmartcard: Emulate_SCardEstablishContext. Context already established.");
		return 0;
	}

	REDIR_SCARDCONTEXT* context = (REDIR_SCARDCONTEXT*)calloc(1, sizeof(REDIR_SCARDCONTEXT));
    if (!context) {
        return SCARD_F_UNKNOWN_ERROR;
    }

    context->cbContext = 8;

    // Fill first 4 bytes with a fake handle, remaining 4 bytes as zeros
    UINT32 fake_handle = 0x00000004;
    memcpy(&context->pbContext[0], &fake_handle, sizeof(UINT32));
    memset(&context->pbContext[4], 0, 4); // Pad the remaining bytes

	smartcard->context = context;
	return 0;
}

LONG Emulate_SCardListReadersW(RemoteSmartcard* smartcard, LPCWSTR mszGroups, LPWSTR* mszReaders, LPDWORD pcchReaders) {
	wLog* log = scard_log();
	WLog_Print(log, WLOG_INFO, "RemoteSmartcard: Emulate_SCardListReadersW");

	// A reader is a device that accepts smartcards - like a USB device.
	// The client adverstises all available readers, and the server may eventually call
	// SCardConnect("Reader A", ...) to access the smartcard in that reader.;
	InitOnceExecuteOnce(&g_ReaderNameWGuard, g_ReaderNameWInit, NULL, NULL);
	const DWORD required_len = (DWORD)g_ReaderNameWLen;

    if (!pcchReaders) {
		WLog_Print(log, WLOG_ERROR, "RemoteSmartcard: Emulate_SCardListReadersW - !pcchReaders");
        return SCARD_E_INVALID_PARAMETER;
	}

	const WCHAR expected[] = {
		'S','C','a','r','d','$','A','l','l','R','e','a','d','e','r','s','\0'
	};
	const size_t len = sizeof(expected); // Includes null terminator

    // Only support SCard$AllReaders
	if (!mszGroups || memcmp(mszGroups, expected, len) != 0) {
		WLog_Print(log, WLOG_ERROR, "RemoteSmartcard: Emulate_SCardListReadersW - query is not SCard$AllReaders");
		return SCARD_E_INVALID_PARAMETER;
	}

    if (!mszReaders)
    {
        *pcchReaders = required_len;
        return SCARD_S_SUCCESS;
    }

    if (*pcchReaders < required_len)
    {
        *pcchReaders = required_len;
		WLog_Print(log, WLOG_ERROR, "RemoteSmartcard: Emulate_SCardListReadersW - SCARD_E_INSUFFICIENT_BUFFER");
        return SCARD_E_INSUFFICIENT_BUFFER;
    }

	// WLog_Print(log, WLOG_ERROR, "RemoteSmartcard: Emulate_SCardListReadersW - allocating...");
	WCHAR* out = (WCHAR*)calloc(required_len, sizeof(WCHAR));

	//WLog_Print(log, WLOG_ERROR, "RemoteSmartcard: Emulate_SCardListReadersW - allocated! copying...");
	memcpy(out, g_ReaderNameW, required_len * sizeof(WCHAR));
	*mszReaders = out;

	//memcpy(mszReaders, g_ReaderNameW, required_len * sizeof(WCHAR));
    *pcchReaders = required_len;
	// WLog_Print(log, WLOG_INFO, "RemoteSmartcard: Emulate_SCardListReadersW. required_len: %lu", required_len);

	// char* mszGroupsUtf = NULL;
	// char* mszReadersUtf = NULL;
	// size_t utf8Len = 0;

	// WLog_Print(log, WLOG_ERROR, "RemoteSmartcard: Emulate_SCardListReadersW - converting groups...");
	// mszGroupsUtf = ConvertMszWCharNToUtf8Alloc(mszGroups, wcslenW(mszGroups), &utf8Len);
	// WLog_Print(log, WLOG_ERROR, "RemoteSmartcard: Emulate_SCardListReadersW - converting readers...");
	// mszReadersUtf = ConvertMszWCharNToUtf8Alloc(*mszReaders, wcslenW(*mszReaders), &utf8Len);

	// WLog_Print(log, WLOG_INFO, "RemoteSmartcard: Emulate_SCardListReadersW. mszGroups: %s, mszReaders: %s, pcchReaders: %lu", mszGroupsUtf, mszReadersUtf, *pcchReaders);
    return 0;
}

LONG Emulate_SCardGetDeviceTypeIdW(RemoteSmartcard* smartcard, LPCWSTR szReaderName, LPDWORD pdwDeviceTypeId) {
	*pdwDeviceTypeId = SCARD_READER_TYPE_USB;

	// TODO: validate the reader name is correct, match the reader name to a device type

	return 0;
}

LONG Emulate_SCardGetStatusChangeW(RemoteSmartcard* smartcard, DWORD dwTimeout,
                                   LPSCARD_READERSTATEW rgReaderStates, DWORD cReaders)
{
	wLog* log = scard_log();
    WLog_Print(log, WLOG_ERROR, "SCardGetStatusChangeW (emulated) called");

    for (DWORD i = 0; i < cReaders; i++)
    {
        LPSCARD_READERSTATEW readerState = &rgReaderStates[i];

        // Set all readers to STATE_PRESENT regardless of their actual state
        readerState->dwEventState = SCARD_STATE_PRESENT;

        // Optionally copy cbAtr and rgbAtr if needed; zero them otherwise
        readerState->cbAtr = 0;
        memset(readerState->rgbAtr, 0, sizeof(readerState->rgbAtr));
    }

    return SCARD_S_SUCCESS;
}
