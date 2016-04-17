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

#ifndef GUAC_RDP_AI_MESSAGES_H
#define GUAC_RDP_AI_MESSAGES_H

#include "config.h"

#include <freerdp/dvc.h>
#include <guacamole/client.h>

#ifdef ENABLE_WINPR
#include <winpr/stream.h>
#else
#include "compat/winpr-stream.h"
#endif

/**
 * The message ID associated with the AUDIO_INPUT Version PDU. The Version PDU
 * is sent by both the client and the server to indicate their version of the
 * AUDIO_INPUT channel protocol (which must always be 1).
 */
#define GUAC_RDP_MSG_SNDIN_VERSION 0x01

/**
 * The message ID associated with the AUDIO_INPUT Sound Formats PDU. The
 * Sound Formats PDU is sent by the client and the server to indicate the
 * formats of audio supported.
 */
#define GUAC_RDP_MSG_SNDIN_FORMATS 0x02

/**
 * The message ID associated with the AUDIO_INPUT Open PDU. The Open PDU is
 * sent by the server to inform the client that the AUDIO_INPUT channel is
 * now open.
 */
#define GUAC_RDP_MSG_SNDIN_OPEN 0x03

/**
 * The message ID associated with the AUDIO_INPUT Open Reply PDU. The Open
 * Reply PDU is sent by the client (after sending a Format Change PDU) to
 * acknowledge that the AUDIO_INPUT channel is open.
 */
#define GUAC_RDP_MSG_SNDIN_OPEN_REPLY 0x04

/**
 * The message ID associated with the AUDIO_INPUT Incoming Data PDU. The
 * Incoming Data PDU is sent by the client to inform the server of incoming
 * sound format or audio data.
 */
#define GUAC_RDP_MSG_SNDIN_DATA_INCOMING 0x05

/**
 * The message ID associated with the AUDIO_INPUT Data PDU. The Data PDU is
 * sent by the client and contains audio data read from the microphone.
 */
#define GUAC_RDP_MSG_SNDIN_DATA 0x06

/**
 * The message ID associated with the AUDIO_INPUT Format Change PDU. The Format
 * Change PDU is sent by the client to acknowledge the current sound format, or
 * by the server to request a different sound format.
 */
#define GUAC_RDP_MSG_SNDIN_FORMATCHANGE 0x07

/**
 * Processes a Version PDU received from the RDP server. The Version PDU is
 * sent by the server to indicate its version of the AUDIO_INPUT channel
 * protocol (which must always be 1).
 *
 * @param client
 *     The guac_client associated with the current RDP connection.
 *
 * @param channel
 *     The IWTSVirtualChannel instance associated with the connected
 *     AUDIO_INPUT channel.
 *
 * @param stream
 *     The received PDU, with the read position just after the message ID field
 *     common to all AUDIO_INPUT PDUs.
 */
void guac_rdp_ai_process_version(guac_client* client,
        IWTSVirtualChannel* channel, wStream* stream);

/**
 * Processes a Sound Formats PDU received from the RDP server. The Sound
 * Formats PDU is sent by the server to indicate the formats of audio
 * supported.
 *
 * @param client
 *     The guac_client associated with the current RDP connection.
 *
 * @param channel
 *     The IWTSVirtualChannel instance associated with the connected
 *     AUDIO_INPUT channel.
 *
 * @param stream
 *     The received PDU, with the read position just after the message ID field
 *     common to all AUDIO_INPUT PDUs.
 */
void guac_rdp_ai_process_formats(guac_client* client,
        IWTSVirtualChannel* channel, wStream* stream);

/**
 * Processes a Open PDU received from the RDP server. The Open PDU is sent by
 * the server to inform the client that the AUDIO_INPUT channel is now open.
 *
 * @param client
 *     The guac_client associated with the current RDP connection.
 *
 * @param channel
 *     The IWTSVirtualChannel instance associated with the connected
 *     AUDIO_INPUT channel.
 *
 * @param stream
 *     The received PDU, with the read position just after the message ID field
 *     common to all AUDIO_INPUT PDUs.
 */
void guac_rdp_ai_process_open(guac_client* client,
        IWTSVirtualChannel* channel, wStream* stream);

/**
 * Processes a Format Change PDU received from the RDP server. The Format
 * Change PDU is sent by the server to request a different sound format.
 *
 * @param client
 *     The guac_client associated with the current RDP connection.
 *
 * @param channel
 *     The IWTSVirtualChannel instance associated with the connected
 *     AUDIO_INPUT channel.
 *
 * @param stream
 *     The received PDU, with the read position just after the message ID field
 *     common to all AUDIO_INPUT PDUs.
 */
void guac_rdp_ai_process_formatchange(guac_client* client,
        IWTSVirtualChannel* channel, wStream* stream);

#endif

