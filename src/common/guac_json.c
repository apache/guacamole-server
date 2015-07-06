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

#include "config.h"

#include "guac_json.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/stream.h>

void guac_common_json_flush(guac_client* client, guac_stream* stream,
        guac_common_json_state* json_state) {

    /* If JSON buffer is non-empty, write contents to blob and reset */
    if (json_state->size > 0) {
        guac_protocol_send_blob(client->socket, stream,
                json_state->buffer, json_state->size);

        /* Reset JSON buffer size */
        json_state->size = 0;

    }

}

int guac_common_json_write(guac_client* client, guac_stream* stream,
        guac_common_json_state* json_state, const char* buffer, int length) {

    int blob_written = 0;

    /*
     * Append to and flush the JSON buffer as necessary to write the given
     * data
     */
    while (length > 0) {

        /* Ensure provided data does not exceed size of buffer */
        int blob_length = length;
        if (blob_length > sizeof(json_state->buffer))
            blob_length = sizeof(json_state->buffer);

        /* Flush if more room is needed */
        if (json_state->size + blob_length > sizeof(json_state->buffer)) {
            guac_common_json_flush(client, stream, json_state);
            blob_written = 1;
        }

        /* Append data to JSON buffer */
        memcpy(json_state->buffer + json_state->size,
                buffer, blob_length);

        json_state->size += blob_length;

        /* Advance to next blob of data */
        buffer += blob_length;
        length -= blob_length;

    }

    return blob_written;

}

int guac_common_json_write_string(guac_client* client,
        guac_stream* stream, guac_common_json_state* json_state,
        const char* str) {

    int blob_written = 0;

    /* Write starting quote */
    blob_written |= guac_common_json_write(client, stream,
            json_state, "\"", 1);

    /* Write given string, escaping as necessary */
    const char* current = str;
    for (; *current != '\0'; current++) {

        /* Escape all quotes */
        if (*current == '"') {

            /* Write any string content up to current character */
            if (current != str)
                blob_written |= guac_common_json_write(client, stream,
                        json_state, str, current - str);

            /* Escape the quote that was just read */
            blob_written |= guac_common_json_write(client, stream,
                    json_state, "\\", 1);

            /* Reset string */
            str = current;

        }

    }

    /* Write any remaining string content */
    if (current != str)
        blob_written |= guac_common_json_write(client, stream,
                json_state, str, current - str);

    /* Write ending quote */
    blob_written |= guac_common_json_write(client, stream,
            json_state, "\"", 1);

    return blob_written;

}

int guac_common_json_write_property(guac_client* client, guac_stream* stream,
        guac_common_json_state* json_state, const char* name,
        const char* value) {

    int blob_written = 0;

    /* Write leading comma if not first property */
    if (json_state->properties_written != 0)
        blob_written |= guac_common_json_write(client, stream,
                json_state, ",", 1);

    /* Write property name */
    blob_written |= guac_common_json_write_string(client, stream,
            json_state, name);

    /* Separate name from value with colon */
    blob_written |= guac_common_json_write(client, stream,
            json_state, ":", 1);

    /* Write property value */
    blob_written |= guac_common_json_write_string(client, stream,
            json_state, value);

    json_state->properties_written++;

    return blob_written;

}

void guac_common_json_begin_object(guac_client* client, guac_stream* stream,
        guac_common_json_state* json_state) {

    /* Init JSON state */
    json_state->size = 0;
    json_state->properties_written = 0;

    /* Write leading brace - no blob can possibly be written by this */
    assert(!guac_common_json_write(client, stream, json_state, "{", 1));

}

int guac_common_json_end_object(guac_client* client, guac_stream* stream,
        guac_common_json_state* json_state) {

    /* Write final brace of JSON object */
    return guac_common_json_write(client, stream, json_state, "}", 1);

}

