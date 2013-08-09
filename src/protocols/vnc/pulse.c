
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

#include <pulse/simple.h>
#include <pulse/error.h>

void* guac_pa_read_audio(void* data) {

    pa_sample_spec spec;
    pa_simple* stream;
    int error;

    /* Get client */
    guac_client* client = (guac_client*) data;

    /* Init sample spec */
    spec.format   = PA_SAMPLE_S16LE;
    spec.rate     = 44100;
    spec.channels = 2;

    /* Create new stream */
    stream = pa_simple_new(NULL,    /* Name of server */
            "Guacamole",            /* Name of client */
            PA_STREAM_RECORD, NULL, /* Direction and device */
            "Audio",                /* Name of stream */
            &spec, NULL, NULL,      /* Stream options */
            &error);

    /* Fail on error */
    if (stream == NULL) {
        guac_client_log_error(client, "Unable to connect to PulseAudio: %s",
                pa_strerror(error));
        return NULL;
    }

    /* Start streaming */
    guac_client_log_info(client, "Streaming audio from PulseAudio");

    while (client->state == GUAC_CLIENT_RUNNING) {

        uint8_t buffer[1024];

        guac_client_log_info(client, "Reading...");

        /* Read packet of audio data */
        if (pa_simple_read(stream, buffer, sizeof(buffer), &error) < 0) {
            guac_client_log_error(client, "Unable to read from PulseAudio: %s",
                    pa_strerror(error));
            return NULL;
        }

        /* STUB: Write data */
        guac_client_log_info(client, "Would write %i", sizeof(buffer));

    }

    /* Close stream */
    guac_client_log_info(client, "Streaming from PulseAudio finished");
    pa_simple_free(stream);

    return NULL;

}

