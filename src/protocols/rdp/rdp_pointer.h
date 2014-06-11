/*
 * Copyright (C) 2013 Glyptodon LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */


#ifndef _GUAC_RDP_RDP_POINTER_H
#define _GUAC_RDP_RDP_POINTER_H

#include "config.h"

#include <freerdp/freerdp.h>
#include <guacamole/layer.h>

typedef struct guac_rdp_pointer {

    /**
     * FreeRDP pointer data - MUST GO FIRST.
     */
    rdpPointer pointer;

    /**
     * Guacamole layer containing cached image data.
     */
    guac_layer* layer;

} guac_rdp_pointer;

void guac_rdp_pointer_new(rdpContext* context, rdpPointer* pointer);
void guac_rdp_pointer_set(rdpContext* context, rdpPointer* pointer);
void guac_rdp_pointer_free(rdpContext* context, rdpPointer* pointer);
void guac_rdp_pointer_set_null(rdpContext* context);
void guac_rdp_pointer_set_default(rdpContext* context);

#endif
