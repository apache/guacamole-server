
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


/* Define service, associate with "rdpsnd" channel */

DEFINE_SVC_PLUGIN(guac_rdpsnd, "rdpsnd",
    CHANNEL_OPTION_INITIALIZED | CHANNEL_OPTION_ENCRYPT_RDP)


/* 
 * Service Handlers
 */

void guac_rdpsnd_process_connect(rdpSvcPlugin* plugin) {

    /* Get audio stream from plugin */
    audio_stream* audio = (audio_stream*)
        plugin->channel_entry_points.pExtendedData;

    /* Update every 10 ms */
    plugin->interval_ms = 10;

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
    guac_rdpsnd_pdu_header header;

    /* Get audio stream from plugin */
    audio_stream* audio = (audio_stream*)
        plugin->channel_entry_points.pExtendedData;

    /* Read RDPSND PDU header */
    stream_read_uint8(input_stream, header.message_type);
    stream_seek_uint8(input_stream);
    stream_read_uint16(input_stream, header.body_size);

    /* 
     * If next PDU is SNDWAVE (due to receiving WaveInfo PDU previously),
     * ignore the header and parse as a Wave PDU.
     */
    if (rdpsnd->next_pdu_is_wave) {
        guac_rdpsnd_wave_handler(rdpsnd, audio, input_stream, &header);
        return;
    }

    /* Dispatch message to standard handlers */
    switch (header.message_type) {

        /* Server Audio Formats and Version PDU */
        case SNDC_FORMATS:
            guac_rdpsnd_formats_handler(rdpsnd, audio,
                    input_stream, &header);
            break;

        /* Training PDU */
        case SNDC_TRAINING:
            guac_rdpsnd_training_handler(rdpsnd, audio,
                    input_stream, &header);
            break;

        /* WaveInfo PDU */
        case SNDC_WAVE:
            guac_rdpsnd_wave_info_handler(rdpsnd, audio,
                    input_stream, &header);
            break;

        /* Close PDU */
        case SNDC_CLOSE:
            guac_rdpsnd_close_handler(rdpsnd, audio,
                    input_stream, &header);
            break;

    }

}

