
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
 * The Original Code is libguac-client-vnc.
 *
 * The Initial Developer of the Original Code is
 * Michael Jumper.
 * Portions created by the Initial Developer are Copyright (C) 2010
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

#include <guacamole/client.h>

#include <pulse/pulseaudio.h>

#include "client.h"

static void __stream_read_callback(pa_stream* stream, size_t length,
        void* data) {

    guac_client* client = (guac_client*) data;
    const void* buffer;

    /* Read data */
    pa_stream_peek(stream, &buffer, &length);

    guac_client_log_info(client, "STUB: Received %i bytes", length);

    /* Advance buffer */
    pa_stream_drop(stream);

}

static void __stream_state_callback(pa_stream* stream, void* data) {

    guac_client* client = (guac_client*) data;

    switch (pa_stream_get_state(stream)) {

        case PA_STREAM_UNCONNECTED:
            guac_client_log_info(client,
                    "PulseAudio stream currently unconnected");
            break;

        case PA_STREAM_CREATING:
            guac_client_log_info(client, "PulseAudio stream being created...");
            break;

        case PA_STREAM_READY:
            guac_client_log_info(client, "PulseAudio stream now ready");
            break;

        case PA_STREAM_FAILED:
            guac_client_log_info(client, "PulseAudio stream connection failed");
            break;

        case PA_STREAM_TERMINATED:
            guac_client_log_info(client, "PulseAudio stream terminated");
            break;

        default:
            guac_client_log_info(client,
                    "Unknown PulseAudio stream state: 0x%x",
                    pa_stream_get_state(stream));

    }

}

static void __context_get_sink_info_callback(pa_context* context,
        const pa_sink_info* info, int is_last, void* data) {

    guac_client* client = (guac_client*) data;
    pa_stream* stream;
    pa_sample_spec spec;

    /* Stop if end of list reached */
    if (is_last)
        return;

    guac_client_log_info(client, "Starting streaming from \"%s\"",
            info->description);

    /* Set format */
    spec.format   = PA_SAMPLE_S16LE;
    spec.rate     = info->sample_spec.rate;
    spec.channels = info->sample_spec.channels;

    /* Create stream */
    stream = pa_stream_new(context, "Guacamole Audio", &spec,
            &(info->channel_map));

    /* Set stream callbacks */
    pa_stream_set_state_callback(stream, __stream_state_callback, client);
    pa_stream_set_read_callback(stream, __stream_read_callback, client);

    /* Start stream */
    pa_stream_connect_record(stream, info->monitor_source_name, NULL,
            0 /* FIXME: PA_STREAM_DONT_INHIBIT_AUTO_SUSPEND */);

}

static void __context_get_server_info_callback(pa_context* context,
        const pa_server_info* info, void* data) {

    guac_client* client = (guac_client*) data;

    /* If no default sink, cannot continue */
    if (info->default_sink_name == NULL) {
        guac_client_log_error(client, "No default sink. Cannot stream audio.");
        return;
    }

    guac_client_log_info(client, "Will use default sink: \"%s\"",
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
            guac_client_log_info(client,
                    "PulseAudio reports it is unconnected");
            break;

        case PA_CONTEXT_CONNECTING:
            guac_client_log_info(client, "Connecting to PulseAudio...");
            break;

        case PA_CONTEXT_AUTHORIZING:
            guac_client_log_info(client,
                    "Authorizing PulseAudio connection...");
            break;

        case PA_CONTEXT_SETTING_NAME:
            guac_client_log_info(client, "Sending client name...");
            break;

        case PA_CONTEXT_READY:
            guac_client_log_info(client, "PulseAudio now ready");
            pa_operation_unref(pa_context_get_server_info(context,
                        __context_get_server_info_callback, client));
            break;

        case PA_CONTEXT_FAILED:
            guac_client_log_info(client, "PulseAudio connection failed");
            break;

        case PA_CONTEXT_TERMINATED:
            guac_client_log_info(client, "PulseAudio connection terminated");
            break;

        default:
            guac_client_log_info(client,
                    "Unknown PulseAudio context state: 0x%x",
                    pa_context_get_state(context));

    }

}

void guac_pa_start_stream(guac_client* client) {

    vnc_guac_client_data* client_data = (vnc_guac_client_data*) client->data;
    pa_context* context;

    guac_client_log_info(client, "Starting audio stream");

    /* Init main loop */
    client_data->pa_mainloop = pa_threaded_mainloop_new();

    /* Create context */
    context = pa_context_new(
            pa_threaded_mainloop_get_api(client_data->pa_mainloop),
            "Guacamole Audio");

    /* Set up context */
    pa_context_set_state_callback(context, __context_state_callback, client);
    pa_context_connect(context, NULL, PA_CONTEXT_NOAUTOSPAWN, NULL);

    /* Start loop */
    pa_threaded_mainloop_start(client_data->pa_mainloop);

}

void guac_pa_stop_stream(guac_client* client) {

    vnc_guac_client_data* client_data = (vnc_guac_client_data*) client->data;

    /* Stop loop */
    pa_threaded_mainloop_stop(client_data->pa_mainloop);

    guac_client_log_info(client, "Audio stream finished");

}

