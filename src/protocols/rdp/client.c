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
#include "rdp_disp.h"
#include "rdp_keymap.h"
#include "user.h"

#ifdef ENABLE_COMMON_SSH
#include <guac_sftp.h>
#include <guac_ssh.h>
#include <guac_ssh_user.h>
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

    /* Init clipboard */
    rdp_client->clipboard = guac_common_clipboard_alloc(GUAC_RDP_CLIPBOARD_MAX_LENGTH);

    /* Init display update module */
    rdp_client->disp = guac_rdp_disp_alloc();

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

    /* Wait for client thread */
    pthread_join(rdp_client->client_thread, NULL);

    /* Free parsed settings */
    if (rdp_client->settings != NULL)
        guac_rdp_settings_free(rdp_client->settings);

    /* Free display update module */
    guac_rdp_disp_free(rdp_client->disp);

    /* Free client data */
    guac_common_clipboard_free(rdp_client->clipboard);
    free(rdp_client);

    return 0;

}

