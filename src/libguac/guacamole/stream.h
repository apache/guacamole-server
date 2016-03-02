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

#ifndef _GUAC_STREAM_H
#define _GUAC_STREAM_H

/**
 * Provides functions and structures required for allocating and using streams.
 *
 * @file stream.h
 */

#include "user-fntypes.h"
#include "stream-types.h"

struct guac_stream {

    /**
     * The index of this stream.
     */
    int index;

    /**
     * Arbitrary data associated with this stream.
     */
    void* data;

    /**
     * Handler for ack events sent by the Guacamole web-client.
     *
     * The handler takes a guac_stream which contains the stream index and
     * will persist through the duration of the transfer, a string containing
     * the error or status message, and a status code.
     *
     * Example:
     * @code
     *     int ack_handler(guac_user* user, guac_stream* stream,
     *             char* error, guac_protocol_status status);
     *
     *     int some_function(guac_user* user) {
     *
     *         guac_stream* stream = guac_user_alloc_stream(user);
     *         stream->ack_handler = ack_handler;
     *
     *         guac_protocol_send_clipboard(user->socket,
     *             stream, "text/plain");
     *
     *     }
     * @endcode
     */
    guac_user_ack_handler* ack_handler;

    /**
     * Handler for blob events sent by the Guacamole web-client.
     *
     * The handler takes a guac_stream which contains the stream index and
     * will persist through the duration of the transfer, an arbitrary buffer
     * containing the blob, and the length of the blob.
     *
     * Example:
     * @code
     *     int blob_handler(guac_user* user, guac_stream* stream,
     *             void* data, int length);
     *
     *     int my_clipboard_handler(guac_user* user, guac_stream* stream,
     *             char* mimetype) {
     *         stream->blob_handler = blob_handler;
     *     }
     * @endcode
     */
    guac_user_blob_handler* blob_handler;

    /**
     * Handler for stream end events sent by the Guacamole web-client.
     *
     * The handler takes only a guac_stream which contains the stream index.
     * This guac_stream will be disposed of immediately after this event is
     * finished.
     *
     * Example:
     * @code
     *     int end_handler(guac_user* user, guac_stream* stream);
     *
     *     int my_clipboard_handler(guac_user* user, guac_stream* stream,
     *             char* mimetype) {
     *         stream->end_handler = end_handler;
     *     }
     * @endcode
     */
    guac_user_end_handler* end_handler;

};

#endif

