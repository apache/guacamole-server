// smartcard-pcsc.h
/*
Wraps smartcard functions. Should be the only file that imports #include <winscard.h>.
This is because FreeRDP mocks lots of windows smartcard functions, and
redefines many types. Causes significant build errors if included elsewhere.
*/

#include <stdio.h>
#include <stdint.h>

#ifndef GUAC_SMARTCARD_PCSC_H
#define GUAC_SMARTCARD_PCSC_H

/**
 * Minimal types from PC/SC. Avoid including <winscard.h>
 * to prevent conflicts with FreeRDP's winpr/wtypes.h.
 */
typedef int32_t LONG;
typedef uint32_t DWORD;
typedef uint32_t UINT32;
typedef uintptr_t SCARDCONTEXT;
typedef uintptr_t SCARDHANDLE;

#define SCARD_AUTOALLOCATE UINT32_MAX

#define SCARD_SCOPE_USER 0
#define SCARD_SCOPE_TERMINAL 1
#define SCARD_SCOPE_SYSTEM 2

#define SCARD_STATE_UNAWARE 0x00000000
#define SCARD_STATE_IGNORE 0x00000001
#define SCARD_STATE_CHANGED 0x00000002
#define SCARD_STATE_UNKNOWN 0x00000004
#define SCARD_STATE_UNAVAILABLE 0x00000008
#define SCARD_STATE_EMPTY 0x00000010
#define SCARD_STATE_PRESENT 0x00000020
#define SCARD_STATE_ATRMATCH 0x00000040
#define SCARD_STATE_EXCLUSIVE 0x00000080
#define SCARD_STATE_INUSE 0x00000100
#define SCARD_STATE_MUTE 0x00000200
#define SCARD_STATE_UNPOWERED 0x00000400

#define SCARD_SHARE_EXCLUSIVE 1
#define SCARD_SHARE_SHARED 2
#define SCARD_SHARE_DIRECT 3

#define SCARD_LEAVE_CARD 0
#define SCARD_RESET_CARD 1
#define SCARD_UNPOWER_CARD 2
#define SCARD_EJECT_CARD 3

#define SCARD_S_SUCCESS       0x00000000
#define SCARD_E_TIMEOUT       0x8010000A
#define SCARD_PROTOCOL_T0      0x0001
#define SCARD_PROTOCOL_T1      0x0002

// Declare needed PC/SC functions
long SCardEstablishContext(uint32_t dwScope, const void* pvReserved1,
                           const void* pvReserved2, SCARDCONTEXT* phContext);

long SCardReleaseContext(SCARDCONTEXT hContext);

long SCardConnect(SCARDCONTEXT hContext, const char* szReader,
                  uint32_t dwShareMode, uint32_t dwPreferredProtocols,
                  SCARDHANDLE* phCard, uint32_t* pdwActiveProtocol);

long SCardDisconnect(SCARDHANDLE hCard, uint32_t dwDisposition);

long SCardGetStatusChange(SCARDCONTEXT hContext, uint32_t dwTimeout,
                          void* rgReaderStates, uint32_t cReaders);

// Simplified context struct to hold PC/SC stuff
typedef struct guac_smartcard_context {
    SCARDCONTEXT hContext;
    SCARDHANDLE hCard;
    DWORD dwActiveProtocol;
    int hStatusChangeEvent;   // PC/SC event handle
    uintptr_t wrappedEventHandle;   // Wrapped handle returned to Windows client
} guac_smartcard_context;

int guac_smartcard_init(guac_smartcard_context* ctx, const char* smartcard_name);

#endif
