/*
 * Copyright (C) 2014 Glyptodon LLC
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

#ifndef _GUAC_VNC_CLIPBOARD_H
#define _GUAC_VNC_CLIPBOARD_H

#include "config.h"

#include "guac_clipboard.h"

#include <guacamole/client.h>
#include <guacamole/stream.h>
#include <guacamole/user.h>
#include <rfb/rfbclient.h>
#include <rfb/rfbproto.h>

/**
 * Sets the encoding of clipboard data exchanged with the VNC server to the
 * encoding having the given name. If the name is NULL, or is invalid, the
 * standard ISO8859-1 encoding will be used.
 *
 * @param client
 *     The client to set the clipboard encoding of.
 *
 * @param name
 *     The name of the encoding to use for all clipboard data. Valid values
 *     are: "ISO8859-1", "UTF-8", "UTF-16", "CP1252", or NULL.
 *
 * @return
 *     Zero if the chosen encoding is standard for VNC, or non-zero if the VNC
 *     standard is being violated.
 */
int guac_vnc_set_clipboard_encoding(guac_client* client,
        const char* name);

/**
 * Handler for inbound clipboard data from Guacamole users.
 */
int guac_vnc_clipboard_handler(guac_user* user, guac_stream* stream, char* mimetype);

/**
 * Handler for stream data related to clipboard.
 */
int guac_vnc_clipboard_blob_handler(guac_user* user, guac_stream* stream, void* data, int length);

/**
 * Handler for end-of-stream related to clipboard.
 */
int guac_vnc_clipboard_end_handler(guac_user* user, guac_stream* stream);

/**
 * Handler for clipboard data received via VNC.
 */
void guac_vnc_cut_text(rfbClient* client, const char* text, int textlen);

#endif

