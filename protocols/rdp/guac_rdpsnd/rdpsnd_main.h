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

#ifndef __RDPSND_MAIN_H
#define __RDPSND_MAIN_H

#define SNDC_CLOSE         1
#define SNDC_WAVE          2
#define SNDC_SETVOLUME     3
#define SNDC_SETPITCH      4
#define SNDC_WAVECONFIRM   5
#define SNDC_TRAINING      6
#define SNDC_FORMATS       7
#define SNDC_CRYPTKEY      8
#define SNDC_WAVEENCRYPT   9
#define SNDC_UDPWAVE       10
#define SNDC_UDPWAVELAST   11
#define SNDC_QUALITYMODE   12

#define TSSNDCAPS_ALIVE  1
#define TSSNDCAPS_VOLUME 2
#define TSSNDCAPS_PITCH  4

#define DYNAMIC_QUALITY  0x0000
#define MEDIUM_QUALITY   0x0001
#define HIGH_QUALITY     0x0002

#define WAVE_FORMAT_PCM  1

typedef struct rdpsndFormat {
	uint16 wFormatTag;
	uint16 nChannels;
	uint32 nSamplesPerSec;
	uint16 nBlockAlign;
	uint16 wBitsPerSample;
	uint16 cbSize;
	uint8* data;
} rdpsndFormat;


typedef struct guac_rdpsndPlugin {

	rdpSvcPlugin plugin;

	uint8 cBlockNo;
	rdpsndFormat* supported_formats;
	int n_supported_formats;
	int current_format;

	boolean expectingWave;
	uint8 waveData[4];
	uint16 waveDataSize;
	uint32 wTimeStamp; /* server timestamp */
	uint32 wave_timestamp; /* client timestamp */

	boolean is_open;
	uint32 close_timestamp;

	uint16 fixed_format;
	uint16 fixed_channel;
	uint32 fixed_rate;
	int latency;

} guac_rdpsndPlugin ;

void guac_rdpsnd_process_receive(rdpSvcPlugin* plugin,
        STREAM* data_in);

void guac_rdpsnd_process_message_formats(guac_rdpsndPlugin* rdpsnd,
        STREAM* data_in);

void guac_rdpsnd_process_message_training(guac_rdpsndPlugin* rdpsnd,
        STREAM* data_in);

void guac_rdpsnd_process_message_wave_info(guac_rdpsndPlugin* rdpsnd,
        STREAM* data_in, uint16 BodySize);

void rdpsnd_process_message_wave(guac_rdpsndPlugin* rdpsnd,
        STREAM* data_in);

void guac_rdpsnd_process_connect(rdpSvcPlugin* plugin);

void guac_rdpsnd_process_message_setvolume(guac_rdpsndPlugin* rdpsnd,
        STREAM* data_in);

void guac_rdpsnd_process_message_close(guac_rdpsndPlugin* rdpsnd);

void guac_rdpsnd_process_terminate(rdpSvcPlugin* plugin);

void guac_rdpsnd_process_event(rdpSvcPlugin* plugin, RDP_EVENT* event);

#endif /* __RDPSND_MAIN_H */

