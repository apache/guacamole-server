// rdrp-smartcard.h

#ifndef GUAC_RDP_CHANNELS_RDPDR_SMARTCARD_H
#define GUAC_RDP_CHANNELS_RDPDR_SMARTCARD_H

#include "channels/common-svc.h"
#include "channels/rdpdr/rdpdr.h"

#include <winpr/stream.h>

#define RDP_SCARD_CTL_CODE(code) \
	CTL_CODE(FILE_DEVICE_FILE_SYSTEM, (code), METHOD_BUFFERED, FILE_ANY_ACCESS)

#define SCARD_IOCTL_ESTABLISHCONTEXT RDP_SCARD_CTL_CODE(5)       /* SCardEstablishContext */
#define SCARD_IOCTL_RELEASECONTEXT RDP_SCARD_CTL_CODE(6)         /* SCardReleaseContext */
#define SCARD_IOCTL_ISVALIDCONTEXT RDP_SCARD_CTL_CODE(7)         /* SCardIsValidContext */
#define SCARD_IOCTL_LISTREADERGROUPSA RDP_SCARD_CTL_CODE(8)      /* SCardListReaderGroupsA */
#define SCARD_IOCTL_LISTREADERGROUPSW RDP_SCARD_CTL_CODE(9)      /* SCardListReaderGroupsW */
#define SCARD_IOCTL_LISTREADERSA RDP_SCARD_CTL_CODE(10)          /* SCardListReadersA */
#define SCARD_IOCTL_LISTREADERSW RDP_SCARD_CTL_CODE(11)          /* SCardListReadersW */
#define SCARD_IOCTL_INTRODUCEREADERGROUPA RDP_SCARD_CTL_CODE(20) /* SCardIntroduceReaderGroupA */
#define SCARD_IOCTL_INTRODUCEREADERGROUPW RDP_SCARD_CTL_CODE(21) /* SCardIntroduceReaderGroupW */
#define SCARD_IOCTL_FORGETREADERGROUPA RDP_SCARD_CTL_CODE(22)    /* SCardForgetReaderGroupA */
#define SCARD_IOCTL_FORGETREADERGROUPW RDP_SCARD_CTL_CODE(23)    /* SCardForgetReaderGroupW */
#define SCARD_IOCTL_INTRODUCEREADERA RDP_SCARD_CTL_CODE(24)      /* SCardIntroduceReaderA */
#define SCARD_IOCTL_INTRODUCEREADERW RDP_SCARD_CTL_CODE(25)      /* SCardIntroduceReaderW */
#define SCARD_IOCTL_FORGETREADERA RDP_SCARD_CTL_CODE(26)         /* SCardForgetReaderA */
#define SCARD_IOCTL_FORGETREADERW RDP_SCARD_CTL_CODE(27)         /* SCardForgetReaderW */
#define SCARD_IOCTL_ADDREADERTOGROUPA RDP_SCARD_CTL_CODE(28)     /* SCardAddReaderToGroupA */
#define SCARD_IOCTL_ADDREADERTOGROUPW RDP_SCARD_CTL_CODE(29)     /* SCardAddReaderToGroupW */
#define SCARD_IOCTL_REMOVEREADERFROMGROUPA                \
	RDP_SCARD_CTL_CODE(30) /* SCardRemoveReaderFromGroupA \
	                        */
#define SCARD_IOCTL_REMOVEREADERFROMGROUPW                                                   \
	RDP_SCARD_CTL_CODE(31)                                    /* SCardRemoveReaderFromGroupW \
	                                                           */
#define SCARD_IOCTL_LOCATECARDSA RDP_SCARD_CTL_CODE(38)       /* SCardLocateCardsA */
#define SCARD_IOCTL_LOCATECARDSW RDP_SCARD_CTL_CODE(39)       /* SCardLocateCardsW */
#define SCARD_IOCTL_GETSTATUSCHANGEA RDP_SCARD_CTL_CODE(40)   /* SCardGetStatusChangeA */
#define SCARD_IOCTL_GETSTATUSCHANGEW RDP_SCARD_CTL_CODE(41)   /* SCardGetStatusChangeW */
#define SCARD_IOCTL_CANCEL RDP_SCARD_CTL_CODE(42)             /* SCardCancel */
#define SCARD_IOCTL_CONNECTA RDP_SCARD_CTL_CODE(43)           /* SCardConnectA */
#define SCARD_IOCTL_CONNECTW RDP_SCARD_CTL_CODE(44)           /* SCardConnectW */
#define SCARD_IOCTL_RECONNECT RDP_SCARD_CTL_CODE(45)          /* SCardReconnect */
#define SCARD_IOCTL_DISCONNECT RDP_SCARD_CTL_CODE(46)         /* SCardDisconnect */
#define SCARD_IOCTL_BEGINTRANSACTION RDP_SCARD_CTL_CODE(47)   /* SCardBeginTransaction */
#define SCARD_IOCTL_ENDTRANSACTION RDP_SCARD_CTL_CODE(48)     /* SCardEndTransaction */
#define SCARD_IOCTL_STATE RDP_SCARD_CTL_CODE(49)              /* SCardState */
#define SCARD_IOCTL_STATUSA RDP_SCARD_CTL_CODE(50)            /* SCardStatusA */
#define SCARD_IOCTL_STATUSW RDP_SCARD_CTL_CODE(51)            /* SCardStatusW */
#define SCARD_IOCTL_TRANSMIT RDP_SCARD_CTL_CODE(52)           /* SCardTransmit */
#define SCARD_IOCTL_CONTROL RDP_SCARD_CTL_CODE(53)            /* SCardControl */
#define SCARD_IOCTL_GETATTRIB RDP_SCARD_CTL_CODE(54)          /* SCardGetAttrib */
#define SCARD_IOCTL_SETATTRIB RDP_SCARD_CTL_CODE(55)          /* SCardSetAttrib */
#define SCARD_IOCTL_ACCESSSTARTEDEVENT RDP_SCARD_CTL_CODE(56) /* SCardAccessStartedEvent */
#define SCARD_IOCTL_RELEASETARTEDEVENT RDP_SCARD_CTL_CODE(57) /* SCardReleaseStartedEvent */
#define SCARD_IOCTL_LOCATECARDSBYATRA RDP_SCARD_CTL_CODE(58)  /* SCardLocateCardsByATRA */
#define SCARD_IOCTL_LOCATECARDSBYATRW RDP_SCARD_CTL_CODE(59)  /* SCardLocateCardsByATRW */
#define SCARD_IOCTL_READCACHEA RDP_SCARD_CTL_CODE(60)         /* SCardReadCacheA */
#define SCARD_IOCTL_READCACHEW RDP_SCARD_CTL_CODE(61)         /* SCardReadCacheW */
#define SCARD_IOCTL_WRITECACHEA RDP_SCARD_CTL_CODE(62)        /* SCardWriteCacheA */
#define SCARD_IOCTL_WRITECACHEW RDP_SCARD_CTL_CODE(63)        /* SCardWriteCacheW */
#define SCARD_IOCTL_GETTRANSMITCOUNT RDP_SCARD_CTL_CODE(64)   /* SCardGetTransmitCount */
#define SCARD_IOCTL_GETREADERICON RDP_SCARD_CTL_CODE(65)      /* SCardGetReaderIconA */
#define SCARD_IOCTL_GETDEVICETYPEID RDP_SCARD_CTL_CODE(66)    /* SCardGetDeviceTypeIdA */
#define SCARD_IOCTL_ISVALID                 0x313624

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

#endif

