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
#include "clipboard.h"
#include "guac_clipboard.h"
#include "guac_dot_cursor.h"
#include "guac_handlers.h"
#include "guac_pointer_cursor.h"
#include "vnc_handlers.h"

#ifdef ENABLE_PULSE
#include "pulse.h"
#endif

#ifdef ENABLE_COMMON_SSH
#include "guac_sftp.h"
#include "guac_ssh.h"
#include "sftp.h"
#endif

#include <rfb/rfbclient.h>
#include <rfb/rfbproto.h>
#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>

#include <stdlib.h>
#include <string.h>
#include <time.h>

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
    "clipboard-encoding",

#ifdef ENABLE_VNC_REPEATER
    "dest-host",
    "dest-port",
#endif

#ifdef ENABLE_PULSE
    "enable-audio",
    "audio-servername",
#endif

#ifdef ENABLE_VNC_LISTEN
    "reverse-connect",
    "listen-timeout",
#endif

#ifdef ENABLE_COMMON_SSH
    "enable-sftp",
    "sftp-hostname",
    "sftp-port",
    "sftp-username",
    "sftp-password",
    "sftp-private-key",
    "sftp-passphrase",
    "sftp-directory",
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
    IDX_CURSOR,
    IDX_AUTORETRY,
    IDX_CLIPBOARD_ENCODING,

#ifdef ENABLE_VNC_REPEATER
    IDX_DEST_HOST,
    IDX_DEST_PORT,
#endif

#ifdef ENABLE_PULSE
    IDX_ENABLE_AUDIO,
    IDX_AUDIO_SERVERNAME,
#endif

#ifdef ENABLE_VNC_LISTEN
    IDX_REVERSE_CONNECT,
    IDX_LISTEN_TIMEOUT,
#endif

#ifdef ENABLE_COMMON_SSH
    IDX_ENABLE_SFTP,
    IDX_SFTP_HOSTNAME,
    IDX_SFTP_PORT,
    IDX_SFTP_USERNAME,
    IDX_SFTP_PASSWORD,
    IDX_SFTP_PRIVATE_KEY,
    IDX_SFTP_PASSPHRASE,
    IDX_SFTP_DIRECTORY,
#endif

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

#ifdef ENABLE_VNC_LISTEN
    /* If reverse connection enabled, start listening */
    if (guac_client_data->reverse_connect) {

        guac_client_log(client, GUAC_LOG_INFO, "Listening for connections on port %i",
                guac_client_data->port);

        /* Listen for connection from server */
        rfb_client->listenPort = guac_client_data->port;
        if (listenForIncomingConnectionsNoFork(rfb_client,
                    guac_client_data->listen_timeout*1000) <= 0)
            return NULL;

    }
#endif

    /* Set encodings if provided */
    if (guac_client_data->encodings)
        rfb_client->appData.encodingsString =
            strdup(guac_client_data->encodings);

    /* Connect */
    if (rfbInitClient(rfb_client, NULL, NULL))
        return rfb_client;

    /* If connection fails, return NULL */
    return NULL;

}

/**
 * Sets the encoding of clipboard data exchanged with the VNC server to the
 * encoding having the given name. If the name is left blank, or is invalid,
 * the standard ISO8859-1 encoding will be used.
 *
 * @param client
 *     The client to set the clipboard encoding of.
 *
 * @param name
 *     The name of the encoding to use for all clipboard data. Valid values
 *     are: "ISO8859-1", "UTF-8", "UTF-16", "CP1252", or "".
 *
 * @return
 *     Zero if the chosen encoding is standard for VNC, or non-zero if the VNC
 *     standard is being violated.
 */
static int guac_vnc_set_clipboard_encoding(guac_client* client,
        const char* name) {

    vnc_guac_client_data* guac_client_data =
        (vnc_guac_client_data*) client->data;

    /* Use ISO8859-1 if explicitly selected or blank */
    if (name[0] == '\0' || strcmp(name, "ISO8859-1") == 0) {
        guac_client_data->clipboard_reader = GUAC_READ_ISO8859_1;
        guac_client_data->clipboard_writer = GUAC_WRITE_ISO8859_1;
        return 0;
    }

    /* UTF-8 */
    if (strcmp(name, "UTF-8") == 0) {
        guac_client_data->clipboard_reader = GUAC_READ_UTF8;
        guac_client_data->clipboard_writer = GUAC_WRITE_UTF8;
        return 1;
    }

    /* UTF-16 */
    if (strcmp(name, "UTF-16") == 0) {
        guac_client_data->clipboard_reader = GUAC_READ_UTF16;
        guac_client_data->clipboard_writer = GUAC_WRITE_UTF16;
        return 1;
    }

    /* CP1252 */
    if (strcmp(name, "CP1252") == 0) {
        guac_client_data->clipboard_reader = GUAC_READ_CP1252;
        guac_client_data->clipboard_writer = GUAC_WRITE_CP1252;
        return 1;
    }

    /* If encoding unrecognized, warn and default to ISO8859-1 */
    guac_client_log(client, GUAC_LOG_WARNING,
            "Encoding '%s' is invalid. Defaulting to ISO8859-1.", name);

    guac_client_data->clipboard_reader = GUAC_READ_ISO8859_1;
    guac_client_data->clipboard_writer = GUAC_WRITE_ISO8859_1;
    return 0;

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
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR, "Wrong argument count received.");
        return 1;
    }

    /* Alloc client data */
    guac_client_data = malloc(sizeof(vnc_guac_client_data));
    client->data = guac_client_data;

    guac_client_data->hostname = strdup(argv[IDX_HOSTNAME]);
    guac_client_data->port = atoi(argv[IDX_PORT]);
    guac_client_data->password = strdup(argv[IDX_PASSWORD]); /* NOTE: freed by libvncclient */
    guac_client_data->default_surface = NULL;

    /* Set flags */
    guac_client_data->remote_cursor = (strcmp(argv[IDX_CURSOR], "remote") == 0);
    guac_client_data->swap_red_blue = (strcmp(argv[IDX_SWAP_RED_BLUE], "true") == 0);
    guac_client_data->read_only     = (strcmp(argv[IDX_READ_ONLY], "true") == 0);

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

#ifdef ENABLE_VNC_LISTEN
    /* Set reverse-connection flag */
    guac_client_data->reverse_connect =
        (strcmp(argv[IDX_REVERSE_CONNECT], "true") == 0);

    /* Parse listen timeout */
    if (argv[IDX_LISTEN_TIMEOUT][0] != '\0')
        guac_client_data->listen_timeout = atoi(argv[IDX_LISTEN_TIMEOUT]);
    else
        guac_client_data->listen_timeout = 5000;
#endif

    /* Init clipboard */
    guac_client_data->clipboard = guac_common_clipboard_alloc(GUAC_VNC_CLIPBOARD_MAX_LENGTH);

    /* Configure clipboard encoding */
    if (guac_vnc_set_clipboard_encoding(client, argv[IDX_CLIPBOARD_ENCODING]))
        guac_client_log(client, GUAC_LOG_INFO,
                "Using non-standard VNC clipboard encoding: '%s'.",
                argv[IDX_CLIPBOARD_ENCODING]);

    /* Ensure connection is kept alive during lengthy connects */
    guac_socket_require_keep_alive(client->socket);

    /* Attempt connection */
    rfb_client = __guac_vnc_get_client(client);

    /* If unsuccessful, retry as many times as specified */
    while (!rfb_client && retries_remaining > 0) {

        struct timespec guac_vnc_connect_interval = {
            .tv_sec  =  GUAC_VNC_CONNECT_INTERVAL/1000,
            .tv_nsec = (GUAC_VNC_CONNECT_INTERVAL%1000)*1000000
        };

        guac_client_log(client, GUAC_LOG_INFO,
                "Connect failed. Waiting %ims before retrying...",
                GUAC_VNC_CONNECT_INTERVAL);

        /* Wait for given interval then retry */
        nanosleep(&guac_vnc_connect_interval, NULL);
        rfb_client = __guac_vnc_get_client(client);
        retries_remaining--;

    }

    /* If the final connect attempt fails, return error */
    if (!rfb_client) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_UPSTREAM_ERROR, "Unable to connect to VNC server.");
        return 1;
    }

#ifdef ENABLE_PULSE
    guac_client_data->audio_enabled =
        (strcmp(argv[IDX_ENABLE_AUDIO], "true") == 0);

    /* If an encoding is available, load an audio stream */
    if (guac_client_data->audio_enabled) {    

        guac_client_data->audio = guac_audio_stream_alloc(client, NULL,
                GUAC_VNC_AUDIO_RATE,
                GUAC_VNC_AUDIO_CHANNELS,
                GUAC_VNC_AUDIO_BPS);

        /* Load servername if specified */
        if (argv[IDX_AUDIO_SERVERNAME][0] != '\0')
            guac_client_data->pa_servername =
                strdup(argv[IDX_AUDIO_SERVERNAME]);
        else
            guac_client_data->pa_servername = NULL;

        /* If successful, init audio system */
        if (guac_client_data->audio != NULL) {
            
            guac_client_log(client, GUAC_LOG_INFO,
                    "Audio will be encoded as %s",
                    guac_client_data->audio->encoder->mimetype);

            /* Require threadsafe sockets if audio enabled */
            guac_socket_require_threadsafe(client->socket);

            /* Start audio stream */
            guac_pa_start_stream(client);
            
        }

        /* Otherwise, audio loading failed */
        else
            guac_client_log(client, GUAC_LOG_INFO,
                    "No available audio encoding. Sound disabled.");

    } /* end if audio enabled */
#endif

#ifdef ENABLE_COMMON_SSH
    guac_common_ssh_init(client);

    /* Connect via SSH if SFTP is enabled */
    if (strcmp(argv[IDX_ENABLE_SFTP], "true") == 0) {

        guac_client_log(client, GUAC_LOG_DEBUG,
                "Connecting via SSH for SFTP filesystem access.");

        guac_client_data->sftp_user =
            guac_common_ssh_create_user(argv[IDX_SFTP_USERNAME]);

        /* Import private key, if given */
        if (argv[IDX_SFTP_PRIVATE_KEY][0] != '\0') {

            guac_client_log(client, GUAC_LOG_DEBUG,
                    "Authenticating with private key.");

            /* Abort if private key cannot be read */
            if (guac_common_ssh_user_import_key(guac_client_data->sftp_user,
                        argv[IDX_SFTP_PRIVATE_KEY],
                        argv[IDX_SFTP_PASSPHRASE])) {
                guac_common_ssh_destroy_user(guac_client_data->sftp_user);
                return 1;
            }

        }

        /* Otherwise, use specified password */
        else {
            guac_client_log(client, GUAC_LOG_DEBUG,
                    "Authenticating with password.");
            guac_common_ssh_user_set_password(guac_client_data->sftp_user,
                    argv[IDX_SFTP_PASSWORD]);
        }

        /* Parse hostname - use VNC hostname by default */
        const char* sftp_hostname = argv[IDX_SFTP_HOSTNAME];
        if (sftp_hostname[0] == '\0')
            sftp_hostname = guac_client_data->hostname;

        /* Parse port, defaulting to standard SSH port */
        const char* sftp_port = argv[IDX_SFTP_PORT];
        if (sftp_port[0] == '\0')
            sftp_port = "22";

        /* Attempt SSH connection */
        guac_client_data->sftp_session =
            guac_common_ssh_create_session(client, sftp_hostname, sftp_port,
                    guac_client_data->sftp_user);

        /* Fail if SSH connection does not succeed */
        if (guac_client_data->sftp_session == NULL) {
            /* Already aborted within guac_common_ssh_create_session() */
            guac_common_ssh_destroy_user(guac_client_data->sftp_user);
            return 1;
        }

        /* Load and expose filesystem */
        guac_client_data->sftp_filesystem =
            guac_common_ssh_create_sftp_filesystem(
                    guac_client_data->sftp_session, "/");

        /* Abort if SFTP connection fails */
        if (guac_client_data->sftp_filesystem == NULL) {
            guac_common_ssh_destroy_session(guac_client_data->sftp_session);
            guac_common_ssh_destroy_user(guac_client_data->sftp_user);
            return 1;
        }

        /* Configure destination for basic uploads, if specified */
        if (argv[IDX_SFTP_DIRECTORY][0] != '\0')
            guac_common_ssh_sftp_set_upload_path(
                    guac_client_data->sftp_filesystem,
                    argv[IDX_SFTP_DIRECTORY]);

        /* Set file handler for basic uploads */
        client->file_handler = guac_vnc_sftp_file_handler;

        guac_client_log(client, GUAC_LOG_DEBUG,
                "SFTP connection succeeded.");

    }
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
        client->clipboard_handler = guac_vnc_clipboard_handler;

        /* If not read-only but cursor is remote, set a dot cursor */
        if (guac_client_data->remote_cursor)
            guac_common_set_dot_cursor(client);

        /* Otherwise, set pointer until explicitly requested otherwise */
        else
            guac_common_set_pointer_cursor(client);

    }

    /* Send name */
    guac_protocol_send_name(client->socket, rfb_client->desktopName);

    /* Create default surface */
    guac_client_data->default_surface = guac_common_surface_alloc(client,
            client->socket, GUAC_DEFAULT_LAYER,
            rfb_client->width, rfb_client->height);
    return 0;

}

