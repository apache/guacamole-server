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

#include "rdpsnd_main.h"

void guac_rdpsnd_process_receive(rdpSvcPlugin* plugin,
        STREAM* data_in) {

	guac_rdpsndPlugin* rdpsnd = (guac_rdpsndPlugin*) plugin;

	uint8 msgType;
	uint16 BodySize;

	if (rdpsnd->expectingWave) {
        rdpsnd_process_message_wave(rdpsnd, data_in);
		return;
	}

    /* Read event */
	stream_read_uint8(data_in, msgType); /* msgType */
	stream_seek_uint8(data_in);          /* bPad */
	stream_read_uint16(data_in, BodySize);

	switch (msgType) {

		case SNDC_FORMATS:
			guac_rdpsnd_process_message_formats(rdpsnd, data_in);
			break;

		case SNDC_TRAINING:
			guac_rdpsnd_process_message_training(rdpsnd, data_in);
			break;

		case SNDC_WAVE:
			guac_rdpsnd_process_message_wave_info(rdpsnd, data_in, BodySize);
			break;

		case SNDC_CLOSE:
			guac_rdpsnd_process_message_close(rdpsnd);
			break;

		case SNDC_SETVOLUME:
			guac_rdpsnd_process_message_setvolume(rdpsnd, data_in);
			break;

		default:
			/*DEBUG_WARN("unknown msgType %d", msgType);*/
			break;
	}

}


/* receives a list of server supported formats and returns a list
   of client supported formats */
void guac_rdpsnd_process_message_formats(guac_rdpsndPlugin* rdpsnd, STREAM* data_in) {

	rdpSvcPlugin* plugin = (rdpSvcPlugin*)rdpsnd;

    /* Get client from plugin */
    guac_client* client = (guac_client*)
        plugin->channel_entry_points.pExtendedData;

	uint16 wNumberOfFormats;
	uint16 nFormat;
	uint16 wVersion;
	STREAM* data_out;
	rdpsndFormat* out_formats;
	uint16 n_out_formats;
	rdpsndFormat* format;
	uint8* format_mark;
	uint8* data_mark;
	int pos;

	stream_seek_uint32(data_in); /* dwFlags */
	stream_seek_uint32(data_in); /* dwVolume */
	stream_seek_uint32(data_in); /* dwPitch */
	stream_seek_uint16(data_in); /* wDGramPort */
	stream_read_uint16(data_in, wNumberOfFormats);
	stream_read_uint8(data_in, rdpsnd->cBlockNo); /* cLastBlockConfirmed */
	stream_read_uint16(data_in, wVersion);
	stream_seek_uint8(data_in); /* bPad */

	out_formats = (rdpsndFormat*)
        xzalloc(wNumberOfFormats * sizeof(rdpsndFormat));

	n_out_formats = 0;

	data_out = stream_new(24);
	stream_write_uint8(data_out, SNDC_FORMATS); /* msgType */
	stream_write_uint8(data_out, 0); /* bPad */
	stream_seek_uint16(data_out); /* BodySize */
	stream_write_uint32(data_out, TSSNDCAPS_ALIVE); /* dwFlags */
	stream_write_uint32(data_out, 0); /* dwVolume */
	stream_write_uint32(data_out, 0); /* dwPitch */
	stream_write_uint16_be(data_out, 0); /* wDGramPort */
	stream_seek_uint16(data_out); /* wNumberOfFormats */
	stream_write_uint8(data_out, 0); /* cLastBlockConfirmed */
	stream_write_uint16(data_out, 6); /* wVersion */
	stream_write_uint8(data_out, 0); /* bPad */

	for (nFormat = 0; nFormat < wNumberOfFormats; nFormat++) {

		stream_get_mark(data_in, format_mark);
		format = &out_formats[n_out_formats];
		stream_read_uint16(data_in, format->wFormatTag);
		stream_read_uint16(data_in, format->nChannels);
		stream_read_uint32(data_in, format->nSamplesPerSec);
		stream_seek_uint32(data_in); /* nAvgBytesPerSec */
		stream_read_uint16(data_in, format->nBlockAlign);
		stream_read_uint16(data_in, format->wBitsPerSample);
		stream_read_uint16(data_in, format->cbSize);
		stream_get_mark(data_in, data_mark);
		stream_seek(data_in, format->cbSize);
		format->data = NULL;

		if (format->wFormatTag == WAVE_FORMAT_PCM) {

            guac_client_log_info(client,
                    "Accepted format: %i-bit PCM with %i channels at "
                    "%i Hz",
                    format->wBitsPerSample,
                    format->nChannels,
                    format->nSamplesPerSec);

			stream_check_size(data_out, 18 + format->cbSize);
			stream_write(data_out, format_mark, 18 + format->cbSize);
			if (format->cbSize > 0)
			{
				format->data = xmalloc(format->cbSize);
				memcpy(format->data, data_mark, format->cbSize);
			}
			n_out_formats++;
		}

	}

    xfree(out_formats);

	pos = stream_get_pos(data_out);
	stream_set_pos(data_out, 2);
	stream_write_uint16(data_out, pos - 4);
	stream_set_pos(data_out, 18);
	stream_write_uint16(data_out, n_out_formats);
	stream_set_pos(data_out, pos);

	svc_plugin_send((rdpSvcPlugin*)rdpsnd, data_out);

	if (wVersion >= 6) {

        /* Respond with guality mode */
		data_out = stream_new(8);
		stream_write_uint8(data_out, SNDC_QUALITYMODE); /* msgType */
		stream_write_uint8(data_out, 0);                /* bPad */
		stream_write_uint16(data_out, 4);               /* BodySize */
		stream_write_uint16(data_out, HIGH_QUALITY);    /* wQualityMode */
		stream_write_uint16(data_out, 0);               /* Reserved */

		svc_plugin_send((rdpSvcPlugin*)rdpsnd, data_out);
	}

}

/* server is getting a feel of the round trip time */
void guac_rdpsnd_process_message_training(guac_rdpsndPlugin* rdpsnd,
        STREAM* data_in) {

	uint16 wTimeStamp;
	uint16 wPackSize;
	STREAM* data_out;

    /* Read timestamp */
	stream_read_uint16(data_in, wTimeStamp);
	stream_read_uint16(data_in, wPackSize);

    /* Send training response */
	data_out = stream_new(8);
	stream_write_uint8(data_out, SNDC_TRAINING); /* msgType */
	stream_write_uint8(data_out, 0);             /* bPad */
	stream_write_uint16(data_out, 4);            /* BodySize */
	stream_write_uint16(data_out, wTimeStamp);
	stream_write_uint16(data_out, wPackSize);

	svc_plugin_send((rdpSvcPlugin*) rdpsnd, data_out);

}

void guac_rdpsnd_process_message_wave_info(guac_rdpsndPlugin* rdpsnd, STREAM* data_in, uint16 BodySize) {

	uint16 wFormatNo;

    /* Read wave information */
	stream_read_uint16(data_in, rdpsnd->wTimeStamp);
	stream_read_uint16(data_in, wFormatNo);
	stream_read_uint8(data_in, rdpsnd->cBlockNo);
	stream_seek(data_in, 3); /* bPad */
	stream_read(data_in, rdpsnd->waveData, 4);

    /* Read wave in next iteration */
	rdpsnd->waveDataSize = BodySize - 8;
	rdpsnd->expectingWave = true;

}

/* header is not removed from data in this function */
void rdpsnd_process_message_wave(guac_rdpsndPlugin* rdpsnd,
        STREAM* data_in) {

	rdpSvcPlugin* plugin = (rdpSvcPlugin*)rdpsnd;

    /* Get client from plugin */
    guac_client* client = (guac_client*)
        plugin->channel_entry_points.pExtendedData;

	STREAM* data_out;

    int size;
    unsigned char* buffer;

	rdpsnd->expectingWave = 0;
	memcpy(stream_get_head(data_in), rdpsnd->waveData, 4);
	if (stream_get_size(data_in) != rdpsnd->waveDataSize) {
		return;
	}

    buffer = stream_get_head(data_in);
    size = stream_get_size(data_in);

    guac_client_log_info(client, "Got sound: %i bytes.", size);

	data_out = stream_new(8);
	stream_write_uint8(data_out, SNDC_WAVECONFIRM);
	stream_write_uint8(data_out, 0);
	stream_write_uint16(data_out, 4);
	stream_write_uint16(data_out, rdpsnd->wTimeStamp);
	stream_write_uint8(data_out, rdpsnd->cBlockNo); /* cConfirmedBlockNo */
	stream_write_uint8(data_out, 0); /* bPad */

    svc_plugin_send(plugin, data_out);
	rdpsnd->plugin.interval_ms = 10;
}

void guac_rdpsnd_process_connect(rdpSvcPlugin* plugin) {

    /* Get client from plugin */
    guac_client* client = (guac_client*)
        plugin->channel_entry_points.pExtendedData;

    /* Log that sound has been loaded */
    guac_client_log_info(client, "guac_rdpsnd connected.");

}

void guac_rdpsnd_process_message_setvolume(guac_rdpsndPlugin* rdpsnd,
        STREAM* data_in) {

    /* Ignored for now */
	uint32 dwVolume;
	stream_read_uint32(data_in, dwVolume);

}

void guac_rdpsnd_process_message_close(guac_rdpsndPlugin* rdpsnd) {
	rdpsnd->plugin.interval_ms = 10;
}

void guac_rdpsnd_process_terminate(rdpSvcPlugin* plugin) {
	xfree(plugin);
}

void guac_rdpsnd_process_event(rdpSvcPlugin* plugin, RDP_EVENT* event) {
	freerdp_event_free(event);
}

DEFINE_SVC_PLUGIN(guac_rdpsnd, "rdpsnd",
	CHANNEL_OPTION_INITIALIZED | CHANNEL_OPTION_ENCRYPT_RDP)

