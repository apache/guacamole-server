
/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "config.h"
#include "guac_image_text.h"
#include "guac_pixmap.h"

#include <xorg-server.h>
#include <xf86.h>
#include <fb.h>

void guac_drv_imagetext8(DrawablePtr drawable, GCPtr gc, int x, int y,
        int count, char* chars) {
    /* STUB */
    xf86Msg(X_INFO, "guac: STUB: %s layer=%i\n", __func__,
        guac_drv_get_drawable(drawable)->layer->layer->index);
    miImageText8(drawable, gc, x, y, count, chars);
}

void guac_drv_imagetext16(DrawablePtr drawable, GCPtr gc, int x, int y,
        int count, unsigned short* chars) {
    /* STUB */
    xf86Msg(X_INFO, "guac: STUB: %s layer=%i\n", __func__,
        guac_drv_get_drawable(drawable)->layer->layer->index);
    miImageText16(drawable, gc, x, y, count, chars);
}

void guac_drv_imageglyphblt(DrawablePtr drawable, GCPtr gc, int x, int y,
        unsigned int nglyph, CharInfoPtr* char_info, pointer glyph_base) {
    /* STUB */
    xf86Msg(X_INFO, "guac: STUB: %s layer=%i\n", __func__,
        guac_drv_get_drawable(drawable)->layer->layer->index);
    fbImageGlyphBlt(drawable, gc, x, y, nglyph, char_info, glyph_base);
}

