/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "config.h"

#include "auth.h"
#include "client.h"
#include "clipboard.h"
#include "common/clipboard.h"
#include "common/cursor.h"
#include "common/display.h"
#include "common/recording.h"
#include "cursor.h"
#include "display.h"
#include "log.h"
#include "settings.h"
#include "vnc.h"

#ifdef ENABLE_PULSE
#include "pulse/pulse.h"
#endif

#ifdef ENABLE_COMMON_SSH
#include "common-ssh/sftp.h"
#include "common-ssh/ssh.h"
#include "sftp.h"
#endif

#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/timestamp.h>
#include <guacamole/wol.h>
#include <rfb/rfbclient.h>
#include <rfb/rfbconfig.h>
#include <rfb/rfbproto.h>

#ifdef LIBVNCSERVER_WITH_CLIENT_GCRYPT
#include <errno.h>
#include <gcrypt.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef LIBVNCSERVER_WITH_CLIENT_GCRYPT
GCRY_THREAD_OPTION_PTHREAD_IMPL;
#endif

char* GUAC_VNC_CLIENT_KEY = "GUAC_VNC";

#ifdef ENABLE_VNC_TLS_LOCKING
/**
 * A callback function that is called by the VNC library prior to writing
 * data to a TLS-encrypted socket.  This returns the rfbBool FALSE value
 * if there's an error locking the mutex, or rfbBool TRUE otherwise.
 * 
 * @param rfb_client
 *     The rfbClient for which to lock the TLS mutex.
 *
 * @returns
 *     rfbBool FALSE if an error occurs locking the mutex, otherwise
 *     TRUE.
 */
static rfbBool guac_vnc_lock_write_to_tls(rfbClient* rfb_client) {

    /* Retrieve the Guacamole data structures */
    guac_client* gc = rfbClientGetClientData(rfb_client, GUAC_VNC_CLIENT_KEY);
    guac_vnc_client* vnc_client = (guac_vnc_client*) gc->data;

    /* Lock write access */
    int retval = pthread_mutex_lock(&(vnc_client->tls_lock));
    if (retval) {
        guac_client_log(gc, GUAC_LOG_ERROR, "Error locking TLS write mutex: %s",
                strerror(retval));
        return FALSE;
    }
    return TRUE;

}

/**
 * A callback function for use by the VNC library that is called once
 * the client is finished writing to a TLS-encrypted socket. A rfbBool
 * FALSE value is returned if an error occurs unlocking the mutex,
 * otherwise TRUE is returned.
 *
 * @param rfb_client
 *     The rfbClient for which to unlock the TLS mutex.
 *
 * @returns
 *     rfbBool FALSE if an error occurs unlocking the mutex, otherwise
 *     TRUE.
 */
static rfbBool guac_vnc_unlock_write_to_tls(rfbClient* rfb_client) {

    /* Retrieve the Guacamole data structures */
    guac_client* gc = rfbClientGetClientData(rfb_client, GUAC_VNC_CLIENT_KEY);
    guac_vnc_client* vnc_client = (guac_vnc_client*) gc->data;

    /* Unlock write access */
    int retval = pthread_mutex_unlock(&(vnc_client->tls_lock));
    if (retval) {
        guac_client_log(gc, GUAC_LOG_ERROR, "Error unlocking TLS write mutex: %s",
                strerror(retval));
        return FALSE;
    }
    return TRUE;
}
#endif

rfbClient* guac_vnc_get_client(guac_client* client) {

    rfbClient* rfb_client = rfbGetClient(8, 3, 4); /* 32-bpp client */
    guac_vnc_client* vnc_client = (guac_vnc_client*) client->data;
    guac_vnc_settings* vnc_settings = vnc_client->settings;

    /* Store Guac client in rfb client */
    rfbClientSetClientData(rfb_client, GUAC_VNC_CLIENT_KEY, client);

    /* Framebuffer update handler */
    rfb_client->GotFrameBufferUpdate = guac_vnc_update;
    rfb_client->GotCopyRect = guac_vnc_copyrect;

#ifdef ENABLE_VNC_TLS_LOCKING
    /* TLS Locking and Unlocking */
    rfb_client->LockWriteToTLS = guac_vnc_lock_write_to_tls;
    rfb_client->UnlockWriteToTLS = guac_vnc_unlock_write_to_tls;
#endif
    
#ifdef LIBVNCSERVER_WITH_CLIENT_GCRYPT
    
    /* Check if GCrypt is initialized, do it if not. */
    if (!gcry_control(GCRYCTL_INITIALIZATION_FINISHED_P)) {
    
        guac_client_log(client, GUAC_LOG_DEBUG, "GCrypt initialization started.");

        /* Initialize thread control. */
        gcry_control(GCRYCTL_SET_THREAD_CBS, &gcry_threads_pthread);

        /* Basic GCrypt library initialization. */
        gcry_check_version(NULL);

        /* Mark initialization as completed. */
        gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);
        guac_client_log(client, GUAC_LOG_DEBUG, "GCrypt initialization completed.");
    
    }
    
#endif

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

#ifdef ENABLE_VNC_GENERIC_CREDENTIALS
    /* Authentication */
    rfb_client->GetCredential = guac_vnc_get_credentials;
#endif
    
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

    /* If Wake-on-LAN is enabled, attempt to wake. */
    if (settings->wol_send_packet) {
        guac_client_log(client, GUAC_LOG_DEBUG, "Sending Wake-on-LAN packet, "
                "and pausing for %d seconds.", settings->wol_wait_time);
        
        /* Send the Wake-on-LAN request. */
        if (guac_wol_wake(settings->wol_mac_addr, settings->wol_broadcast_addr,
                settings->wol_udp_port))
            return NULL;
        
        /* If wait time is specified, sleep for that amount of time. */
        if (settings->wol_wait_time > 0)
            guac_timestamp_msleep(settings->wol_wait_time * 1000);
    }
    
    /* Configure clipboard encoding */
    if (guac_vnc_set_clipboard_encoding(client, settings->clipboard_encoding)) {
        guac_client_log(client, GUAC_LOG_INFO, "Using non-standard VNC "
                "clipboard encoding: '%s'.", settings->clipboard_encoding);
    }

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
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_UPSTREAM_NOT_FOUND,
                "Unable to connect to VNC server.");
        return NULL;
    }

#ifdef ENABLE_PULSE
    /* If audio is enabled, start streaming via PulseAudio */
    if (settings->audio_enabled)
        vnc_client->audio = guac_pa_stream_alloc(client, 
                settings->pa_servername);
#endif

#ifdef ENABLE_COMMON_SSH
    guac_common_ssh_init(client);

    /* Connect via SSH if SFTP is enabled */
    if (settings->enable_sftp) {

        /* Abort if username is missing */
        if (settings->sftp_username == NULL) {
            guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR,
                    "SFTP username is required if SFTP is enabled.");
            return NULL;
        }

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
                guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR,
                        "Private key unreadable.");
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
                    settings->sftp_port, vnc_client->sftp_user, settings->sftp_server_alive_interval,
                    settings->sftp_host_key, NULL);

        /* Fail if SSH connection does not succeed */
        if (vnc_client->sftp_session == NULL) {
            /* Already aborted within guac_common_ssh_create_session() */
            return NULL;
        }

        /* Load filesystem */
        vnc_client->sftp_filesystem =
            guac_common_ssh_create_sftp_filesystem(vnc_client->sftp_session,
                    settings->sftp_root_directory, NULL,
                    settings->sftp_disable_download,
                    settings->sftp_disable_upload);

        /* Expose filesystem to connection owner */
        guac_client_for_owner(client,
                guac_common_ssh_expose_sftp_filesystem,
                vnc_client->sftp_filesystem);

        /* Abort if SFTP connection fails */
        if (vnc_client->sftp_filesystem == NULL) {
            guac_client_abort(client, GUAC_PROTOCOL_STATUS_UPSTREAM_ERROR,
                    "SFTP connection failed.");
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

    /* Set up screen recording, if requested */
    if (settings->recording_path != NULL) {
        vnc_client->recording = guac_common_recording_create(client,
                settings->recording_path,
                settings->recording_name,
                settings->create_recording_path,
                !settings->recording_exclude_output,
                !settings->recording_exclude_mouse,
                0, /* Touch events not supported */
                settings->recording_include_keys);
    }

    /* Create display */
    vnc_client->display = guac_common_display_alloc(client,
            rfb_client->width, rfb_client->height);

    /* Use lossless compression only if requested (otherwise, use default
     * heuristics) */
    guac_common_display_set_lossless(vnc_client->display, settings->lossless);

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
        int wait_result = guac_vnc_wait_for_messages(rfb_client,
                GUAC_VNC_FRAME_START_TIMEOUT);
        if (wait_result > 0) {

            int processing_lag = guac_client_get_processing_lag(client);
            guac_timestamp frame_start = guac_timestamp_current();

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

                /* Calculate time that client needs to catch up */
                int time_elapsed = frame_end - last_frame_end;
                int required_wait = processing_lag - time_elapsed;

                /* Increase the duration of this frame if client is lagging */
                if (required_wait > GUAC_VNC_FRAME_TIMEOUT)
                    wait_result = guac_vnc_wait_for_messages(rfb_client,
                            required_wait*1000);

                /* Wait again if frame remaining */
                else if (frame_remaining > 0)
                    wait_result = guac_vnc_wait_for_messages(rfb_client,
                            GUAC_VNC_FRAME_TIMEOUT*1000);
                else
                    break;

            } while (wait_result > 0);

            /* Record end of frame, excluding server-side rendering time (we
             * assume server-side rendering time will be consistent between any
             * two subsequent frames, and that this time should thus be
             * excluded from the required wait period of the next frame). */
            last_frame_end = frame_start;

        }

        /* If an error occurs, log it and fail */
        if (wait_result < 0)
            guac_client_abort(client, GUAC_PROTOCOL_STATUS_UPSTREAM_ERROR, "Connection closed.");

        /* Flush frame */
        guac_common_surface_flush(vnc_client->display->default_surface);
        guac_client_end_frame(client);
        guac_socket_flush(client->socket);

    }

    /* Kill client and finish connection */
    guac_client_stop(client);
    guac_client_log(client, GUAC_LOG_INFO, "Internal VNC client disconnected");
    return NULL;

}
