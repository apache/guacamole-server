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

#include "client.h"
#include "common/clipboard.h"
#include "settings.h"
#include "spice.h"
#include "user.h"

#ifdef ENABLE_COMMON_SSH
#include "common-ssh/sftp.h"
#include "common-ssh/ssh.h"
#include "common-ssh/user.h"
#endif

#include <guacamole/audio.h>
#include <guacamole/client.h>
#include <guacamole/display.h>
#include <guacamole/mem.h>
#include <guacamole/recording.h>
#include <guacamole/socket.h>
#include <guacamole/string.h>

#include <pthread.h>

/**
 * Adds the given pending user to the SPICE audio stream, ensuring the user can
 * receive audio.
 *
 * @param user
 *     The pending user who should be added to the audio stream.
 *
 * @param data
 *     The guac_audio_stream that the user should be added to.
 *
 * @return
 *     Always NULL.
 */
static void* guac_spice_sync_pending_user_audio(guac_user* user, void* data) {
    guac_audio_stream* audio = (guac_audio_stream*) data;
    guac_audio_stream_add_user(audio, user);
    return NULL;
}

/**
 * A pending join handler which synchronizes the connection state for all
 * pending users prior to them being promoted to full users.
 *
 * @param client
 *     The client whose pending users are about to be promoted.
 *
 * @return
 *     Always zero.
 */
static int guac_spice_join_pending_handler(guac_client* client) {

    guac_spice_client* spice_client = (guac_spice_client*) client->data;
    guac_socket* broadcast_socket = client->pending_socket;

    /* Synchronize any audio stream for each pending user */
    if (spice_client->audio)
        guac_client_foreach_pending_user(client,
                guac_spice_sync_pending_user_audio, spice_client->audio);

    /* Advertise the maximum number of secondary monitors permitted for this
     * connection so a multi-monitor client can offer the right number of
     * monitor windows */
    if (spice_client->settings != NULL) {
        char max_monitors[12];
        guac_itoa(max_monitors,
                spice_client->settings->max_secondary_monitors);
        guac_client_stream_argv(client, broadcast_socket, "text/plain",
                "secondary-monitors", max_monitors);
    }

    /* Synchronize with current display */
    if (spice_client->display != NULL) {
        guac_display_dup(spice_client->display, broadcast_socket);
        guac_socket_flush(broadcast_socket);
    }

    return 0;

}

int guac_client_init(guac_client* client) {

    /* Set client args */
    client->args = GUAC_SPICE_CLIENT_ARGS;

    /* Alloc client data */
    guac_spice_client* spice_client = guac_mem_zalloc(sizeof(guac_spice_client));
    client->data = spice_client;

    /* Init surface metadata lock */
    pthread_mutex_init(&spice_client->surface_lock, NULL);

    /* Init outbound message lock and keyboard state lock */
    pthread_mutex_init(&spice_client->message_lock, NULL);
    pthread_rwlock_init(&spice_client->lock, NULL);

    /* Default to client (absolute) mouse mode until told otherwise */
    spice_client->mouse_mode = SPICE_MOUSE_MODE_CLIENT;

    /* Set handlers */
    client->join_handler = guac_spice_user_join_handler;
    client->join_pending_handler = guac_spice_join_pending_handler;
    client->leave_handler = guac_spice_user_leave_handler;
    client->free_handler = guac_spice_client_free_handler;

    return 0;

}

int guac_spice_client_free_handler(guac_client* client) {

    guac_spice_client* spice_client = (guac_spice_client*) client->data;
    guac_spice_settings* settings = spice_client->settings;

    /* Ensure all background rendering processes are stopped before freeing
     * underlying memory */
    if (spice_client->display != NULL)
        guac_display_stop(spice_client->display);

    /* Signal the SPICE event loop to stop, then wait for the client thread to
     * finish */
    if (spice_client->main_loop != NULL)
        g_main_loop_quit(spice_client->main_loop);

    pthread_join(spice_client->client_thread, NULL);

    /* Clean up the SPICE session */
    if (spice_client->spice_session != NULL) {
        spice_session_disconnect(spice_client->spice_session);
        g_object_unref(spice_client->spice_session);
    }

    /* Free the GLib main loop and context */
    if (spice_client->main_loop != NULL)
        g_main_loop_unref(spice_client->main_loop);

    if (spice_client->main_context != NULL)
        g_main_context_unref(spice_client->main_context);

#ifdef ENABLE_COMMON_SSH
    /* Free SFTP filesystem, if loaded */
    if (spice_client->sftp_filesystem)
        guac_common_ssh_destroy_sftp_filesystem(spice_client->sftp_filesystem);

    /* Free SFTP session */
    if (spice_client->sftp_session)
        guac_common_ssh_destroy_session(spice_client->sftp_session);

    /* Free SFTP user */
    if (spice_client->sftp_user)
        guac_common_ssh_destroy_user(spice_client->sftp_user);

    guac_common_ssh_uninit();
#endif

    /* Free shared folder, if allocated */
    if (spice_client->shared_folder != NULL)
        guac_spice_folder_free(spice_client->shared_folder);

    /* Clean up recording, if in progress */
    if (spice_client->recording != NULL)
        guac_recording_free(spice_client->recording);

    /* Free clipboard */
    if (spice_client->clipboard != NULL)
        guac_common_clipboard_free(spice_client->clipboard);

    /* Free display */
    if (spice_client->display != NULL)
        guac_display_free(spice_client->display);

    /* If audio enabled, stop streaming */
    if (spice_client->audio != NULL)
        guac_audio_stream_free(spice_client->audio);

    /* Free keyboard (client thread has already been joined, so no concurrent
     * access remains) */
    if (spice_client->keyboard != NULL)
        guac_spice_keyboard_free(spice_client->keyboard);

    /* Free parsed settings */
    if (settings != NULL)
        guac_spice_settings_free(settings);

    /* Clean up locks */
    pthread_mutex_destroy(&spice_client->surface_lock);
    pthread_mutex_destroy(&spice_client->message_lock);
    pthread_rwlock_destroy(&spice_client->lock);

    /* Free generic data struct */
    guac_mem_free(client->data);

    return 0;

}
