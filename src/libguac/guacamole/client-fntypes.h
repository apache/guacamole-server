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

#ifndef _GUAC_CLIENT_FNTYPES_H
#define _GUAC_CLIENT_FNTYPES_H

/**
 * Function type definitions related to the Guacamole client structure,
 * guac_client.
 *
 * @file client-fntypes.h
 */

#include "client-types.h"
#include "object-types.h"
#include "protocol-types.h"
#include "stream-types.h"

#include <stdarg.h>

/**
 * Handler for server messages (where "server" refers to the server that
 * the proxy client is connected to).
 */
typedef int guac_client_handle_messages(guac_client* client);

/**
 * Handler for Guacamole mouse events.
 */
typedef int guac_client_mouse_handler(guac_client* client, int x, int y, int button_mask);

/**
 * Handler for Guacamole key events.
 */
typedef int guac_client_key_handler(guac_client* client, int keysym, int pressed);

/**
 * Handler for Guacamole clipboard events.
 */
typedef int guac_client_clipboard_handler(guac_client* client, guac_stream* stream,
        char* mimetype);
/**
 * Handler for Guacamole screen size events.
 */
typedef int guac_client_size_handler(guac_client* client,
        int width, int height);

/**
 * Handler for Guacamole file transfer events.
 */
typedef int guac_client_file_handler(guac_client* client, guac_stream* stream,
        char* mimetype, char* filename);

/**
 * Handler for Guacamole pipe events.
 */
typedef int guac_client_pipe_handler(guac_client* client, guac_stream* stream,
        char* mimetype, char* name);

/**
 * Handler for Guacamole stream blob events.
 */
typedef int guac_client_blob_handler(guac_client* client, guac_stream* stream,
        void* data, int length);

/**
 * Handler for Guacamole stream ack events.
 */
typedef int guac_client_ack_handler(guac_client* client, guac_stream* stream,
        char* error, guac_protocol_status status);

/**
 * Handler for Guacamole stream end events.
 */
typedef int guac_client_end_handler(guac_client* client, guac_stream* stream);

/**
 * Handler for Guacamole object get events.
 */
typedef int guac_client_get_handler(guac_client* client, guac_object* object,
        char* name);

/**
 * Handler for Guacamole object put events.
 */
typedef int guac_client_put_handler(guac_client* client, guac_object* object,
        guac_stream* stream, char* mimetype, char* name);

/**
 * Handler for Guacamole audio format events.
 */
typedef int guac_client_audio_handler(guac_client* client, char* mimetype);

/**
 * Handler for Guacamole video format events.
 */
typedef int guac_client_video_handler(guac_client* client, char* mimetype);

/**
 * Handler for freeing up any extra data allocated by the client
 * implementation.
 */
typedef int guac_client_free_handler(guac_client* client);

/**
 * Handler for logging messages
 */
typedef void guac_client_log_handler(guac_client* client, guac_client_log_level level, const char* format, va_list args); 

/**
 * Handler which should initialize the given guac_client.
 */
typedef int guac_client_init_handler(guac_client* client, int argc, char** argv);

#endif

