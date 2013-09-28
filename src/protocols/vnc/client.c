
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
 * James Muehlner
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

#include <rfb/rfbclient.h>

#include <guacamole/socket.h>
#include <guacamole/protocol.h>
#include <guacamole/client.h>
#include <guacamole/audio.h>

#include "client.h"
#include "vnc_handlers.h"
#include "guac_handlers.h"
#include "default_pointer.h"

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
    "cursor",
    "autoretry",

#ifdef ENABLE_VNC_REPEATER
    "dest-host",
    "dest-port",
#endif

#ifdef ENABLE_PULSE
    "enable-audio",
    "audio-servername",
#endif

    "reverse-connect",
    "listen-timeout",
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
    IDX_CURSOR,
    IDX_AUTORETRY,

#ifdef ENABLE_VNC_REPEATER
    IDX_DEST_HOST,
    IDX_DEST_PORT,
#endif

#ifdef ENABLE_PULSE
    IDX_ENABLE_AUDIO,
    IDX_AUDIO_SERVERNAME,
#endif

    IDX_REVERSE_CONNECT,
    IDX_LISTEN_TIMEOUT,
    VNC_ARGS_COUNT
};


char* __GUAC_CLIENT = "GUAC_CLIENT";

/**
 * Allocates a new rfbClient instance given the parameters stored within the
 * client, returning NULL on failure.
 */
static rfbClient* __guac_vnc_get_client(guac_client* client) {

    rfbClient* rfb_client = rfbGetClient(8, 3, 4); /* 32-bpp client */
    vnc_guac_client_data* guac_client_data =
        (vnc_guac_client_data*) client->data;

    /* Store Guac client in rfb client */
    rfbClientSetClientData(rfb_client, __GUAC_CLIENT, client);

    /* Framebuffer update handler */
    rfb_client->GotFrameBufferUpdate = guac_vnc_update;
    rfb_client->GotCopyRect = guac_vnc_copyrect;

    /* Do not handle clipboard and local cursor if read-only */
    if (guac_client_data->read_only == 0) {

        /* Clipboard */
        rfb_client->GotXCutText = guac_vnc_cut_text;

        /* Set remote cursor */
        if (guac_client_data->remote_cursor)
            rfb_client->appData.useRemoteCursor = FALSE;

        else {
            /* Enable client-side cursor */
            rfb_client->appData.useRemoteCursor = TRUE;
            rfb_client->GotCursorShape = guac_vnc_cursor;
        }
    }

    /* Password */
    rfb_client->GetPassword = guac_vnc_get_password;

    /* Depth */
    guac_vnc_set_pixel_format(rfb_client, guac_client_data->color_depth);

    /* Hook into allocation so we can handle resize. */
    guac_client_data->rfb_MallocFrameBuffer = rfb_client->MallocFrameBuffer;
    rfb_client->MallocFrameBuffer = guac_vnc_malloc_framebuffer;
    rfb_client->canHandleNewFBSize = 1;

    /* Set hostname and port */
    rfb_client->serverHost = strdup(guac_client_data->hostname);
    rfb_client->serverPort = guac_client_data->port;

#ifdef ENABLE_VNC_REPEATER
    /* Set repeater parameters if specified */
    if (guac_client_data->dest_host) {
        rfb_client->destHost = strdup(guac_client_data->dest_host);
        rfb_client->destPort = guac_client_data->dest_port;
    }
#endif

    /* If reverse connection enabled, start listening */
    if (guac_client_data->reverse_connect) {

        guac_client_log_info(client, "Listening for connections on port %i",
                guac_client_data->port);

        /* Listen for connection from server */
        rfb_client->listenPort = guac_client_data->port;
        if (listenForIncomingConnectionsNoFork(rfb_client,
                    guac_client_data->listen_timeout*1000) <= 0)
            return NULL;

    }

    /* Connect */
    if (rfbInitClient(rfb_client, NULL, NULL))
        return rfb_client;

    /* If connection fails, return NULL */
    return NULL;

}

int guac_client_init(guac_client* client, int argc, char** argv) {

    rfbClient* rfb_client;

    vnc_guac_client_data* guac_client_data;

    int retries_remaining;

    /* Set up libvncclient logging */
    rfbClientLog = guac_vnc_client_log_info;
    rfbClientErr = guac_vnc_client_log_error;

    /*** PARSE ARGUMENTS ***/

    if (argc != VNC_ARGS_COUNT) {
        guac_protocol_send_error(client->socket,
                "Wrong argument count received.",
                GUAC_PROTOCOL_STATUS_INVALID_PARAMETER);
        guac_socket_flush(client->socket);
        return 1;
    }

    /* Alloc client data */
    guac_client_data = malloc(sizeof(vnc_guac_client_data));
    client->data = guac_client_data;

    guac_client_data->hostname = strdup(argv[IDX_HOSTNAME]);
    guac_client_data->port = atoi(argv[IDX_PORT]);

    /* Set remote cursor flag */
    guac_client_data->remote_cursor = (strcmp(argv[IDX_CURSOR], "remote") == 0);

    /* Set red/blue swap flag */
    guac_client_data->swap_red_blue = (strcmp(argv[IDX_SWAP_RED_BLUE], "true") == 0);

    /* Set read-only flag */
    guac_client_data->read_only = (strcmp(argv[IDX_READ_ONLY], "true") == 0);

    /* Freed after use by libvncclient */
    guac_client_data->password = strdup(argv[IDX_PASSWORD]);

    /* Parse color depth */
    guac_client_data->color_depth = atoi(argv[IDX_COLOR_DEPTH]);

#ifdef ENABLE_VNC_REPEATER
    /* Set repeater parameters if specified */
    if (argv[IDX_DEST_HOST][0] != '\0')
        guac_client_data->dest_host = strdup(argv[IDX_DEST_HOST]);
    else
        guac_client_data->dest_host = NULL;

    if (argv[IDX_DEST_PORT][0] != '\0')
        guac_client_data->dest_port = atoi(argv[IDX_DEST_PORT]);
#endif

    /* Set encodings if specified */
    if (argv[IDX_ENCODINGS][0] != '\0')
        guac_client_data->encodings = strdup(argv[IDX_ENCODINGS]);
    else
        guac_client_data->encodings = NULL;

    /* Parse autoretry */
    if (argv[IDX_AUTORETRY][0] != '\0')
        retries_remaining = atoi(argv[IDX_AUTORETRY]);
    else
        retries_remaining = 0; 

    /* Set reverse-connection flag */
    guac_client_data->reverse_connect =
        (strcmp(argv[IDX_REVERSE_CONNECT], "true") == 0);

    /* Parse listen timeout */
    if (argv[IDX_LISTEN_TIMEOUT][0] != '\0')
        guac_client_data->listen_timeout = atoi(argv[IDX_LISTEN_TIMEOUT]);
    else
        guac_client_data->listen_timeout = 5000; 

    /* Ensure connection is kept alive during lengthy connects */
    guac_socket_require_keep_alive(client->socket);

    /* Attempt connection */
    rfb_client = __guac_vnc_get_client(client);

    /* If unsuccessful, retry as many times as specified */
    while (!rfb_client && retries_remaining > 0) {

        guac_client_log_info(client,
                "Connect failed. Waiting %ims before retrying...",
                GUAC_VNC_CONNECT_INTERVAL);

        /* Wait for given interval then retry */
        usleep(GUAC_VNC_CONNECT_INTERVAL*1000);
        rfb_client = __guac_vnc_get_client(client);
        retries_remaining--;

    }

    /* If the final connect attempt fails, return error */
    if (!rfb_client) {
        guac_protocol_send_error(client->socket,
                "Error initializing VNC client",
                GUAC_PROTOCOL_STATUS_INTERNAL_ERROR);
        guac_socket_flush(client->socket);
        return 1;
    }

#ifdef ENABLE_PULSE
    guac_client_data->audio_enabled =
        (strcmp(argv[IDX_ENABLE_AUDIO], "true") == 0);

    /* If an encoding is available, load an audio stream */
    if (guac_client_data->audio_enabled) {    

        guac_client_data->audio = guac_audio_stream_alloc(client, NULL);

        /* Load servername if specified */
        if (argv[IDX_AUDIO_SERVERNAME][0] != '\0')
            guac_client_data->pa_servername =
                strdup(argv[IDX_AUDIO_SERVERNAME]);
        else
            guac_client_data->pa_servername = NULL;

        /* If successful, init audio system */
        if (guac_client_data->audio != NULL) {
            
            guac_client_log_info(client,
                    "Audio will be encoded as %s",
                    guac_client_data->audio->encoder->mimetype);

            /* Require threadsafe sockets if audio enabled */
            guac_socket_require_threadsafe(client->socket);

            /* Start audio stream */
            guac_pa_start_stream(client);
            
        }

        /* Otherwise, audio loading failed */
        else
            guac_client_log_info(client,
                    "No available audio encoding. Sound disabled.");

    } /* end if audio enabled */
#endif

    /* Set remaining client data */
    guac_client_data->rfb_client = rfb_client;
    guac_client_data->copy_rect_used = 0;
    guac_client_data->cursor = guac_client_alloc_buffer(client);

    /* Set handlers */
    client->handle_messages = vnc_guac_client_handle_messages;
    client->free_handler = vnc_guac_client_free_handler;

    /* If not read-only, set input handlers and pointer */
    if (guac_client_data->read_only == 0) {

        /* Only handle mouse/keyboard/clipboard if not read-only */
        client->mouse_handler = vnc_guac_client_mouse_handler;
        client->key_handler = vnc_guac_client_key_handler;
        client->clipboard_handler = vnc_guac_client_clipboard_handler;

        /* If not read-only but cursor is remote, set a default pointer */
        if (guac_client_data->remote_cursor)
            guac_vnc_set_default_pointer(client);

    }

    /* Send name */
    guac_protocol_send_name(client->socket, rfb_client->desktopName);

    /* Send size */
    guac_protocol_send_size(client->socket,
            GUAC_DEFAULT_LAYER, rfb_client->width, rfb_client->height);

    return 0;

}

