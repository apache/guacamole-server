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

#ifndef __GUAC_CLIPBOARD_H
#define __GUAC_CLIPBOARD_H

#include "config.h"

#include <guacamole/client.h>

/**
 * The maximum number of bytes to send in an individual blob when
 * transmitting the clipboard contents to a connected client.
 */
#define GUAC_COMMON_CLIPBOARD_BLOCK_SIZE 4096

/**
 * Generic clipboard structure.
 */
typedef struct guac_common_clipboard {

    /**
     * The mimetype of the contained clipboard data.
     */
    char mimetype[256];

    /**
     * Arbitrary clipboard data.
     */
    char* buffer;

    /**
     * The number of bytes currently stored in the clipboard buffer.
     */
    int length;

    /**
     * The total number of bytes available in the clipboard buffer.
     */
    int available;

} guac_common_clipboard;

/**
 * Creates a new clipboard having the given initial size.
 *
 * @param size The maximum number of bytes to allow within the clipboard.
 * @return A newly-allocated clipboard.
 */
guac_common_clipboard* guac_common_clipboard_alloc(int size);

/**
 * Frees the given clipboard.
 *
 * @param clipboard The clipboard to free.
 */
void guac_common_clipboard_free(guac_common_clipboard* clipboard);

/**
 * Sends the contents of the clipboard along the given client, splitting
 * the contents as necessary.
 *
 * @param clipboard The clipboard whose contents should be sent.
 * @param client The client to send the clipboard contents on.
 */
void guac_common_clipboard_send(guac_common_clipboard* clipboard, guac_client* client);

/**
 * Clears the clipboard contents and assigns a new mimetype for future data.
 *
 * @param clipboard The clipboard to reset.
 * @param mimetype The mimetype of future data.
 */
void guac_common_clipboard_reset(guac_common_clipboard* clipboard, const char* mimetype);

/**
 * Appends the given data to the current clipboard contents. The data must
 * match the mimetype chosen for the clipboard data by
 * guac_common_clipboard_reset().
 *
 * @param clipboard The clipboard to append data to.
 * @param data The data to append.
 * @param length The number of bytes to append from the data given.
 */
void guac_common_clipboard_append(guac_common_clipboard* clipboard, const char* data, int length);

#endif

