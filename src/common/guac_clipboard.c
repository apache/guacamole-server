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
#include "guac_clipboard.h"

#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <guacamole/stream.h>
#include <guacamole/user.h>
#include <string.h>
#include <stdlib.h>

guac_common_clipboard* guac_common_clipboard_alloc(int size) {

    guac_common_clipboard* clipboard = malloc(sizeof(guac_common_clipboard));

    /* Init clipboard */
    clipboard->mimetype[0] = '\0';
    clipboard->buffer = malloc(size);
    clipboard->length = 0;
    clipboard->available = size;

    return clipboard;

}

void guac_common_clipboard_free(guac_common_clipboard* clipboard) {
    free(clipboard->buffer);
    free(clipboard);
}

/**
 * Callback for guac_client_foreach_user() which sends clipboard data to each
 * connected client.
 */
static void* __send_user_clipboard(guac_user* user, void* data) {

    guac_common_clipboard* clipboard = (guac_common_clipboard*) data;

    char* current = clipboard->buffer;
    int remaining = clipboard->length;

    /* Begin stream */
    guac_stream* stream = guac_user_alloc_stream(user);
    guac_protocol_send_clipboard(user->socket, stream, clipboard->mimetype);

    guac_user_log(user, GUAC_LOG_DEBUG,
            "Created stream %i for %s clipboard data.",
            stream->index, clipboard->mimetype);

    /* Split clipboard into chunks */
    while (remaining > 0) {

        /* Calculate size of next block */
        int block_size = GUAC_COMMON_CLIPBOARD_BLOCK_SIZE;
        if (remaining < block_size)
            block_size = remaining; 

        /* Send block */
        guac_protocol_send_blob(user->socket, stream, current, block_size);
        guac_user_log(user, GUAC_LOG_DEBUG,
                "Sent %i bytes of clipboard data on stream %i.",
                block_size, stream->index);

        /* Next block */
        remaining -= block_size;
        current += block_size;

    }

    guac_user_log(user, GUAC_LOG_DEBUG,
            "Clipboard stream %i complete.",
            stream->index);

    /* End stream */
    guac_protocol_send_end(user->socket, stream);
    guac_user_free_stream(user, stream);

    return NULL;

}

void guac_common_clipboard_send(guac_common_clipboard* clipboard, guac_client* client) {
    guac_client_log(client, GUAC_LOG_DEBUG, "Broadcasting clipboard to all connected users.");
    guac_client_foreach_user(client, __send_user_clipboard, clipboard);
    guac_client_log(client, GUAC_LOG_DEBUG, "Broadcast of clipboard complete.");
}

void guac_common_clipboard_reset(guac_common_clipboard* clipboard, const char* mimetype) {
    clipboard->length = 0;
    strncpy(clipboard->mimetype, mimetype, sizeof(clipboard->mimetype)-1);
}

void guac_common_clipboard_append(guac_common_clipboard* clipboard, const char* data, int length) {

    /* Truncate data to available length */
    int remaining = clipboard->available - clipboard->length;
    if (remaining < length)
        length = remaining;

    /* Append to buffer */
    memcpy(clipboard->buffer + clipboard->length, data, length);

    /* Update length */
    clipboard->length += length;

}

