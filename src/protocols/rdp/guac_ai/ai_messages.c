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

#include "ai_messages.h"
#include "rdp.h"

#include <stdlib.h>

#include <freerdp/freerdp.h>
#include <freerdp/constants.h>
#include <freerdp/dvc.h>
#include <guacamole/client.h>

#ifdef ENABLE_WINPR
#include <winpr/stream.h>
#else
#include "compat/winpr-stream.h"
#endif

void guac_rdp_ai_process_version(guac_client* client,
        IWTSVirtualChannel* channel, wStream* stream) {

    UINT32 version;
    Stream_Read_UINT32(stream, version);

    /* Warn if server's version number is incorrect */
    if (version != 1)
        guac_client_log(client, GUAC_LOG_WARNING,
                "Server reports AUDIO_INPUT version %i, not 1", version);

    /* Build response version PDU */
    wStream* response = Stream_New(NULL, 5);
    Stream_Write_UINT8(response,  GUAC_RDP_MSG_SNDIN_VERSION); /* MessageId */
    Stream_Write_UINT32(response, 1); /* Version */

    /* Send response */
    channel->Write(channel, (UINT32) Stream_GetPosition(response),
            Stream_Buffer(response), NULL);
    Stream_Free(response, TRUE);

}

void guac_rdp_ai_process_formats(guac_client* client,
        IWTSVirtualChannel* channel, wStream* stream) {

    /* STUB */
    guac_client_log(client, GUAC_LOG_DEBUG, "AUDIO_INPUT: formats");

}

void guac_rdp_ai_process_open(guac_client* client,
        IWTSVirtualChannel* channel, wStream* stream) {

    /* STUB */
    guac_client_log(client, GUAC_LOG_DEBUG, "AUDIO_INPUT: open");

}

void guac_rdp_ai_process_formatchange(guac_client* client,
        IWTSVirtualChannel* channel, wStream* stream) {

    /* STUB */
    guac_client_log(client, GUAC_LOG_DEBUG, "AUDIO_INPUT: formatchange");

}

