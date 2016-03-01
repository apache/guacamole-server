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

#ifndef GUAC_VNC_DISPLAY_H
#define GUAC_VNC_DISPLAY_H

#include "config.h"

#include <rfb/rfbclient.h>
#include <rfb/rfbproto.h>

/**
 * Called for normal binary VNC image data.
 */
void guac_vnc_update(rfbClient* client, int x, int y, int w, int h);

/**
 * Called for updates which contain data from existing rectangles of the
 * display.
 */
void guac_vnc_copyrect(rfbClient* client, int src_x, int src_y, int w, int h, int dest_x, int dest_y);

/**
 * Called when the pixel format of future updates is changing.
 */
void guac_vnc_set_pixel_format(rfbClient* client, int color_depth);

/**
 * Called when the display is being resized (or initially allocated).
 */
rfbBool guac_vnc_malloc_framebuffer(rfbClient* rfb_client);

#endif

