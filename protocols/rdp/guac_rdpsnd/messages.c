/**
 * FreeRDP: A Remote Desktop Protocol client.
 * Audio Output Virtual Channel
 *
 * Copyright 2009-2011 Jay Sorg
 * Copyright 2010-2011 Vic Lee
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <freerdp/constants.h>
#include <freerdp/types.h>
#include <freerdp/utils/memory.h>
#include <freerdp/utils/stream.h>
#include <freerdp/utils/list.h>
#include <freerdp/utils/load_plugin.h>
#include <freerdp/utils/svc_plugin.h>

#include <guacamole/client.h>

#include "audio.h"
#include "service.h"
#include "messages.h"

/* MESSAGE HANDLERS */

/* receives a list of server supported formats and returns a list
   of client supported formats */
void guac_rdpsnd_process_message_formats(guac_rdpsndPlugin* rdpsnd,
        audio_stream* audio, STREAM* input_stream) {

	uint16 wNumberOfFormats;
	uint16 nFormat;
	uint16 wVersion;
	STREAM* output_stream;
	rdpsndFormat* out_formats;
	uint16 n_out_formats;
	rdpsndFormat* format;
	uint8* format_mark;
	uint8* data_mark;
	int pos;

	stream_seek_uint32(input_stream); /* dwFlags */
	stream_seek_uint32(input_stream); /* dwVolume */
	stream_seek_uint32(input_stream); /* dwPitch */
	stream_seek_uint16(input_stream); /* wDGramPort */
	stream_read_uint16(input_stream, wNumberOfFormats);
	stream_read_uint8(input_stream, rdpsnd->cBlockNo); /* cLastBlockConfirmed */
	stream_read_uint16(input_stream, wVersion);
	stream_seek_uint8(input_stream); /* bPad */

	out_formats = (rdpsndFormat*)
        xzalloc(wNumberOfFormats * sizeof(rdpsndFormat));

	n_out_formats = 0;

	output_stream = stream_new(24);
	stream_write_uint8(output_stream, SNDC_FORMATS); /* msgType */
	stream_write_uint8(output_stream, 0); /* bPad */
	stream_seek_uint16(output_stream); /* BodySize */
	stream_write_uint32(output_stream, TSSNDCAPS_ALIVE); /* dwFlags */
	stream_write_uint32(output_stream, 0); /* dwVolume */
	stream_write_uint32(output_stream, 0); /* dwPitch */
	stream_write_uint16_be(output_stream, 0); /* wDGramPort */
	stream_seek_uint16(output_stream); /* wNumberOfFormats */
	stream_write_uint8(output_stream, 0); /* cLastBlockConfirmed */
	stream_write_uint16(output_stream, 6); /* wVersion */
	stream_write_uint8(output_stream, 0); /* bPad */

	for (nFormat = 0; nFormat < wNumberOfFormats; nFormat++) {

		stream_get_mark(input_stream, format_mark);
		format = &out_formats[n_out_formats];
		stream_read_uint16(input_stream, format->wFormatTag);
		stream_read_uint16(input_stream, format->nChannels);
		stream_read_uint32(input_stream, format->nSamplesPerSec);
		stream_seek_uint32(input_stream); /* nAvgBytesPerSec */
		stream_read_uint16(input_stream, format->nBlockAlign);
		stream_read_uint16(input_stream, format->wBitsPerSample);
		stream_read_uint16(input_stream, format->cbSize);
		stream_get_mark(input_stream, data_mark);
		stream_seek(input_stream, format->cbSize);
		format->data = NULL;

		if (format->wFormatTag == WAVE_FORMAT_PCM) {

            /* If can fit another format, accept it */
            if (rdpsnd->format_count < GUAC_RDP_MAX_FORMATS) {

                /* Add channel */
                int current = rdpsnd->format_count++;
                rdpsnd->formats[current].rate     = format->nSamplesPerSec;
                rdpsnd->formats[current].channels = format->nChannels;
                rdpsnd->formats[current].bps      = format->wBitsPerSample;

                /* Log format */
                guac_client_log_info(audio->client,
                        "Accepted format: %i-bit PCM with %i channels at "
                        "%i Hz",
                        format->wBitsPerSample,
                        format->nChannels,
                        format->nSamplesPerSec);

                /* Store as accepted for future response */
                stream_check_size(output_stream, 18 + format->cbSize);
                stream_write(output_stream, format_mark, 18 + format->cbSize);
                if (format->cbSize > 0) {
                    format->data = xmalloc(format->cbSize);
                    memcpy(format->data, data_mark, format->cbSize);
                }
                n_out_formats++;

            }

            /* Otherwise, log that we dropped one */
            else
                guac_client_log_info(audio->client,
                        "Dropped valid format: %i-bit PCM with %i channels at "
                        "%i Hz",
                        format->wBitsPerSample,
                        format->nChannels,
                        format->nSamplesPerSec);

		}

	}

    xfree(out_formats);

	pos = stream_get_pos(output_stream);
	stream_set_pos(output_stream, 2);
	stream_write_uint16(output_stream, pos - 4);
	stream_set_pos(output_stream, 18);
	stream_write_uint16(output_stream, n_out_formats);
	stream_set_pos(output_stream, pos);

	svc_plugin_send((rdpSvcPlugin*)rdpsnd, output_stream);

	if (wVersion >= 6) {

        /* Respond with guality mode */
		output_stream = stream_new(8);
		stream_write_uint8(output_stream, SNDC_QUALITYMODE); /* msgType */
		stream_write_uint8(output_stream, 0);                /* bPad */
		stream_write_uint16(output_stream, 4);               /* BodySize */
		stream_write_uint16(output_stream, HIGH_QUALITY);    /* wQualityMode */
		stream_write_uint16(output_stream, 0);               /* Reserved */

		svc_plugin_send((rdpSvcPlugin*)rdpsnd, output_stream);
	}

}

/* server is getting a feel of the round trip time */
void guac_rdpsnd_process_message_training(guac_rdpsndPlugin* rdpsnd,
        audio_stream* audio, STREAM* input_stream) {

	uint16 wTimeStamp;
	uint16 wPackSize;
	STREAM* output_stream;

    /* Read timestamp */
	stream_read_uint16(input_stream, wTimeStamp);
	stream_read_uint16(input_stream, wPackSize);

    /* Send training response */
	output_stream = stream_new(8);
	stream_write_uint8(output_stream, SNDC_TRAINING); /* msgType */
	stream_write_uint8(output_stream, 0);             /* bPad */
	stream_write_uint16(output_stream, 4);            /* BodySize */
	stream_write_uint16(output_stream, wTimeStamp);
	stream_write_uint16(output_stream, wPackSize);

	svc_plugin_send((rdpSvcPlugin*) rdpsnd, output_stream);

}

void guac_rdpsnd_process_message_wave_info(guac_rdpsndPlugin* rdpsnd, audio_stream* audio, STREAM* input_stream, uint16 BodySize) {

	uint16 wFormatNo;

    /* Read wave information */
	stream_read_uint16(input_stream, rdpsnd->wTimeStamp);
	stream_read_uint16(input_stream, wFormatNo);
	stream_read_uint8(input_stream, rdpsnd->cBlockNo);
	stream_seek(input_stream, 3); /* bPad */
	stream_read(input_stream, rdpsnd->waveData, 4);

    /* Read wave in next iteration */
	rdpsnd->waveDataSize = BodySize - 8;
	rdpsnd->expectingWave = true;

    /* Init stream with requested format */
    audio_stream_begin(audio,
            rdpsnd->formats[wFormatNo].rate,
            rdpsnd->formats[wFormatNo].channels,
            rdpsnd->formats[wFormatNo].bps);

}

/* header is not removed from data in this function */
void rdpsnd_process_message_wave(guac_rdpsndPlugin* rdpsnd,
        audio_stream* audio, STREAM* input_stream) {

	rdpSvcPlugin* plugin = (rdpSvcPlugin*)rdpsnd;

	STREAM* output_stream;

    unsigned char* buffer = stream_get_head(input_stream);

	rdpsnd->expectingWave = 0;
	memcpy(buffer, rdpsnd->waveData, 4);

    /* Write rest of audio packet */
    audio_stream_write_pcm(audio, buffer, rdpsnd->waveDataSize);
    audio_stream_end(audio);

	output_stream = stream_new(8);
	stream_write_uint8(output_stream, SNDC_WAVECONFIRM);
	stream_write_uint8(output_stream, 0);
	stream_write_uint16(output_stream, 4);
	stream_write_uint16(output_stream, rdpsnd->wTimeStamp);
	stream_write_uint8(output_stream, rdpsnd->cBlockNo); /* cConfirmedBlockNo */
	stream_write_uint8(output_stream, 0); /* bPad */

    svc_plugin_send(plugin, output_stream);
	rdpsnd->plugin.interval_ms = 10;
}

void guac_rdpsnd_process_message_setvolume(guac_rdpsndPlugin* rdpsnd,
        audio_stream* audio, STREAM* input_stream) {

    /* Ignored for now */
	uint32 dwVolume;
	stream_read_uint32(input_stream, dwVolume);

}

void guac_rdpsnd_process_message_close(guac_rdpsndPlugin* rdpsnd,
        audio_stream* audio) {
	rdpsnd->plugin.interval_ms = 10;
}

