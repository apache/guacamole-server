// smartcard-pcsc.c

#include "channels/rdpdr/smartcard-pcsc.h"
#include <stdio.h>
#include <stdint.h>

int guac_smartcard_init(guac_smartcard_context* ctx, const char* smartcard_name) {
    LONG ret = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &ctx->hContext);
    if (ret != SCARD_S_SUCCESS) {
        fprintf(stderr, "SCardEstablishContext failed: 0x%X\n", (unsigned int) ret);
        return -1;
    }

    // Connect to VICC reader with shared mode, any protocol
    ret = SCardConnect(ctx->hContext, smartcard_name,
                       SCARD_SHARE_SHARED,
                       SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1,
                       &ctx->hCard,
                       &ctx->dwActiveProtocol);
    if (ret != SCARD_S_SUCCESS) {
        fprintf(stderr, "SCardConnect failed: 0x%X\n", (unsigned int) ret);
        SCardReleaseContext(ctx->hContext);
        return -1;
    }

    // Obtain the PC/SC status change event handle
    ret = SCardGetStatusChange(ctx->hContext, 0, NULL, 0);
    if (ret != SCARD_S_SUCCESS && ret != SCARD_E_TIMEOUT) {
        fprintf(stderr, "SCardGetStatusChange(0) failed: 0x%X\n", (unsigned int) ret);
        SCardDisconnect(ctx->hCard, SCARD_LEAVE_CARD);
        SCardReleaseContext(ctx->hContext);
        return -1;
    }

    ctx->wrappedEventHandle = (uintptr_t)ctx + 0x1000;

    printf("wrappedEventHandle=%p\n", (void*) (uintptr_t) ctx->wrappedEventHandle);
    return 0;
}
