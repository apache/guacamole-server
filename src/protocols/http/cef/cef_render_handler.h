#ifndef CEF_RENDER_HANDLER_H
#define CEF_RENDER_HANDLER_H

#include "include/capi/cef_render_handler_capi.h"

/**
 * Function called when a frame's pixel data is available.
 *
 * @param self Pointer to the render handler.
 * @param browser Pointer to the browser instance.
 * @param type Paint element type.
 * @param dirtyRectsCount Number of dirty rectangles to be updated.
 * @param dirtyRects Array of dirty rectangles to be updated.
 * @param buffer Buffer containing the updated pixel data.
 * @param width Width of the updated buffer in pixels.
 * @param height Height of the updated buffer in pixels.
 * 
 * @return Zero (0) if successful.
 */
int on_paint(struct _cef_render_handler_t *self,
             struct _cef_browser_t *browser,
             cef_paint_element_type_t type,
             size_t dirtyRectsCount,
             const cef_rect_t *dirtyRects,
             const void *buffer,
             int width,
             int height);

/**
 * Function called to retrieve the view rectangle for the browser instance.
 * 
 * @param browser Pointer to the browser instance.
 * @param self Pointer to the render handler.
 * @param rect Output: Rectangle structure to store the view rectangle.
 * 
 * @return One (1) indicates the rect was provided successfully, zero (0) otherwise.
 */
int get_view_rect(struct _cef_browser_t *browser,
                  struct _cef_render_handler_t *self,
                  cef_rect_t *rect);

/**
 * Custom render handler-specific structure.
 */
typedef struct _custom_render_handler_t {
    /* Base class containing default handler functions. */
    cef_render_handler_t base;
    
    int (*get_view_rect)(struct _cef_browser_t *, struct _cef_render_handler_t *, cef_rect_t *);
    int (*on_paint)(struct _cef_render_handler_t *, struct _cef_browser_t *, cef_paint_element_type_t, size_t, const cef_rect_t *, const void *, int, int);
} custom_render_handler_t;

/**
 * Function to create a custom render handler.
 *
 * @return Pointer to an allocated custom_render_handler_t structure.
 */
custom_render_handler_t* create_render_handler();

#endif // CEF_RENDER_HANDLER_H
