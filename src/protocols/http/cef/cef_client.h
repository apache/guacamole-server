#ifndef CEF_CLIENT_H
#define CEF_CLIENT_H

#include "include/capi/cef_client_capi.h"

#include "cef_life_span_handler.h"
#include "cef_render_handler.h"

/**
 * Custom client-specific structure.
 */
typedef struct _custom_client_t {
    /* Base class containing default handler functions. */
    cef_client_t base;                            
    
    /* Custom render_handler_t field to store our custom render handler. */
    custom_render_handler_t *render_handler;      
    
    custom_life_span_handler_t *life_span_handler;

} custom_client_t;

/**
 * Function to create a custom client instance.
 * 
 * @param render_handler Pointer to an allocated custom_render_handler_t structure.
 * 
 * @return Pointer to the custom client instance.
 */
cef_client_t *create_client(custom_render_handler_t *render_handler) ;

#endif // CEF_CLIENT_H
