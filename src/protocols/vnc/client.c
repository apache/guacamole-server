
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

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <rfb/rfbclient.h>

#include <guacamole/socket.h>
#include <guacamole/protocol.h>
#include <guacamole/client.h>
#include <guacamole/audio.h>

#include "client.h"
#include "vnc_handlers.h"
#include "guac_handlers.h"

#ifdef ENABLE_PULSE
#include "pulse.h"
#endif

/* Client plugin arguments */
const char* GUAC_CLIENT_ARGS[] = {
    "hostname",
    "port",
    "read-only",
    "encodings",
    "password",
    "swap-red-blue",
    "color-depth",

#ifdef ENABLE_VNC_REPEATER
    "dest-host",
    "dest-port",
#endif

#ifdef ENABLE_PULSE
    "disable-audio",
#endif

    NULL
};

enum VNC_ARGS_IDX {

    IDX_HOSTNAME,
    IDX_PORT,
    IDX_READ_ONLY,
    IDX_ENCODINGS,
    IDX_PASSWORD,
    IDX_SWAP_RED_BLUE,
    IDX_COLOR_DEPTH,

#ifdef ENABLE_VNC_REPEATER
    IDX_DEST_HOST,
    IDX_DEST_PORT,
#endif

#ifdef ENABLE_PULSE
    IDX_DISABLE_AUDIO,
#endif

    VNC_ARGS_COUNT
};


char* __GUAC_CLIENT = "GUAC_CLIENT";

int guac_client_init(guac_client* client, int argc, char** argv) {

    rfbClient* rfb_client;

    vnc_guac_client_data* guac_client_data;

#ifdef ENABLE_PULSE    
    pthread_t pa_read_thread;
#endif

    int read_only;

    /* Set up libvncclient logging */
    rfbClientLog = guac_vnc_client_log_info;
    rfbClientErr = guac_vnc_client_log_error;

    /*** PARSE ARGUMENTS ***/

    if (argc != VNC_ARGS_COUNT) {
        guac_protocol_send_error(client->socket, "Wrong argument count received.");
        guac_socket_flush(client->socket);
        return 1;
    }

    /* Alloc client data */
    guac_client_data = malloc(sizeof(vnc_guac_client_data));
    client->data = guac_client_data;

    /* Set read-only flag */
    read_only = (strcmp(argv[IDX_READ_ONLY], "true") == 0);

    /* Set red/blue swap flag */
    guac_client_data->swap_red_blue = (strcmp(argv[IDX_SWAP_RED_BLUE], "true") == 0);

    /* Freed after use by libvncclient */
    guac_client_data->password = strdup(argv[IDX_PASSWORD]);

    /*** INIT RFB CLIENT ***/

    rfb_client = rfbGetClient(8, 3, 4); /* 32-bpp client */

    /* Store Guac client in rfb client */
    rfbClientSetClientData(rfb_client, __GUAC_CLIENT, client);

    /* Framebuffer update handler */
    rfb_client->GotFrameBufferUpdate = guac_vnc_update;
    rfb_client->GotCopyRect = guac_vnc_copyrect;

    /* Do not handle clipboard and local cursor if read-only */
    if (read_only == 0) {
        /* Enable client-side cursor */
        rfb_client->GotCursorShape = guac_vnc_cursor;
        rfb_client->appData.useRemoteCursor = TRUE;

        /* Clipboard */
        rfb_client->GotXCutText = guac_vnc_cut_text;
    }

    /* Password */
    rfb_client->GetPassword = guac_vnc_get_password;

    /* Depth */
    guac_vnc_set_pixel_format(rfb_client, atoi(argv[IDX_COLOR_DEPTH]));

#ifdef ENABLE_PULSE
    guac_client_data->audio_enabled = (strcmp(argv[IDX_DISABLE_AUDIO], "true") != 0);

    /* If an encoding is available, load an audio stream */
    if (guac_client_data->audio_enabled) {    

        guac_client_data->audio = guac_audio_stream_alloc(client, NULL);

        /* If successful, init audio system */
        if (guac_client_data->audio != NULL) {
            
            guac_client_log_info(client,
                    "Audio will be encoded as %s",
                    guac_client_data->audio->encoder->mimetype);

            /* Create a thread to read audio data */
            if (pthread_create(&pa_read_thread, NULL, guac_pa_read_audio,
                        (void*) client)) {
                guac_protocol_send_error(client->socket,
                        "Error initializing PulseAudio read thread");
                guac_socket_flush(client->socket);
                return 1;
            }
            
            guac_client_data->audio_read_thread = &pa_read_thread;
            
        }

        /* Otherwise, audio loading failed */
        else
            guac_client_log_info(client,
                    "No available audio encoding. Sound disabled.");

        /* Require threadsafe sockets if audio enabled */
        guac_socket_require_threadsafe(client->socket);

    } /* end if audio enabled */
#endif

    /* Hook into allocation so we can handle resize. */
    guac_client_data->rfb_MallocFrameBuffer = rfb_client->MallocFrameBuffer;
    rfb_client->MallocFrameBuffer = guac_vnc_malloc_framebuffer;
    rfb_client->canHandleNewFBSize = 1;

    /* Set hostname and port */
    rfb_client->serverHost = strdup(argv[0]);
    rfb_client->serverPort = atoi(argv[1]);

#ifdef ENABLE_VNC_REPEATER
    /* Set repeater parameters if specified */
    if(argv[IDX_DEST_HOST][0] != '\0')
        rfb_client->destHost = strdup(argv[IDX_DEST_HOST]);

    if(argv[IDX_DEST_PORT][0] != '\0')
        rfb_client->destPort = atoi(argv[IDX_DEST_PORT]);
#endif

    /* Set encodings if specified */
    if (argv[IDX_ENCODINGS][0] != '\0')
        rfb_client->appData.encodingsString = guac_client_data->encodings
            = strdup(argv[IDX_ENCODINGS]);
    else
        guac_client_data->encodings = NULL;

    /* Connect */
    if (!rfbInitClient(rfb_client, NULL, NULL)) {
        guac_protocol_send_error(client->socket, "Error initializing VNC client");
        guac_socket_flush(client->socket);
        return 1;
    }

    /* Set remaining client data */
    guac_client_data->rfb_client = rfb_client;
    guac_client_data->copy_rect_used = 0;
    guac_client_data->cursor = guac_client_alloc_buffer(client);

    /* Set handlers */
    client->handle_messages = vnc_guac_client_handle_messages;
    client->free_handler = vnc_guac_client_free_handler;
    if (read_only == 0) {
        /* Do not handle mouse/keyboard/clipboard if read-only */
        client->mouse_handler = vnc_guac_client_mouse_handler;
        client->key_handler = vnc_guac_client_key_handler;
        client->clipboard_handler = vnc_guac_client_clipboard_handler;
    }

    /* Send name */
    guac_protocol_send_name(client->socket, rfb_client->desktopName);

    /* Send size */
    guac_protocol_send_size(client->socket,
            GUAC_DEFAULT_LAYER, rfb_client->width, rfb_client->height);

    return 0;

}

