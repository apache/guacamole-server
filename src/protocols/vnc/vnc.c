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

#include "auth.h"
#include "client.h"
#include "clipboard.h"
#include "cursor.h"
#include "display.h"
#include "guac_clipboard.h"
#include "guac_cursor.h"
#include "guac_display.h"
#include "log.h"
#include "settings.h"
#include "vnc.h"

#ifdef ENABLE_PULSE
#include "pulse.h"
#endif

#ifdef ENABLE_COMMON_SSH
#include "guac_sftp.h"
#include "guac_ssh.h"
#include "sftp.h"
#endif

#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/timestamp.h>
#include <rfb/rfbclient.h>
#include <rfb/rfbproto.h>

#include <stdlib.h>
#include <string.h>
#include <time.h>

char* GUAC_VNC_CLIENT_KEY = "GUAC_VNC";

rfbClient* guac_vnc_get_client(guac_client* client) {

    rfbClient* rfb_client = rfbGetClient(8, 3, 4); /* 32-bpp client */
    guac_vnc_client* vnc_client = (guac_vnc_client*) client->data;
    guac_vnc_settings* vnc_settings = vnc_client->settings;

    /* Store Guac client in rfb client */
    rfbClientSetClientData(rfb_client, GUAC_VNC_CLIENT_KEY, client);

    /* Framebuffer update handler */
    rfb_client->GotFrameBufferUpdate = guac_vnc_update;
    rfb_client->GotCopyRect = guac_vnc_copyrect;

    /* Do not handle clipboard and local cursor if read-only */
    if (vnc_settings->read_only == 0) {

        /* Clipboard */
        rfb_client->GotXCutText = guac_vnc_cut_text;

        /* Set remote cursor */
        if (vnc_settings->remote_cursor) {
            rfb_client->appData.useRemoteCursor = FALSE;
        }

        else {
            /* Enable client-side cursor */
            rfb_client->appData.useRemoteCursor = TRUE;
            rfb_client->GotCursorShape = guac_vnc_cursor;
        }

    }

    /* Password */
    rfb_client->GetPassword = guac_vnc_get_password;

    /* Depth */
    guac_vnc_set_pixel_format(rfb_client, vnc_settings->color_depth);

    /* Hook into allocation so we can handle resize. */
    vnc_client->rfb_MallocFrameBuffer = rfb_client->MallocFrameBuffer;
    rfb_client->MallocFrameBuffer = guac_vnc_malloc_framebuffer;
    rfb_client->canHandleNewFBSize = 1;

    /* Set hostname and port */
    rfb_client->serverHost = strdup(vnc_settings->hostname);
    rfb_client->serverPort = vnc_settings->port;

#ifdef ENABLE_VNC_REPEATER
    /* Set repeater parameters if specified */
    if (vnc_settings->dest_host) {
        rfb_client->destHost = strdup(vnc_settings->dest_host);
        rfb_client->destPort = vnc_settings->dest_port;
    }
#endif

#ifdef ENABLE_VNC_LISTEN
    /* If reverse connection enabled, start listening */
    if (vnc_settings->reverse_connect) {

        guac_client_log(client, GUAC_LOG_INFO, "Listening for connections on port %i", vnc_settings->port);

        /* Listen for connection from server */
        rfb_client->listenPort = vnc_settings->port;
        if (listenForIncomingConnectionsNoFork(rfb_client, vnc_settings->listen_timeout*1000) <= 0)
            return NULL;

    }
#endif

    /* Set encodings if provided */
    if (vnc_settings->encodings)
        rfb_client->appData.encodingsString = strdup(vnc_settings->encodings);

    /* Connect */
    if (rfbInitClient(rfb_client, NULL, NULL))
        return rfb_client;

    /* If connection fails, return NULL */
    return NULL;

}

/**
 * Waits until data is available to be read from the given rfbClient, and thus
 * a call to HandleRFBServerMessages() should not block. If the timeout elapses
 * before data is available, zero is returned.
 *
 * @param rfb_client
 *     The rfbClient to wait for.
 *
 * @param timeout
 *     The maximum amount of time to wait, in microseconds.
 *
 * @returns
 *     A positive value if data is available, zero if the timeout elapses
 *     before data becomes available, or a negative value on error.
 */
static int guac_vnc_wait_for_messages(rfbClient* rfb_client, int timeout) {

    /* Do not explicitly wait while data is on the buffer */
    if (rfb_client->buffered)
        return 1;

    /* If no data on buffer, wait for data on socket */
    return WaitForMessage(rfb_client, timeout);

}

void* guac_vnc_client_thread(void* data) {

    guac_client* client = (guac_client*) data;
    guac_vnc_client* vnc_client = (guac_vnc_client*) client->data;
    guac_vnc_settings* settings = vnc_client->settings;

    /* Configure clipboard encoding */
    if (guac_vnc_set_clipboard_encoding(client, settings->clipboard_encoding)) {
        guac_client_log(client, GUAC_LOG_INFO, "Using non-standard VNC "
                "clipboard encoding: '%s'.", settings->clipboard_encoding);
    }

    /* Ensure connection is kept alive during lengthy connects */
    guac_socket_require_keep_alive(client->socket);

    /* Set up libvncclient logging */
    rfbClientLog = guac_vnc_client_log_info;
    rfbClientErr = guac_vnc_client_log_error;

    /* Attempt connection */
    rfbClient* rfb_client = guac_vnc_get_client(client);
    int retries_remaining = settings->retries;

    /* If unsuccessful, retry as many times as specified */
    while (!rfb_client && retries_remaining > 0) {

        guac_client_log(client, GUAC_LOG_INFO,
                "Connect failed. Waiting %ims before retrying...",
                GUAC_VNC_CONNECT_INTERVAL);

        /* Wait for given interval then retry */
        guac_timestamp_msleep(GUAC_VNC_CONNECT_INTERVAL);
        rfb_client = guac_vnc_get_client(client);
        retries_remaining--;

    }

    /* If the final connect attempt fails, return error */
    if (!rfb_client) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_UPSTREAM_ERROR, "Unable to connect to VNC server.");
        return NULL;
    }

#ifdef ENABLE_PULSE
    /* If an encoding is available, load an audio stream */
    if (settings->audio_enabled) {

        vnc_client->audio = guac_audio_stream_alloc(client, NULL,
                GUAC_VNC_AUDIO_RATE,
                GUAC_VNC_AUDIO_CHANNELS,
                GUAC_VNC_AUDIO_BPS);

        /* If successful, init audio system */
        if (vnc_client->audio != NULL) {
            
            guac_client_log(client, GUAC_LOG_INFO,
                    "Audio will be encoded as %s",
                    vnc_client->audio->encoder->mimetype);

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
    if (settings->enable_sftp) {

        guac_client_log(client, GUAC_LOG_DEBUG,
                "Connecting via SSH for SFTP filesystem access.");

        vnc_client->sftp_user =
            guac_common_ssh_create_user(settings->sftp_username);

        /* Import private key, if given */
        if (settings->sftp_private_key != NULL) {

            guac_client_log(client, GUAC_LOG_DEBUG,
                    "Authenticating with private key.");

            /* Abort if private key cannot be read */
            if (guac_common_ssh_user_import_key(vnc_client->sftp_user,
                        settings->sftp_private_key,
                        settings->sftp_passphrase)) {
                guac_common_ssh_destroy_user(vnc_client->sftp_user);
                return NULL;
            }

        }

        /* Otherwise, use specified password */
        else {
            guac_client_log(client, GUAC_LOG_DEBUG,
                    "Authenticating with password.");
            guac_common_ssh_user_set_password(vnc_client->sftp_user,
                    settings->sftp_password);
        }

        /* Attempt SSH connection */
        vnc_client->sftp_session =
            guac_common_ssh_create_session(client, settings->sftp_hostname,
                    settings->sftp_port, vnc_client->sftp_user);

        /* Fail if SSH connection does not succeed */
        if (vnc_client->sftp_session == NULL) {
            /* Already aborted within guac_common_ssh_create_session() */
            guac_common_ssh_destroy_user(vnc_client->sftp_user);
            return NULL;
        }

        /* Load filesystem */
        vnc_client->sftp_filesystem =
            guac_common_ssh_create_sftp_filesystem(
                    vnc_client->sftp_session, "/");

        /* Expose filesystem to connection owner */
        guac_client_for_owner(client,
                guac_common_ssh_expose_sftp_filesystem,
                vnc_client->sftp_filesystem);

        /* Abort if SFTP connection fails */
        if (vnc_client->sftp_filesystem == NULL) {
            guac_common_ssh_destroy_session(vnc_client->sftp_session);
            guac_common_ssh_destroy_user(vnc_client->sftp_user);
            return NULL;
        }

        /* Configure destination for basic uploads, if specified */
        if (settings->sftp_directory != NULL)
            guac_common_ssh_sftp_set_upload_path(
                    vnc_client->sftp_filesystem,
                    settings->sftp_directory);

        guac_client_log(client, GUAC_LOG_DEBUG,
                "SFTP connection succeeded.");

    }
#endif

    /* Set remaining client data */
    vnc_client->rfb_client = rfb_client;

    /* Send name */
    guac_protocol_send_name(client->socket, rfb_client->desktopName);

    /* Create display */
    vnc_client->display = guac_common_display_alloc(client,
            rfb_client->width, rfb_client->height);

    /* If not read-only, set an appropriate cursor */
    if (settings->read_only == 0) {
        if (settings->remote_cursor)
            guac_common_cursor_set_dot(vnc_client->display->cursor);
        else
            guac_common_cursor_set_pointer(vnc_client->display->cursor);

    }

    guac_socket_flush(client->socket);

    guac_timestamp last_frame_end = guac_timestamp_current();

    /* Handle messages from VNC server while client is running */
    while (client->state == GUAC_CLIENT_RUNNING) {

        /* Wait for start of frame */
        int wait_result = guac_vnc_wait_for_messages(rfb_client, 1000000);
        if (wait_result > 0) {

            guac_timestamp frame_start = guac_timestamp_current();

            /* Calculate time since last frame */
            int time_elapsed = frame_start - last_frame_end;
            int processing_lag = guac_client_get_processing_lag(client);

            /* Force roughly-equal length of server and client frames */
            if (time_elapsed < processing_lag)
                guac_timestamp_msleep(processing_lag - time_elapsed);

            /* Read server messages until frame is built */
            do {

                guac_timestamp frame_end;
                int frame_remaining;

                /* Handle any message received */
                if (!HandleRFBServerMessage(rfb_client)) {
                    guac_client_abort(client,
                            GUAC_PROTOCOL_STATUS_UPSTREAM_ERROR,
                            "Error handling message from VNC server.");
                    break;
                }

                /* Calculate time remaining in frame */
                frame_end = guac_timestamp_current();
                frame_remaining = frame_start + GUAC_VNC_FRAME_DURATION
                                - frame_end;

                /* Wait again if frame remaining */
                if (frame_remaining > 0)
                    wait_result = guac_vnc_wait_for_messages(rfb_client,
                            GUAC_VNC_FRAME_TIMEOUT*1000);
                else
                    break;

            } while (wait_result > 0);

            /* Record end of frame */
            last_frame_end = guac_timestamp_current();

        }

        /* If an error occurs, log it and fail */
        if (wait_result < 0)
            guac_client_abort(client, GUAC_PROTOCOL_STATUS_UPSTREAM_ERROR, "Connection closed.");

        guac_common_surface_flush(vnc_client->display->default_surface);
        guac_client_end_frame(client);
        guac_socket_flush(client->socket);

    }

    guac_client_log(client, GUAC_LOG_INFO, "Internal VNC client disconnected");
    return NULL;

}

