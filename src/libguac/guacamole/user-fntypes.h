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

#ifndef _GUAC_USER_FNTYPES_H
#define _GUAC_USER_FNTYPES_H

/**
 * Function type definitions related to the guac_user object.
 *
 * @file user-fntypes.h
 */

#include "object-types.h"
#include "protocol-types.h"
#include "stream-types.h"
#include "timestamp-types.h"
#include "user-types.h"

/**
 * Callback which relates to a single guac_user at a time, along with arbitrary
 * data.
 *
 * @see guac_client_foreach_user()
 * @see guac_client_for_owner()
 */
typedef void* guac_user_callback(guac_user* user, void* data);

/**
 * Handler for Guacamole mouse events.
 */
typedef int guac_user_mouse_handler(guac_user* user, int x, int y,
        int button_mask);

/**
 * Handler for Guacamole key events.
 */
typedef int guac_user_key_handler(guac_user* user, int keysym, int pressed);

/**
 * Handler for Guacamole clipboard events.
 */
typedef int guac_user_clipboard_handler(guac_user* user, guac_stream* stream,
        char* mimetype);

/**
 * Handler for Guacamole screen size events.
 */
typedef int guac_user_size_handler(guac_user* user,
        int width, int height);

/**
 * Handler for Guacamole file transfer events.
 */
typedef int guac_user_file_handler(guac_user* user, guac_stream* stream,
        char* mimetype, char* filename);

/**
 * Handler for Guacamole pipe events.
 */
typedef int guac_user_pipe_handler(guac_user* user, guac_stream* stream,
        char* mimetype, char* name);

/**
 * Handler for Guacamole stream blob events.
 */
typedef int guac_user_blob_handler(guac_user* user, guac_stream* stream,
        void* data, int length);

/**
 * Handler for Guacamole stream ack events.
 */
typedef int guac_user_ack_handler(guac_user* user, guac_stream* stream,
        char* error, guac_protocol_status status);

/**
 * Handler for Guacamole stream end events.
 */
typedef int guac_user_end_handler(guac_user* user, guac_stream* stream);

/**
 * Handler for Guacamole audio format events.
 */
typedef int guac_user_audio_handler(guac_user* user, char* mimetype);

/**
 * Handler for Guacamole video format events.
 */
typedef int guac_user_video_handler(guac_user* user, char* mimetype);

/**
 * Handler for Guacamole join events. A join event is fired by the
 * guac_client whenever a guac_user joins the connection.
 */
typedef int guac_user_join_handler(guac_user* user, int argc, char** argv);

/**
 * Handler for Guacamole leave events. A leave event is fired by the
 * guac_client whenever a guac_user leaves the connection.
 */
typedef int guac_user_leave_handler(guac_user* user);

/**
 * Handler for Guacamole sync events. A sync event is fired by the
 * guac_client whenever a guac_user responds to a sync instruction.
 */
typedef int guac_user_sync_handler(guac_user* user, guac_timestamp timestamp);

/**
 * Handler for Guacamole object get events.
 */
typedef int guac_user_get_handler(guac_user* user, guac_object* object,
        char* name);

/**
 * Handler for Guacamole object put events.
 */
typedef int guac_user_put_handler(guac_user* user, guac_object* object,
        guac_stream* stream, char* mimetype, char* name);

#endif

