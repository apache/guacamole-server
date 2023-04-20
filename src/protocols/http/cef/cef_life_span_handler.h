#ifndef CEF_LIFE_SPAN_HANDLER_H
#define CEF_LIFE_SPAN_HANDLER_H

#include "include/capi/cef_client_capi.h"
#include "include/capi/cef_life_span_handler_capi.h"

/**
 * Custom life span handler-specific structure.
 */
typedef struct _custom_life_span_handler_t {
    /* Base class containing default handler functions. */
    cef_life_span_handler_t base;
} custom_life_span_handler_t;

/**
 * Function to create a custom life span handler.
 *
 * @return Pointer to an allocated custom_life_span_handler_t structure.
 */
custom_life_span_handler_t* create_life_span_handler();

#endif // CEF_LIFE_SPAN_HANDLER_H
