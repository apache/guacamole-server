// rdrp-smartcard.h

#ifndef GUAC_RDP_CHANNELS_RDPDR_SMARTCARD_H
#define GUAC_RDP_CHANNELS_RDPDR_SMARTCARD_H

#include "channels/common-svc.h"
#include "channels/rdpdr/rdpdr.h"
#include "channels/rdpdr/scard.h"

#include <guacamole/client.h>

#include <winpr/stream.h>

#define RDP_SCARD_CTL_CODE(code) \
	CTL_CODE(FILE_DEVICE_FILE_SYSTEM, (code), METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct guac_rdp_scard_operation {
    guac_client* client;
    UINT32 ioControlCode;
    UINT32 outputBufferLength;
	wStream* out;

	union
	{
		Handles_Call handles;
		Long_Call lng;
		Context_Call context;
		ContextAndStringA_Call contextAndStringA;
		ContextAndStringW_Call contextAndStringW;
		ContextAndTwoStringA_Call contextAndTwoStringA;
		ContextAndTwoStringW_Call contextAndTwoStringW;
		EstablishContext_Call establishContext;
		ListReaderGroups_Call listReaderGroups;
		ListReaders_Call listReaders;
		GetStatusChangeA_Call getStatusChangeA;
		LocateCardsA_Call locateCardsA;
		LocateCardsW_Call locateCardsW;
		LocateCards_ATRMask locateCardsATRMask;
		LocateCardsByATRA_Call locateCardsByATRA;
		LocateCardsByATRW_Call locateCardsByATRW;
		GetStatusChangeW_Call getStatusChangeW;
		GetReaderIcon_Call getReaderIcon;
		GetDeviceTypeId_Call getDeviceTypeId;
		Connect_Common_Call connect;
		ConnectA_Call connectA;
		ConnectW_Call connectW;
		Reconnect_Call reconnect;
		HCardAndDisposition_Call hCardAndDisposition;
		State_Call state;
		Status_Call status;
		SCardIO_Request scardIO;
		Transmit_Call transmit;
		GetTransmitCount_Call getTransmitCount;
		Control_Call control;
		GetAttrib_Call getAttrib;
		SetAttrib_Call setAttrib;
		ReadCache_Common readCache;
		ReadCacheA_Call readCacheA;
		ReadCacheW_Call readCacheW;
		WriteCache_Common writeCache;
		WriteCacheA_Call writeCacheA;
		WriteCacheW_Call writeCacheW;
	} call;
} guac_rdp_scard_operation;

typedef struct _SMARTCARD_CONTEXT {
    struct s_scard_call_context* call_context;
} SMARTCARD_CONTEXT;

// struct guac_rdpdr_thread_arg {
//     guac_rdp_common_svc* svc;
// 	guac_rdpdr_device* device;
//     guac_rdpdr_iorequest* iorequest;
//     guac_rdp_scard_operation op;
// 	wStream* input_stream;
// };

/**
 * Name of the printer driver that should be used on the server.
 */
#define GUAC_SMARTCARD_DRIVER "S\0m\0a\0r\0t\0 \0C\0a\0r\0d\0\0\0"

/**
 * The size of GUAC_SMARTCARD_DRIVER in bytes.
 */
#define GUAC_SMARTCARD_DRIVER_LENGTH 22

/**
 * Registers a new smartcard device within the RDPDR plugin. This must be done
 * before RDPDR connection finishes.
 *
 * @param svc
 *     The static virtual channel instance being used for RDPDR.
 *
 * @param smartcard_name
 *     The name of the smartcard that will be registered with the RDP
 *     connection and passed through to the server.
 */
void guac_rdpdr_register_smartcard(guac_rdp_common_svc* svc, char* smartcard_name);

/**
 * Handler for RDPDR Device I/O Requests which processes received messages on
 * behalf of a printer device, in this case a simulated printer which produces
 * PDF output.
 */
guac_rdpdr_device_iorequest_handler guac_rdpdr_device_smartcard_iorequest_handler;

/**
 * Free handler which frees all data specific to the simulated printer device.
 */
guac_rdpdr_device_free_handler guac_rdpdr_device_smartcard_free_handler;

// bool guac_rdpdr_start_irp_thread(guac_rdp_common_svc* svc, guac_rdpdr_device* device, const guac_rdpdr_iorequest* request, wStream* input_stream);

BOOL rdpdr_write_iocompletion_header(wStream* out, UINT32 DeviceId, UINT32 CompletionId, NTSTATUS ioStatus);

int assign_smartcard_context_to_device(guac_rdpdr_device* device, guac_client* client);

#endif

