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

#include "config.h"

#include "client.h"
#include "client-handlers.h"
#include "instruction.h"
#include "object.h"
#include "protocol.h"
#include "stream.h"
#include "timestamp.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

/* Guacamole instruction handler map */

__guac_instruction_handler_mapping __guac_instruction_handler_map[] = {
   {"sync",       __guac_handle_sync},
   {"mouse",      __guac_handle_mouse},
   {"key",        __guac_handle_key},
   {"clipboard",  __guac_handle_clipboard},
   {"disconnect", __guac_handle_disconnect},
   {"size",       __guac_handle_size},
   {"file",       __guac_handle_file},
   {"pipe",       __guac_handle_pipe},
   {"ack",        __guac_handle_ack},
   {"blob",       __guac_handle_blob},
   {"end",        __guac_handle_end},
   {"get",        __guac_handle_get},
   {"put",        __guac_handle_put},
   {NULL,         NULL}
};

int64_t __guac_parse_int(const char* str) {

    int sign = 1;
    int64_t num = 0;

    for (; *str != '\0'; str++) {

        if (*str == '-')
            sign = -sign;
        else
            num = num * 10 + (*str - '0');

    }

    return num * sign;

}

/* Guacamole instruction handlers */

int __guac_handle_sync(guac_client* client, guac_instruction* instruction) {
    guac_timestamp timestamp = __guac_parse_int(instruction->argv[0]);

    /* Error if timestamp is in future */
    if (timestamp > client->last_sent_timestamp)
        return -1;

    client->last_received_timestamp = timestamp;
    return 0;
}

int __guac_handle_mouse(guac_client* client, guac_instruction* instruction) {
    if (client->mouse_handler)
        return client->mouse_handler(
            client,
            atoi(instruction->argv[0]), /* x */
            atoi(instruction->argv[1]), /* y */
            atoi(instruction->argv[2])  /* mask */
        );
    return 0;
}

int __guac_handle_key(guac_client* client, guac_instruction* instruction) {
    if (client->key_handler)
        return client->key_handler(
            client,
            atoi(instruction->argv[0]), /* keysym */
            atoi(instruction->argv[1])  /* pressed */
        );
    return 0;
}

static guac_stream* __get_input_stream(guac_client* client, int stream_index) {

    /* Validate stream index */
    if (stream_index < 0 || stream_index >= GUAC_CLIENT_MAX_STREAMS) {

        guac_stream dummy_stream;
        dummy_stream.index = stream_index;

        guac_protocol_send_ack(client->socket, &dummy_stream,
                "Invalid stream index", GUAC_PROTOCOL_STATUS_CLIENT_BAD_REQUEST);
        return NULL;
    }

    return &(client->__input_streams[stream_index]);

}

static guac_stream* __get_open_input_stream(guac_client* client, int stream_index) {

    guac_stream* stream = __get_input_stream(client, stream_index);

    /* Fail if no such stream */
    if (stream == NULL)
        return NULL;

    /* Validate initialization of stream */
    if (stream->index == GUAC_CLIENT_CLOSED_STREAM_INDEX) {

        guac_stream dummy_stream;
        dummy_stream.index = stream_index;

        guac_protocol_send_ack(client->socket, &dummy_stream,
                "Invalid stream index", GUAC_PROTOCOL_STATUS_CLIENT_BAD_REQUEST);
        return NULL;
    }

    return stream;

}

static guac_stream* __init_input_stream(guac_client* client, int stream_index) {

    guac_stream* stream = __get_input_stream(client, stream_index);

    /* Fail if no such stream */
    if (stream == NULL)
        return NULL;

    /* Initialize stream */
    stream->index = stream_index;
    stream->data = NULL;
    stream->ack_handler = NULL;
    stream->blob_handler = NULL;
    stream->end_handler = NULL;

    return stream;

}

int __guac_handle_clipboard(guac_client* client, guac_instruction* instruction) {

    /* Pull corresponding stream */
    int stream_index = atoi(instruction->argv[0]);
    guac_stream* stream = __init_input_stream(client, stream_index);
    if (stream == NULL)
        return 0;

    /* If supported, call handler */
    if (client->clipboard_handler)
        return client->clipboard_handler(
            client,
            stream,
            instruction->argv[1] /* mimetype */
        );

    /* Otherwise, abort */
    guac_protocol_send_ack(client->socket, stream,
            "Clipboard unsupported", GUAC_PROTOCOL_STATUS_UNSUPPORTED);
    return 0;

}

int __guac_handle_size(guac_client* client, guac_instruction* instruction) {
    if (client->size_handler)
        return client->size_handler(
            client,
            atoi(instruction->argv[0]), /* width */
            atoi(instruction->argv[1])  /* height */
        );
    return 0;
}

int __guac_handle_file(guac_client* client, guac_instruction* instruction) {

    /* Pull corresponding stream */
    int stream_index = atoi(instruction->argv[0]);
    guac_stream* stream = __init_input_stream(client, stream_index);
    if (stream == NULL)
        return 0;

    /* If supported, call handler */
    if (client->file_handler)
        return client->file_handler(
            client,
            stream,
            instruction->argv[1], /* mimetype */
            instruction->argv[2]  /* filename */
        );

    /* Otherwise, abort */
    guac_protocol_send_ack(client->socket, stream,
            "File transfer unsupported", GUAC_PROTOCOL_STATUS_UNSUPPORTED);
    return 0;
}

int __guac_handle_pipe(guac_client* client, guac_instruction* instruction) {

    /* Pull corresponding stream */
    int stream_index = atoi(instruction->argv[0]);
    guac_stream* stream = __init_input_stream(client, stream_index);
    if (stream == NULL)
        return 0;

    /* If supported, call handler */
    if (client->pipe_handler)
        return client->pipe_handler(
            client,
            stream,
            instruction->argv[1], /* mimetype */
            instruction->argv[2]  /* name */
        );

    /* Otherwise, abort */
    guac_protocol_send_ack(client->socket, stream,
            "Named pipes unsupported", GUAC_PROTOCOL_STATUS_UNSUPPORTED);
    return 0;
}

int __guac_handle_ack(guac_client* client, guac_instruction* instruction) {

    guac_stream* stream;

    /* Validate stream index */
    int stream_index = atoi(instruction->argv[0]);
    if (stream_index < 0 || stream_index >= GUAC_CLIENT_MAX_STREAMS)
        return 0;

    stream = &(client->__output_streams[stream_index]);

    /* Validate initialization of stream */
    if (stream->index == GUAC_CLIENT_CLOSED_STREAM_INDEX)
        return 0;

    /* Call stream handler if defined */
    if (stream->ack_handler)
        return stream->ack_handler(client, stream, instruction->argv[1],
                atoi(instruction->argv[2]));

    /* Fall back to global handler if defined */
    if (client->ack_handler)
        return client->ack_handler(client, stream, instruction->argv[1],
                atoi(instruction->argv[2]));

    return 0;
}

int __guac_handle_blob(guac_client* client, guac_instruction* instruction) {

    int stream_index = atoi(instruction->argv[0]);
    guac_stream* stream = __get_open_input_stream(client, stream_index);

    /* Fail if no such stream */
    if (stream == NULL)
        return 0;

    /* Call stream handler if defined */
    if (stream->blob_handler) {
        int length = guac_protocol_decode_base64(instruction->argv[1]);
        return stream->blob_handler(client, stream, instruction->argv[1],
            length);
    }

    /* Fall back to global handler if defined */
    if (client->blob_handler) {
        int length = guac_protocol_decode_base64(instruction->argv[1]);
        return client->blob_handler(client, stream, instruction->argv[1],
            length);
    }

    guac_protocol_send_ack(client->socket, stream,
            "File transfer unsupported", GUAC_PROTOCOL_STATUS_UNSUPPORTED);
    return 0;
}

int __guac_handle_end(guac_client* client, guac_instruction* instruction) {

    int result = 0;
    int stream_index = atoi(instruction->argv[0]);
    guac_stream* stream = __get_open_input_stream(client, stream_index);

    /* Fail if no such stream */
    if (stream == NULL)
        return 0;

    /* Call stream handler if defined */
    if (stream->end_handler)
        result = stream->end_handler(client, stream);

    /* Fall back to global handler if defined */
    if (client->end_handler)
        result = client->end_handler(client, stream);

    /* Mark stream as closed */
    stream->index = GUAC_CLIENT_CLOSED_STREAM_INDEX;
    return result;
}

int __guac_handle_get(guac_client* client, guac_instruction* instruction) {

    guac_object* object;

    /* Validate object index */
    int object_index = atoi(instruction->argv[0]);
    if (object_index < 0 || object_index >= GUAC_CLIENT_MAX_OBJECTS)
        return 0;

    object = &(client->__objects[object_index]);

    /* Validate initialization of object */
    if (object->index == GUAC_CLIENT_UNDEFINED_OBJECT_INDEX)
        return 0;

    /* Call object handler if defined */
    if (object->get_handler)
        return object->get_handler(client, object,
            instruction->argv[1] /* name */
        );

    /* Fall back to global handler if defined */
    if (client->get_handler)
        return client->get_handler(client, object,
            instruction->argv[1] /* name */
        );

    return 0;
}

int __guac_handle_put(guac_client* client, guac_instruction* instruction) {

    guac_object* object;

    /* Validate object index */
    int object_index = atoi(instruction->argv[0]);
    if (object_index < 0 || object_index >= GUAC_CLIENT_MAX_OBJECTS)
        return 0;

    object = &(client->__objects[object_index]);

    /* Validate initialization of object */
    if (object->index == GUAC_CLIENT_UNDEFINED_OBJECT_INDEX)
        return 0;

    /* Pull corresponding stream */
    int stream_index = atoi(instruction->argv[1]);
    guac_stream* stream = __init_input_stream(client, stream_index);
    if (stream == NULL)
        return 0;

    /* Call object handler if defined */
    if (object->put_handler)
        return object->put_handler(client, object, stream,
            instruction->argv[2], /* mimetype */
            instruction->argv[3]  /* name */
        );

    /* Fall back to global handler if defined */
    if (client->put_handler)
        return client->put_handler(client, object, stream,
            instruction->argv[2], /* mimetype */
            instruction->argv[3]  /* name */
        );

    /* Otherwise, abort */
    guac_protocol_send_ack(client->socket, stream,
            "Object write unsupported", GUAC_PROTOCOL_STATUS_UNSUPPORTED);
    return 0;
}

int __guac_handle_disconnect(guac_client* client, guac_instruction* instruction) {
    guac_client_stop(client);
    return 0;
}

