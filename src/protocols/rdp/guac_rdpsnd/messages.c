
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is libguac-client-rdp.
 *
 * The Initial Developer of the Original Code is
 * Michael Jumper.
 * Portions created by the Initial Developer are Copyright (C) 2011
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <freerdp/constants.h>
#include <freerdp/types.h>
#include <freerdp/utils/memory.h>
#include <freerdp/utils/stream.h>
#include <freerdp/utils/svc_plugin.h>

#include <guacamole/client.h>

#include "audio.h"
#include "service.h"
#include "messages.h"
#include "client.h"

/* MESSAGE HANDLERS */

void guac_rdpsnd_formats_handler(guac_rdpsndPlugin* rdpsnd,
        audio_stream* audio, STREAM* input_stream, 
        guac_rdpsnd_pdu_header* header) {

    int server_format_count;
    int server_version;
    int i;

    STREAM* output_stream;
    int output_body_size;
    unsigned char* output_stream_end;

    rdp_guac_client_data* guac_client_data =
        (rdp_guac_client_data*) audio->client->data;

    /* Format header */
    stream_seek(input_stream, 14);
    stream_read_uint16(input_stream, server_format_count);
    stream_seek_uint8(input_stream);
    stream_read_uint16(input_stream, server_version);
    stream_seek_uint8(input_stream);

    /* Initialize Client Audio Formats and Version PDU */
    output_stream = stream_new(24);
    stream_write_uint8(output_stream,  SNDC_FORMATS);
    stream_write_uint8(output_stream,  0);

    /* Fill in body size later */
    stream_seek_uint16(output_stream); /* offset = 0x02 */

    /* Flags, volume, and pitch */
    stream_write_uint32(output_stream, TSSNDCAPS_ALIVE);
    stream_write_uint32(output_stream, 0);
    stream_write_uint32(output_stream, 0);

    /* Datagram port (UDP) */
    stream_write_uint16(output_stream, 0);

    /* Fill in format count later */
    stream_seek_uint16(output_stream); /* offset = 0x12 */

    /* Version and padding */
    stream_write_uint8(output_stream,  0);
    stream_write_uint16(output_stream, 6);
    stream_write_uint8(output_stream,  0);

    /* Check each server format, respond if supported */
    for (i=0; i < server_format_count; i++) {

        unsigned char* format_start;

        int format_tag;
        int channels;
        int rate;
        int bps;
        int body_size;

        /* Remember position in stream */
        stream_get_mark(input_stream, format_start);

        /* Read format */
        stream_read_uint16(input_stream, format_tag);
        stream_read_uint16(input_stream, channels);
        stream_read_uint32(input_stream, rate);
        stream_seek_uint32(input_stream);
        stream_seek_uint16(input_stream);
        stream_read_uint16(input_stream, bps);

        /* Skip past extra data */
        stream_read_uint16(input_stream, body_size);
        stream_seek(input_stream, body_size);

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
                guac_client_log_info(audio->client,
                        "Accepted format: %i-bit PCM with %i channels at "
                        "%i Hz",
                        bps, channels, rate);

                /* Queue format for sending as accepted */
                stream_check_size(output_stream, 18 + body_size);
                stream_write(output_stream, format_start, 18 + body_size);

                /* 
                 * BEWARE that using stream_check_size means that any "marks"
                 * set via stream_set_mark on output_stream are invalid.
                 */

            }

            /* Otherwise, log that we dropped one */
            else
                guac_client_log_info(audio->client,
                        "Dropped valid format: %i-bit PCM with %i channels at "
                        "%i Hz",
                        bps, channels, rate);

        }

    }

    /* Calculate size of PDU */
    output_body_size = stream_get_length(output_stream) - 4;
    stream_get_mark(output_stream, output_stream_end);

    /* Set body size */
    stream_set_pos(output_stream, 0x02);
    stream_write_uint16(output_stream, output_body_size);

    /* Set format count */
    stream_set_pos(output_stream, 0x12);
    stream_write_uint16(output_stream, rdpsnd->format_count);

    /* Reposition cursor at end (necessary for message send) */
    stream_set_mark(output_stream, output_stream_end);

    /* Send accepted formats */
    pthread_mutex_lock(&(guac_client_data->rdp_lock));
    svc_plugin_send((rdpSvcPlugin*)rdpsnd, output_stream);

    /* If version greater than 6, must send Quality Mode PDU */
    if (server_version >= 6) {

        /* Always send High Quality for now */
        output_stream = stream_new(8);
        stream_write_uint8(output_stream, SNDC_QUALITYMODE);
        stream_write_uint8(output_stream, 0);
        stream_write_uint16(output_stream, 4);
        stream_write_uint16(output_stream, HIGH_QUALITY);
        stream_write_uint16(output_stream, 0);

        svc_plugin_send((rdpSvcPlugin*)rdpsnd, output_stream);
    }

    pthread_mutex_unlock(&(guac_client_data->rdp_lock));

}

/* server is getting a feel of the round trip time */
void guac_rdpsnd_training_handler(guac_rdpsndPlugin* rdpsnd,
        audio_stream* audio, STREAM* input_stream,
        guac_rdpsnd_pdu_header* header) {

    int data_size;
    STREAM* output_stream;

    rdp_guac_client_data* guac_client_data =
        (rdp_guac_client_data*) audio->client->data;

    /* Read timestamp and data size */
    stream_read_uint16(input_stream, rdpsnd->server_timestamp);
    stream_read_uint16(input_stream, data_size);

    /* Send training response */
    output_stream = stream_new(8);
    stream_write_uint8(output_stream, SNDC_TRAINING);
    stream_write_uint8(output_stream, 0);
    stream_write_uint16(output_stream, 4);
    stream_write_uint16(output_stream, rdpsnd->server_timestamp);
    stream_write_uint16(output_stream, data_size);

    pthread_mutex_lock(&(guac_client_data->rdp_lock));
    svc_plugin_send((rdpSvcPlugin*) rdpsnd, output_stream);
    pthread_mutex_unlock(&(guac_client_data->rdp_lock));

}

void guac_rdpsnd_wave_info_handler(guac_rdpsndPlugin* rdpsnd,
        audio_stream* audio, STREAM* input_stream,
        guac_rdpsnd_pdu_header* header) {

    unsigned char buffer[4];
    int format;

    /* Read wave information */
    stream_read_uint16(input_stream, rdpsnd->server_timestamp);
    stream_read_uint16(input_stream, format);
    stream_read_uint8(input_stream, rdpsnd->waveinfo_block_number);
    stream_seek(input_stream, 3);
    stream_read(input_stream, buffer, 4);

    /*
     * Size of incoming wave data is equal to the body size field of this
     * header, less the size of a WaveInfo PDU (not including the header),
     * thus body_size - 12.
     */
    rdpsnd->incoming_wave_size = header->body_size - 12;

    /* Read wave in next iteration */
    rdpsnd->next_pdu_is_wave = true;

    /* Init stream with requested format */
    audio_stream_begin(audio,
            rdpsnd->formats[format].rate,
            rdpsnd->formats[format].channels,
            rdpsnd->formats[format].bps);

    /* Write initial 4 bytes of data */
    audio_stream_write_pcm(audio, buffer, 4);

}

void guac_rdpsnd_wave_handler(guac_rdpsndPlugin* rdpsnd,
        audio_stream* audio, STREAM* input_stream,
        guac_rdpsnd_pdu_header* header) {

    rdpSvcPlugin* plugin = (rdpSvcPlugin*)rdpsnd;

    rdp_guac_client_data* guac_client_data =
        (rdp_guac_client_data*) audio->client->data;

    /* Wave Confirmation PDU */
    STREAM* output_stream = stream_new(8);

    /* Get wave data */
    unsigned char* buffer = stream_get_head(input_stream) + 4;

    /* Write rest of audio packet */
    audio_stream_write_pcm(audio, buffer, rdpsnd->incoming_wave_size);
    audio_stream_end(audio);

    /* Write Wave Confirmation PDU */
    stream_write_uint8(output_stream, SNDC_WAVECONFIRM);
    stream_write_uint8(output_stream, 0);
    stream_write_uint16(output_stream, 4);
    stream_write_uint16(output_stream, rdpsnd->server_timestamp);
    stream_write_uint8(output_stream, rdpsnd->waveinfo_block_number);
    stream_write_uint8(output_stream, 0);

    /* Send Wave Confirmation PDU */
    pthread_mutex_lock(&(guac_client_data->rdp_lock));
    svc_plugin_send(plugin, output_stream);
    pthread_mutex_unlock(&(guac_client_data->rdp_lock));

    /* We no longer expect to receive wave data */
    rdpsnd->next_pdu_is_wave = false;

}

void guac_rdpsnd_close_handler(guac_rdpsndPlugin* rdpsnd,
        audio_stream* audio, STREAM* input_stream,
        guac_rdpsnd_pdu_header* header) {

    /* STUB: Do nothing for now */

}

