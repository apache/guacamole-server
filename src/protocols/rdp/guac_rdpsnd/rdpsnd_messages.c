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

#include "rdp.h"
#include "rdpsnd_messages.h"
#include "rdpsnd_service.h"

#include <pthread.h>
#include <stdlib.h>

#include <freerdp/utils/svc_plugin.h>
#include <guacamole/audio.h>
#include <guacamole/client.h>

#ifdef ENABLE_WINPR
#include <winpr/stream.h>
#include <winpr/wtypes.h>
#else
#include "compat/winpr-stream.h"
#include "compat/winpr-wtypes.h"
#endif

/* MESSAGE HANDLERS */

void guac_rdpsnd_formats_handler(guac_rdpsndPlugin* rdpsnd,
        wStream* input_stream, guac_rdpsnd_pdu_header* header) {

    int server_format_count;
    int server_version;
    int i;

    wStream* output_stream;
    int output_body_size;
    unsigned char* output_stream_end;

    /* Get associated client data */
    guac_client* client = rdpsnd->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    /* Get audio stream from client data */
    guac_audio_stream* audio = rdp_client->audio;

    /* Reset own format count */
    rdpsnd->format_count = 0;

    /* Format header */
    Stream_Seek(input_stream, 14);
    Stream_Read_UINT16(input_stream, server_format_count);
    Stream_Seek_UINT8(input_stream);
    Stream_Read_UINT16(input_stream, server_version);
    Stream_Seek_UINT8(input_stream);

    /* Initialize Client Audio Formats and Version PDU */
    output_stream = Stream_New(NULL, 24);
    Stream_Write_UINT8(output_stream,  SNDC_FORMATS);
    Stream_Write_UINT8(output_stream,  0);

    /* Fill in body size later */
    Stream_Seek_UINT16(output_stream); /* offset = 0x02 */

    /* Flags, volume, and pitch */
    Stream_Write_UINT32(output_stream, TSSNDCAPS_ALIVE);
    Stream_Write_UINT32(output_stream, 0);
    Stream_Write_UINT32(output_stream, 0);

    /* Datagram port (UDP) */
    Stream_Write_UINT16(output_stream, 0);

    /* Fill in format count later */
    Stream_Seek_UINT16(output_stream); /* offset = 0x12 */

    /* Version and padding */
    Stream_Write_UINT8(output_stream,  0);
    Stream_Write_UINT16(output_stream, 6);
    Stream_Write_UINT8(output_stream,  0);

    /* Check each server format, respond if supported and audio is enabled */
    if (audio != NULL) {
        for (i=0; i < server_format_count; i++) {

            unsigned char* format_start;

            int format_tag;
            int channels;
            int rate;
            int bps;
            int body_size;

            /* Remember position in stream */
            Stream_GetPointer(input_stream, format_start);

            /* Read format */
            Stream_Read_UINT16(input_stream, format_tag);
            Stream_Read_UINT16(input_stream, channels);
            Stream_Read_UINT32(input_stream, rate);
            Stream_Seek_UINT32(input_stream);
            Stream_Seek_UINT16(input_stream);
            Stream_Read_UINT16(input_stream, bps);

            /* Skip past extra data */
            Stream_Read_UINT16(input_stream, body_size);
            Stream_Seek(input_stream, body_size);

            /* If PCM, accept */
            if (format_tag == WAVE_FORMAT_PCM) {

                /* If can fit another format, accept it */
                if (rdpsnd->format_count < GUAC_RDP_MAX_FORMATS) {

                    /* Add channel */
                    int current = rdpsnd->format_count++;
                    rdpsnd->formats[current].rate     = rate;
                    rdpsnd->formats[current].channels = channels;
                    rdpsnd->formats[current].bps      = bps;

                    /* Log format */
                    guac_client_log(client, GUAC_LOG_INFO,
                            "Accepted format: %i-bit PCM with %i channels at "
                            "%i Hz",
                            bps, channels, rate);

                    /* Ensure audio stream is configured to use accepted
                     * format */
                    guac_audio_stream_reset(audio, NULL, rate, channels, bps);

                    /* Queue format for sending as accepted */
                    Stream_EnsureRemainingCapacity(output_stream,
                            18 + body_size);
                    Stream_Write(output_stream, format_start, 18 + body_size);

                    /*
                     * BEWARE that using Stream_EnsureRemainingCapacity means
                     * that any pointers returned via Stream_GetPointer on
                     * output_stream are invalid.
                     */

                }

                /* Otherwise, log that we dropped one */
                else
                    guac_client_log(client, GUAC_LOG_INFO,
                            "Dropped valid format: %i-bit PCM with %i "
                            "channels at %i Hz",
                            bps, channels, rate);

            }

        }
    }

    /* Otherwise, ignore all supported formats as we do not intend to actually
     * receive audio */
    else
        guac_client_log(client, GUAC_LOG_DEBUG,
                "Audio explicitly disabled. Ignoring supported formats.");

    /* Calculate size of PDU */
    output_body_size = Stream_GetPosition(output_stream) - 4;
    Stream_GetPointer(output_stream, output_stream_end);

    /* Set body size */
    Stream_SetPosition(output_stream, 0x02);
    Stream_Write_UINT16(output_stream, output_body_size);

    /* Set format count */
    Stream_SetPosition(output_stream, 0x12);
    Stream_Write_UINT16(output_stream, rdpsnd->format_count);

    /* Reposition cursor at end (necessary for message send) */
    Stream_SetPointer(output_stream, output_stream_end);

    /* Send accepted formats */
    pthread_mutex_lock(&(rdp_client->rdp_lock));
    svc_plugin_send((rdpSvcPlugin*)rdpsnd, output_stream);

    /* If version greater than 6, must send Quality Mode PDU */
    if (server_version >= 6) {

        /* Always send High Quality for now */
        output_stream = Stream_New(NULL, 8);
        Stream_Write_UINT8(output_stream, SNDC_QUALITYMODE);
        Stream_Write_UINT8(output_stream, 0);
        Stream_Write_UINT16(output_stream, 4);
        Stream_Write_UINT16(output_stream, HIGH_QUALITY);
        Stream_Write_UINT16(output_stream, 0);

        svc_plugin_send((rdpSvcPlugin*)rdpsnd, output_stream);
    }

    pthread_mutex_unlock(&(rdp_client->rdp_lock));

}

/* server is getting a feel of the round trip time */
void guac_rdpsnd_training_handler(guac_rdpsndPlugin* rdpsnd,
        wStream* input_stream, guac_rdpsnd_pdu_header* header) {

    int data_size;
    wStream* output_stream;

    /* Get associated client data */
    guac_client* client = rdpsnd->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    /* Read timestamp and data size */
    Stream_Read_UINT16(input_stream, rdpsnd->server_timestamp);
    Stream_Read_UINT16(input_stream, data_size);

    /* Send training response */
    output_stream = Stream_New(NULL, 8);
    Stream_Write_UINT8(output_stream, SNDC_TRAINING);
    Stream_Write_UINT8(output_stream, 0);
    Stream_Write_UINT16(output_stream, 4);
    Stream_Write_UINT16(output_stream, rdpsnd->server_timestamp);
    Stream_Write_UINT16(output_stream, data_size);

    pthread_mutex_lock(&(rdp_client->rdp_lock));
    svc_plugin_send((rdpSvcPlugin*) rdpsnd, output_stream);
    pthread_mutex_unlock(&(rdp_client->rdp_lock));

}

void guac_rdpsnd_wave_info_handler(guac_rdpsndPlugin* rdpsnd,
        wStream* input_stream, guac_rdpsnd_pdu_header* header) {

    int format;

    /* Get associated client data */
    guac_client* client = rdpsnd->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    /* Get audio stream from client data */
    guac_audio_stream* audio = rdp_client->audio;

    /* Read wave information */
    Stream_Read_UINT16(input_stream, rdpsnd->server_timestamp);
    Stream_Read_UINT16(input_stream, format);
    Stream_Read_UINT8(input_stream, rdpsnd->waveinfo_block_number);
    Stream_Seek(input_stream, 3);
    Stream_Read(input_stream, rdpsnd->initial_wave_data, 4);

    /*
     * Size of incoming wave data is equal to the body size field of this
     * header, less the size of a WaveInfo PDU (not including the header),
     * thus body_size - 12.
     */
    rdpsnd->incoming_wave_size = header->body_size - 12;

    /* Read wave in next iteration */
    rdpsnd->next_pdu_is_wave = TRUE;

    /* Reset audio stream if format has changed */
    if (audio != NULL)
        guac_audio_stream_reset(audio, NULL,
                rdpsnd->formats[format].rate,
                rdpsnd->formats[format].channels,
                rdpsnd->formats[format].bps);

}

void guac_rdpsnd_wave_handler(guac_rdpsndPlugin* rdpsnd,
        wStream* input_stream, guac_rdpsnd_pdu_header* header) {

    rdpSvcPlugin* plugin = (rdpSvcPlugin*)rdpsnd;

    /* Get associated client data */
    guac_client* client = rdpsnd->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    /* Get audio stream from client data */
    guac_audio_stream* audio = rdp_client->audio;

    /* Wave Confirmation PDU */
    wStream* output_stream = Stream_New(NULL, 8);

    /* Get wave data */
    unsigned char* buffer = Stream_Buffer(input_stream);

    /* Copy over first four bytes */
    memcpy(buffer, rdpsnd->initial_wave_data, 4);

    /* Write rest of audio packet */
    if (audio != NULL) {
        guac_audio_stream_write_pcm(audio, buffer,
                rdpsnd->incoming_wave_size + 4);
        guac_audio_stream_flush(audio);
    }

    /* Write Wave Confirmation PDU */
    Stream_Write_UINT8(output_stream, SNDC_WAVECONFIRM);
    Stream_Write_UINT8(output_stream, 0);
    Stream_Write_UINT16(output_stream, 4);
    Stream_Write_UINT16(output_stream, rdpsnd->server_timestamp);
    Stream_Write_UINT8(output_stream, rdpsnd->waveinfo_block_number);
    Stream_Write_UINT8(output_stream, 0);

    /* Send Wave Confirmation PDU */
    pthread_mutex_lock(&(rdp_client->rdp_lock));
    svc_plugin_send(plugin, output_stream);
    pthread_mutex_unlock(&(rdp_client->rdp_lock));

    /* We no longer expect to receive wave data */
    rdpsnd->next_pdu_is_wave = FALSE;

}

void guac_rdpsnd_close_handler(guac_rdpsndPlugin* rdpsnd,
        wStream* input_stream, guac_rdpsnd_pdu_header* header) {

    /* Do nothing */

}

