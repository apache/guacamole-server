/*
 * Copyright (C) 2015 Glyptodon LLC
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
#include "rdp.h"
#include "rdp_keymap.h"
#include "user.h"

#ifdef ENABLE_COMMON_SSH
#include <guac_sftp.h>
#include <guac_ssh.h>
#include <guac_ssh_user.h>
#endif

#ifdef HAVE_FREERDP_DISPLAY_UPDATE_SUPPORT
#include "rdp_disp.h"
#endif

#include <freerdp/cache/cache.h>
#include <freerdp/channels/channels.h>
#include <freerdp/freerdp.h>
#include <guacamole/client.h>
#include <guacamole/socket.h>

#ifdef HAVE_FREERDP_CLIENT_CLIPRDR_H
#include <freerdp/client/cliprdr.h>
#else
#include "compat/client-cliprdr.h"
#endif

#ifdef HAVE_FREERDP_CLIENT_CHANNELS_H
#include <freerdp/client/channels.h>
#endif

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

int guac_client_init(guac_client* client, int argc, char** argv) {

    /* Set client args */
    client->args = GUAC_RDP_CLIENT_ARGS;

    /* Alloc client data */
    guac_rdp_client* rdp_client = calloc(1, sizeof(guac_rdp_client));
    client->data = rdp_client;

    /* Init clipboard and shared mouse */
    rdp_client->clipboard = guac_common_clipboard_alloc(GUAC_RDP_CLIPBOARD_MAX_LENGTH);
    rdp_client->requested_clipboard_format = CB_FORMAT_TEXT;
    rdp_client->available_svc = guac_common_list_alloc();

    /* Recursive attribute for locks */
    pthread_mutexattr_init(&(rdp_client->attributes));
    pthread_mutexattr_settype(&(rdp_client->attributes),
            PTHREAD_MUTEX_RECURSIVE);

    /* Init RDP lock */
    pthread_mutex_init(&(rdp_client->rdp_lock), &(rdp_client->attributes));

    /* Clear keysym state mapping and keymap */
    memset(rdp_client->keysym_state, 0, sizeof(guac_rdp_keysym_state_map));
    memset(rdp_client->keymap, 0, sizeof(guac_rdp_static_keymap));

    /* Set handlers */
    client->join_handler = guac_rdp_user_join_handler;
    client->free_handler = guac_rdp_client_free_handler;

    return 0;

}

int guac_rdp_client_free_handler(guac_client* client) {

    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    freerdp* rdp_inst = rdp_client->rdp_inst;
    if (rdp_inst != NULL) {
        rdpChannels* channels = rdp_inst->context->channels;

        /* Clean up RDP client */
        freerdp_channels_close(channels, rdp_inst);
        freerdp_channels_free(channels);
        freerdp_disconnect(rdp_inst);
        freerdp_clrconv_free(((rdp_freerdp_context*) rdp_inst->context)->clrconv);
        cache_free(rdp_inst->context->cache);
        freerdp_free(rdp_inst);
    }

    /* Clean up filesystem, if allocated */
    if (rdp_client->filesystem != NULL)
        guac_rdp_fs_free(rdp_client->filesystem);

#ifdef ENABLE_COMMON_SSH
    /* Free SFTP filesystem, if loaded */
    if (rdp_client->sftp_filesystem)
        guac_common_ssh_destroy_sftp_filesystem(rdp_client->sftp_filesystem);

    /* Free SFTP session */
    if (rdp_client->sftp_session)
        guac_common_ssh_destroy_session(rdp_client->sftp_session);

    /* Free SFTP user */
    if (rdp_client->sftp_user)
        guac_common_ssh_destroy_user(rdp_client->sftp_user);

    guac_common_ssh_uninit();
#endif

#ifdef HAVE_FREERDP_DISPLAY_UPDATE_SUPPORT
    /* Free display update module */
    guac_rdp_disp_free(rdp_client->disp);
#endif

    /* Free SVC list */
    guac_common_list_free(rdp_client->available_svc);

    /* Free parsed settings */
    if (rdp_client->settings != NULL)
        guac_rdp_settings_free(rdp_client->settings);

    /* Free client data */
    guac_common_clipboard_free(rdp_client->clipboard);
    guac_common_display_free(rdp_client->display);
    free(rdp_client);

    return 0;

}

