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

#include "clipboard.h"
#include "ipmi.h"
#include "terminal/terminal.h"

#include <guacamole/client.h>
#include <guacamole/stream.h>
#include <guacamole/user.h>

int guac_ipmi_clipboard_handler(guac_user* user, guac_stream* stream,
        char* mimetype) {

    /* Clear clipboard and prepare for new data */
    guac_client* client = user->client;
    guac_ipmi_client* ipmi_client = (guac_ipmi_client*) client->data;
    guac_terminal_clipboard_reset(ipmi_client->term, mimetype);

    /* Set handlers for clipboard stream */
    stream->blob_handler = guac_ipmi_clipboard_blob_handler;
    stream->end_handler = guac_ipmi_clipboard_end_handler;

    /* Report clipboard within recording */
    if (ipmi_client->recording != NULL)
        guac_recording_report_clipboard_begin(ipmi_client->recording, stream,
                mimetype);

    return 0;
}

int guac_ipmi_clipboard_blob_handler(guac_user* user, guac_stream* stream,
        void* data, int length) {

    guac_client* client = user->client;
    guac_ipmi_client* ipmi_client = (guac_ipmi_client*) client->data;

    /* Report clipboard blob within recording */
    if (ipmi_client->recording != NULL)
        guac_recording_report_clipboard_blob(ipmi_client->recording, stream, data, length);

    /* Append new data */
    guac_terminal_clipboard_append(ipmi_client->term, data, length);

    return 0;
}

int guac_ipmi_clipboard_end_handler(guac_user* user, guac_stream* stream) {

    guac_client* client = user->client;
    guac_ipmi_client* ipmi_client = (guac_ipmi_client*) client->data;

    /* Report clipboard stream end within recording */
    if (ipmi_client->recording != NULL)
        guac_recording_report_clipboard_end(ipmi_client->recording, stream);

    /* Nothing to do - clipboard is implemented within client */

    return 0;
}
