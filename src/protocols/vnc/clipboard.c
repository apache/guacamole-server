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

#include "config.h"
#include "client.h"
#include "clipboard.h"
#include "guac_clipboard.h"
#include "guac_iconv.h"

#include <guacamole/client.h>
#include <guacamole/stream.h>
#include <rfb/rfbclient.h>

int guac_vnc_clipboard_handler(guac_client* client, guac_stream* stream,
        char* mimetype) {

    /* Clear clipboard and prepare for new data */
    vnc_guac_client_data* client_data = (vnc_guac_client_data*) client->data;
    guac_common_clipboard_reset(client_data->clipboard, mimetype);

    /* Set handlers for clipboard stream */
    stream->blob_handler = guac_vnc_clipboard_blob_handler;
    stream->end_handler = guac_vnc_clipboard_end_handler;

    return 0;
}

int guac_vnc_clipboard_blob_handler(guac_client* client, guac_stream* stream,
        void* data, int length) {

    /* Append new data */
    vnc_guac_client_data* client_data = (vnc_guac_client_data*) client->data;
    guac_common_clipboard_append(client_data->clipboard, (char*) data, length);

    return 0;
}

int guac_vnc_clipboard_end_handler(guac_client* client, guac_stream* stream) {

    vnc_guac_client_data* client_data = (vnc_guac_client_data*) client->data;
    rfbClient* rfb_client = client_data->rfb_client;

    char output_data[GUAC_VNC_CLIPBOARD_MAX_LENGTH];

    const char* input = client_data->clipboard->buffer;
    char* output = output_data;
    guac_iconv_write* writer = client_data->clipboard_writer;

    /* Convert clipboard contents */
    guac_iconv(GUAC_READ_UTF8, &input, client_data->clipboard->length,
               writer, &output, sizeof(output_data));

    /* Send via VNC */
    SendClientCutText(rfb_client, output_data, output - output_data);

    return 0;
}

