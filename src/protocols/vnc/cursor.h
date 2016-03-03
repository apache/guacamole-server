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

#ifndef GUAC_VNC_CURSOR_H
#define GUAC_VNC_CURSOR_H

#include "config.h"

#include <rfb/rfbclient.h>
#include <rfb/rfbproto.h>

/**
 * Callback invoked by libVNCServer when it receives a new cursor image from
 * the VNC server. The cursor image itself will be split across
 * client->rcSource and client->rcMask, where rcSource is an image buffer of
 * the format natively used by the current VNC connection, and rcMask is an
 * array if bitmasks. Each bit within rcMask corresponds to a pixel within
 * rcSource, where a 0 denotes full transparency and a 1 denotes full opacity.
 *
 * @param client
 *     The VNC client associated with the VNC session in which the new cursor
 *     image was received.
 *
 * @param x
 *     The X coordinate of the new cursor image's hotspot, in pixels.
 *
 * @param y
 *     The Y coordinate of the new cursor image's hotspot, in pixels.
 *
 * @param w
 *     The width of the cursor image, in pixels.
 *
 * @param h
 *     The height of the cursor image, in pixels.
 *
 * @param bpp
 *     The number of bytes in each pixel, which must be either 4, 2, or 1.
 */
void guac_vnc_cursor(rfbClient* client, int x, int y, int w, int h, int bpp);

#endif

