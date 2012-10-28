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

/* SVC DEFINITION */

DEFINE_SVC_PLUGIN(guac_rdpsnd, "rdpsnd",
	CHANNEL_OPTION_INITIALIZED | CHANNEL_OPTION_ENCRYPT_RDP)

void guac_rdpsnd_process_connect(rdpSvcPlugin* plugin) {

    /* Get audio stream from plugin */
    audio_stream* audio = (audio_stream*)
        plugin->channel_entry_points.pExtendedData;

    /* Log that sound has been loaded */
    guac_client_log_info(audio->client, "guac_rdpsnd connected.");

}

void guac_rdpsnd_process_terminate(rdpSvcPlugin* plugin) {
	xfree(plugin);
}

void guac_rdpsnd_process_event(rdpSvcPlugin* plugin, RDP_EVENT* event) {
	freerdp_event_free(event);
}

void guac_rdpsnd_process_receive(rdpSvcPlugin* plugin,
        STREAM* input_stream) {

	guac_rdpsndPlugin* rdpsnd = (guac_rdpsndPlugin*) plugin;

    /* Get audio stream from plugin */
    audio_stream* audio = (audio_stream*)
        plugin->channel_entry_points.pExtendedData;

	uint8 msgType;
	uint16 BodySize;

	if (rdpsnd->expectingWave) {
        rdpsnd_process_message_wave(rdpsnd, audio, input_stream);
		return;
	}

    /* Read event */
	stream_read_uint8(input_stream, msgType); /* msgType */
	stream_seek_uint8(input_stream);          /* bPad */
	stream_read_uint16(input_stream, BodySize);

	switch (msgType) {

		case SNDC_FORMATS:
			guac_rdpsnd_process_message_formats(rdpsnd, audio, input_stream);
			break;

		case SNDC_TRAINING:
			guac_rdpsnd_process_message_training(rdpsnd, audio, input_stream);
			break;

		case SNDC_WAVE:
			guac_rdpsnd_process_message_wave_info(rdpsnd, audio, input_stream, BodySize);
			break;

		case SNDC_CLOSE:
			guac_rdpsnd_process_message_close(rdpsnd, audio);
			break;

		case SNDC_SETVOLUME:
			guac_rdpsnd_process_message_setvolume(rdpsnd, audio, input_stream);
			break;

		default:
			/*DEBUG_WARN("unknown msgType %d", msgType);*/
			break;
	}

}

