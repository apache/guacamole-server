// smartcard.h

#include "channels/rdpdr/scard.h"

#include <guacamole/client.h>

#include <stdio.h>
#include <stdint.h>
#include <winpr/stream.h>

#ifndef GUAC_SMARTCARD_H
#define GUAC_SMARTCARD_H
/*
Provides a layer of abstraction over the smart card. Every IOCTL call made over the
device channel should eventually make its way here. Some of the calls will be emulated
and cached, while others will send data to the guacamole server.
*/

typedef struct smartcard_emulation_context
{
	// wHashTable* handles;
	BOOL configured;
	const char* pem;
	const char* key;
	const char* pin;
	REDIR_SCARDCONTEXT* context;

	guac_client* log_client;
} RemoteSmartcard;

void Emulate_SCardAccessStartedEvent(RemoteSmartcard* smartcard);

LONG Emulate_SCardEstablishContext(RemoteSmartcard* smartcard, DWORD dwScope);

LONG Emulate_SCardListReadersW(RemoteSmartcard* smartcard,
	                                                  LPCWSTR mszGroups,
	                                                  LPWSTR* mszReaders, LPDWORD pcchReaders);

LONG Emulate_SCardGetDeviceTypeIdW(RemoteSmartcard* smartcard, LPCWSTR szReaderName, LPDWORD pdwDeviceTypeId);

LONG Emulate_SCardGetStatusChangeW(RemoteSmartcard* smartcard, DWORD dwTimeout, LPSCARD_READERSTATEW rgReaderStates, DWORD cReaders);

void RemoteSmartcard_free(RemoteSmartcard* smartcard);

#endif
