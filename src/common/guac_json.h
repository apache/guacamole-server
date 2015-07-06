/*
 * Copyright (C) 2015 Glyptodon LLC
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

#ifndef GUAC_COMMON_JSON_H
#define GUAC_COMMON_JSON_H

#include "config.h"

#include <guacamole/client.h>
#include <guacamole/stream.h>

/**
 * The current streaming state of an arbitrary JSON object, consisting of
 * any number of property name/value pairs.
 */
typedef struct guac_common_json_state {

    /**
     * Buffer of partial JSON data. The individual blobs which make up the JSON
     * body of the object being sent over the Guacamole protocol will be
     * built here.
     */
    char buffer[4096];

    /**
     * The number of bytes currently used within the JSON buffer.
     */
    int size;

    /**
     * The number of property name/value pairs written to the JSON object thus
     * far.
     */
    int properties_written;

} guac_common_json_state;

/**
 * Given a stream, the client to which it belongs, and the current stream state
 * of a JSON object, flushes the contents of the JSON buffer to a blob
 * instruction. Note that this will flush the JSON buffer only, and will not
 * necessarily flush the underlying guac_socket of the client.
 *
 * @param client
 *     The client to which the data will be flushed.
 *
 * @param stream
 *     The stream through which the flushed data should be sent as a blob.
 *
 * @param json_state
 *     The state object whose buffer should be flushed.
 */
void guac_common_json_flush(guac_client* client, guac_stream* stream,
        guac_common_json_state* json_state);

/**
 * Given a stream, the client to which it belongs, and the current stream state
 * of a JSON object, writes the contents of the given buffer to the JSON buffer
 * of the stream state, flushing as necessary.
 *
 * @param client
 *     The client to which the data will be flushed as necessary.
 *
 * @param stream
 *     The stream through which the flushed data should be sent as a blob, if
 *     data must be flushed at all.
 *
 * @param json_state
 *     The state object containing the JSON buffer to which the given buffer
 *     should be written.
 *
 * @param buffer
 *     The buffer to write.
 *
 * @param length
 *     The number of bytes in the buffer.
 *
 * @return
 *     Non-zero if at least one blob was written, zero otherwise.
 */
int guac_common_json_write(guac_client* client, guac_stream* stream,
        guac_common_json_state* json_state, const char* buffer, int length);

/**
 * Given a stream, the client to which it belongs, and the current stream state
 * of a JSON object state, writes the given string as a proper JSON string,
 * including starting and ending quotes. The contents of the string will be
 * escaped as necessary.
 *
 * @param client
 *     The client to which the data will be flushed as necessary.
 *
 * @param stream
 *     The stream through which the flushed data should be sent as a blob, if
 *     data must be flushed at all.
 *
 * @param json_state
 *     The state object containing the JSON buffer to which the given string
 *     should be written as a JSON name/value pair.
 *
 * @param str
 *     The string to write.
 *
 * @return
 *     Non-zero if at least one blob was written, zero otherwise.
 */
int guac_common_json_write_string(guac_client* client,
        guac_stream* stream, guac_common_json_state* json_state,
        const char* str);

/**
 * Given a stream, the client to which it belongs, and the current stream state
 * of a JSON object, writes the given JSON property name/value pair. The
 * name and value will be written as proper JSON strings separated by a colon.
 *
 * @param client
 *     The client to which the data will be flushed as necessary.
 *
 * @param stream
 *     The stream through which the flushed data should be sent as a blob, if
 *     data must be flushed at all.
 *
 * @param json_state
 *     The state object containing the JSON buffer to which the given strings
 *     should be written as a JSON name/value pair.
 *
 * @param name
 *     The name of the property to write.
 *
 * @param value
 *     The value of the property to write.
 *
 * @return
 *     Non-zero if at least one blob was written, zero otherwise.
 */
int guac_common_json_write_property(guac_client* client, guac_stream* stream,
        guac_common_json_state* json_state, const char* name,
        const char* value);

/**
 * Given a stream, the client to which it belongs, and the current stream state
 * of a JSON object, initializes the state for writing a new JSON object. Note
 * that although the client and stream must be provided, no instruction or
 * blobs will be written due to any call to this function.
 *
 * @param client
 *     The client associated with the given stream.
 *
 * @param stream
 *     The stream associated with the JSON object being written.
 *
 * @param json_state
 *     The state object to initialize.
 */
void guac_common_json_begin_object(guac_client* client, guac_stream* stream,
        guac_common_json_state* json_state);

/**
 * Given a stream, the client to which it belongs, and the current stream state
 * of a JSON object, completes writing that JSON object by writing the final
 * terminating brace. This function must only be called following a
 * corresponding call to guac_common_json_begin_object().
 *
 * @param client
 *     The client associated with the given stream.
 *
 * @param stream
 *     The stream associated with the JSON object being written.
 *
 * @param json_state
 *     The state object whose in-progress JSON object should be terminated.
 *
 * @return
 *     Non-zero if at least one blob was written, zero otherwise.
 */
int guac_common_json_end_object(guac_client* client, guac_stream* stream,
        guac_common_json_state* json_state);

#endif

