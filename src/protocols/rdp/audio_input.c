/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "config.h"
#include "audio_input.h"
#include "ptr_string.h"
#include "rdp.h"

#include <freerdp/freerdp.h>
#include <freerdp/channels/channels.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/stream.h>
#include <guacamole/user.h>

#include <stdlib.h>

int guac_rdp_audio_handler(guac_user* user, guac_stream* stream,
        char* mimetype) {

    /* FIXME: Assuming mimetype of "audio/L16;rate=44100,channels=2" */

    /* Init stream data */
    stream->blob_handler = guac_rdp_audio_blob_handler;
    stream->end_handler = guac_rdp_audio_end_handler;

    guac_protocol_send_ack(user->socket, stream,
            "OK", GUAC_PROTOCOL_STATUS_SUCCESS);
    guac_socket_flush(user->socket);

    return 0;

}

int guac_rdp_audio_blob_handler(guac_user* user, guac_stream* stream,
        void* data, int length) {

    /* STUB */
    return 0;

}

int guac_rdp_audio_end_handler(guac_user* user, guac_stream* stream) {

    /* STUB */
    return 0;

}

void guac_rdp_audio_load_plugin(rdpContext* context) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;

    /* Add "AUDIO_INPUT" channel */
    ADDIN_ARGV* args = malloc(sizeof(ADDIN_ARGV));
    args->argc = 1;
    args->argv = malloc(sizeof(char**) * 1);
    args->argv[0] = strdup("guacai");
    args->argv[1] = guac_rdp_ptr_to_string(client);
    freerdp_dynamic_channel_collection_add(context->settings, args);

}

