/**
 * FreeRDP: A Remote Desktop Protocol client.
 * Audio Output Virtual Channel
 *
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

#ifndef __GUAC_RDPSND_SERVICE_H
#define __GUAC_RDPSND_SERVICE_H


#define GUAC_RDP_MAX_FORMATS 16


typedef struct guac_pcm_format {

    int rate;

    int channels;

    int bps;

} guac_pcm_format;


typedef struct guac_rdpsndPlugin {

	rdpSvcPlugin plugin;

	uint8 cBlockNo;

	boolean expectingWave;
	uint8 waveData[4];
	uint16 waveDataSize;
	uint32 wTimeStamp; /* server timestamp */

    guac_pcm_format formats[GUAC_RDP_MAX_FORMATS];

    int format_count;

} guac_rdpsndPlugin;

void guac_rdpsnd_process_connect(rdpSvcPlugin* plugin);

void guac_rdpsnd_process_receive(rdpSvcPlugin* plugin,
        STREAM* data_in);

void guac_rdpsnd_process_terminate(rdpSvcPlugin* plugin);

void guac_rdpsnd_process_event(rdpSvcPlugin* plugin, RDP_EVENT* event);

#endif

