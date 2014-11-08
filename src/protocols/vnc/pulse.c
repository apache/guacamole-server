/*
 * Copyright (C) 2013 Glyptodon LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "config.h"

#include "client.h"
#include "pulse.h"

#include <guacamole/audio.h>
#include <guacamole/client.h>
#include <pulse/pulseaudio.h>

static void __stream_read_callback(pa_stream* stream, size_t length,
        void* data) {

    guac_client* client = (guac_client*) data;
    vnc_guac_client_data* client_data = (vnc_guac_client_data*) client->data;
    guac_audio_stream* audio = client_data->audio;

    const void* buffer;

    /* Read data */
    pa_stream_peek(stream, &buffer, &length);

    /* Write data */
    guac_audio_stream_write_pcm(audio, buffer, length);

    /* Flush occasionally */
    if (audio->pcm_bytes_written > GUAC_VNC_PCM_WRITE_RATE) {
        guac_audio_stream_end(audio);
        guac_audio_stream_begin(client_data->audio,
                GUAC_VNC_AUDIO_RATE,
                GUAC_VNC_AUDIO_CHANNELS,
                GUAC_VNC_AUDIO_BPS);
    }

    /* Advance buffer */
    pa_stream_drop(stream);

}

static void __stream_state_callback(pa_stream* stream, void* data) {

    guac_client* client = (guac_client*) data;

    switch (pa_stream_get_state(stream)) {

        case PA_STREAM_UNCONNECTED:
            guac_client_log(client, GUAC_LOG_INFO,
                    "PulseAudio stream currently unconnected");
            break;

        case PA_STREAM_CREATING:
            guac_client_log(client, GUAC_LOG_INFO, "PulseAudio stream being created...");
            break;

        case PA_STREAM_READY:
            guac_client_log(client, GUAC_LOG_INFO, "PulseAudio stream now ready");
            break;

        case PA_STREAM_FAILED:
            guac_client_log(client, GUAC_LOG_INFO, "PulseAudio stream connection failed");
            break;

        case PA_STREAM_TERMINATED:
            guac_client_log(client, GUAC_LOG_INFO, "PulseAudio stream terminated");
            break;

        default:
            guac_client_log(client, GUAC_LOG_INFO,
                    "Unknown PulseAudio stream state: 0x%x",
                    pa_stream_get_state(stream));

    }

}

static void __context_get_sink_info_callback(pa_context* context,
        const pa_sink_info* info, int is_last, void* data) {

    guac_client* client = (guac_client*) data;
    pa_stream* stream;
    pa_sample_spec spec;
    pa_buffer_attr attr;

    /* Stop if end of list reached */
    if (is_last)
        return;

    guac_client_log(client, GUAC_LOG_INFO, "Starting streaming from \"%s\"",
            info->description);

    /* Set format */
    spec.format   = PA_SAMPLE_S16LE;
    spec.rate     = GUAC_VNC_AUDIO_RATE;
    spec.channels = GUAC_VNC_AUDIO_CHANNELS;

    attr.maxlength = -1;
    attr.fragsize  = GUAC_VNC_AUDIO_FRAGMENT_SIZE;

    /* Create stream */
    stream = pa_stream_new(context, "Guacamole Audio", &spec, NULL);

    /* Set stream callbacks */
    pa_stream_set_state_callback(stream, __stream_state_callback, client);
    pa_stream_set_read_callback(stream, __stream_read_callback, client);

    /* Start stream */
    pa_stream_connect_record(stream, info->monitor_source_name, &attr,
                PA_STREAM_DONT_INHIBIT_AUTO_SUSPEND
              | PA_STREAM_ADJUST_LATENCY);

}

static void __context_get_server_info_callback(pa_context* context,
        const pa_server_info* info, void* data) {

    guac_client* client = (guac_client*) data;

    /* If no default sink, cannot continue */
    if (info->default_sink_name == NULL) {
        guac_client_log(client, GUAC_LOG_ERROR, "No default sink. Cannot stream audio.");
        return;
    }

    guac_client_log(client, GUAC_LOG_INFO, "Will use default sink: \"%s\"",
            info->default_sink_name);

    /* Wait for default sink information */
    pa_operation_unref(
            pa_context_get_sink_info_by_name(context,
                info->default_sink_name, __context_get_sink_info_callback,
                client));

}

static void __context_state_callback(pa_context* context, void* data) {

    guac_client* client = (guac_client*) data;

    switch (pa_context_get_state(context)) {

        case PA_CONTEXT_UNCONNECTED:
            guac_client_log(client, GUAC_LOG_INFO,
                    "PulseAudio reports it is unconnected");
            break;

        case PA_CONTEXT_CONNECTING:
            guac_client_log(client, GUAC_LOG_INFO, "Connecting to PulseAudio...");
            break;

        case PA_CONTEXT_AUTHORIZING:
            guac_client_log(client, GUAC_LOG_INFO,
                    "Authorizing PulseAudio connection...");
            break;

        case PA_CONTEXT_SETTING_NAME:
            guac_client_log(client, GUAC_LOG_INFO, "Sending client name...");
            break;

        case PA_CONTEXT_READY:
            guac_client_log(client, GUAC_LOG_INFO, "PulseAudio now ready");
            pa_operation_unref(pa_context_get_server_info(context,
                        __context_get_server_info_callback, client));
            break;

        case PA_CONTEXT_FAILED:
            guac_client_log(client, GUAC_LOG_INFO, "PulseAudio connection failed");
            break;

        case PA_CONTEXT_TERMINATED:
            guac_client_log(client, GUAC_LOG_INFO, "PulseAudio connection terminated");
            break;

        default:
            guac_client_log(client, GUAC_LOG_INFO,
                    "Unknown PulseAudio context state: 0x%x",
                    pa_context_get_state(context));

    }

}

void guac_pa_start_stream(guac_client* client) {

    vnc_guac_client_data* client_data = (vnc_guac_client_data*) client->data;
    pa_context* context;

    guac_client_log(client, GUAC_LOG_INFO, "Starting audio stream");
    guac_audio_stream_begin(client_data->audio,
                GUAC_VNC_AUDIO_RATE,
                GUAC_VNC_AUDIO_CHANNELS,
                GUAC_VNC_AUDIO_BPS);

    /* Init main loop */
    client_data->pa_mainloop = pa_threaded_mainloop_new();

    /* Create context */
    context = pa_context_new(
            pa_threaded_mainloop_get_api(client_data->pa_mainloop),
            "Guacamole Audio");

    /* Set up context */
    pa_context_set_state_callback(context, __context_state_callback, client);
    pa_context_connect(context, client_data->pa_servername,
            PA_CONTEXT_NOAUTOSPAWN, NULL);

    /* Start loop */
    pa_threaded_mainloop_start(client_data->pa_mainloop);

}

void guac_pa_stop_stream(guac_client* client) {

    vnc_guac_client_data* client_data = (vnc_guac_client_data*) client->data;

    /* Stop loop */
    pa_threaded_mainloop_stop(client_data->pa_mainloop);

    guac_client_log(client, GUAC_LOG_INFO, "Audio stream finished");

}

