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
#include "guac_cursor.h"
#include "guac_display.h"
#include "rdp.h"
#include "rdp_bitmap.h"
#include "rdp_cliprdr.h"
#include "rdp_gdi.h"
#include "rdp_glyph.h"
#include "rdp_keymap.h"
#include "rdp_pointer.h"
#include "rdp_rail.h"
#include "rdp_stream.h"
#include "rdp_svc.h"

#ifdef ENABLE_COMMON_SSH
#include <guac_sftp.h>
#include <guac_ssh.h>
#include <guac_ssh_user.h>
#endif

#ifdef HAVE_FREERDP_DISPLAY_UPDATE_SUPPORT
#include "rdp_disp.h"
#endif

#include <freerdp/cache/bitmap.h>
#include <freerdp/cache/brush.h>
#include <freerdp/cache/glyph.h>
#include <freerdp/cache/offscreen.h>
#include <freerdp/cache/palette.h>
#include <freerdp/cache/pointer.h>
#include <freerdp/channels/channels.h>
#include <freerdp/freerdp.h>
#include <guacamole/audio.h>
#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/timestamp.h>

#ifdef HAVE_FREERDP_CLIENT_CLIPRDR_H
#include <freerdp/client/cliprdr.h>
#else
#include "compat/client-cliprdr.h"
#endif

#ifdef HAVE_FREERDP_CLIENT_DISP_H
#include <freerdp/client/disp.h>
#endif

#ifdef HAVE_FREERDP_EVENT_PUBSUB
#include <freerdp/event.h>
#endif

#ifdef LEGACY_FREERDP
#include "compat/rail.h"
#else
#include <freerdp/rail.h>
#endif

#ifdef ENABLE_WINPR
#include <winpr/wtypes.h>
#else
#include "compat/winpr-wtypes.h"
#endif

#ifdef HAVE_FREERDP_ADDIN_H
#include <freerdp/addin.h>
#endif

#ifdef HAVE_FREERDP_CLIENT_CHANNELS_H
#include <freerdp/client/channels.h>
#endif

#ifdef HAVE_FREERDP_VERSION_H
#include <freerdp/version.h>
#endif

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>

#if defined(FREERDP_VERSION_MAJOR) && (FREERDP_VERSION_MAJOR > 1 || FREERDP_VERSION_MINOR >= 2)
int __guac_receive_channel_data(freerdp* rdp_inst, UINT16 channelId, BYTE* data, int size, int flags, int total_size) {
#else
int __guac_receive_channel_data(freerdp* rdp_inst, int channelId, UINT8* data, int size, int flags, int total_size) {
#endif
    return freerdp_channels_data(rdp_inst, channelId, data, size, flags, total_size);
}

#ifdef HAVE_FREERDP_EVENT_PUBSUB
/**
 * Called whenever a channel connects via the PubSub event system within
 * FreeRDP.
 *
 * @param context The rdpContext associated with the active RDP session.
 * @param e Event-specific arguments, mainly the name of the channel, and a
 *          reference to the associated plugin loaded for that channel by
 *          FreeRDP.
 */
static void guac_rdp_channel_connected(rdpContext* context,
        ChannelConnectedEventArgs* e) {

#ifdef HAVE_RDPSETTINGS_SUPPORTDISPLAYCONTROL
    /* Store reference to the display update plugin once it's connected */
    if (strcmp(e->name, DISP_DVC_CHANNEL_NAME) == 0) {

        DispClientContext* disp = (DispClientContext*) e->pInterface;

        guac_client* client = ((rdp_freerdp_context*) context)->client;
        guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

        /* Init module with current display size */
        guac_rdp_disp_set_size(rdp_client->disp, context,
                guac_rdp_get_width(context->instance),
                guac_rdp_get_height(context->instance));

        /* Store connected channel */
        guac_rdp_disp_connect(rdp_client->disp, disp);
        guac_client_log(client, GUAC_LOG_DEBUG,
                "Display update channel connected.");

    }
#endif

}
#endif

BOOL rdp_freerdp_pre_connect(freerdp* instance) {

    rdpContext* context = instance->context;
    guac_client* client = ((rdp_freerdp_context*) context)->client;
    rdpChannels* channels = context->channels;
    rdpBitmap* bitmap;
    rdpGlyph* glyph;
    rdpPointer* pointer;
    rdpPrimaryUpdate* primary;
    CLRCONV* clrconv;

    guac_rdp_client* rdp_client =
        (guac_rdp_client*) client->data;

#ifdef HAVE_FREERDP_REGISTER_ADDIN_PROVIDER
    /* Init FreeRDP add-in provider */
    freerdp_register_addin_provider(freerdp_channels_load_static_addin_entry, 0);
#endif

#ifdef HAVE_FREERDP_EVENT_PUBSUB
    /* Subscribe to and handle channel connected events */
    PubSub_SubscribeChannelConnected(context->pubSub,
            (pChannelConnectedEventHandler) guac_rdp_channel_connected);
#endif

#ifdef HAVE_FREERDP_DISPLAY_UPDATE_SUPPORT
    /* Load virtual channel management plugin */
    if (freerdp_channels_load_plugin(channels, instance->settings,
                "drdynvc", instance->settings))
        guac_client_log(client, GUAC_LOG_WARNING,
                "Failed to load drdynvc plugin.");

    /* Init display update plugin */
    rdp_client->disp = guac_rdp_disp_alloc();
    guac_rdp_disp_load_plugin(instance->context);
#endif

    /* Load clipboard plugin */
    if (freerdp_channels_load_plugin(channels, instance->settings,
                "cliprdr", NULL))
        guac_client_log(client, GUAC_LOG_WARNING,
                "Failed to load cliprdr plugin. Clipboard will not work.");

    /* If audio enabled, choose an encoder */
    if (rdp_client->settings->audio_enabled) {

        rdp_client->audio = guac_audio_stream_alloc(client, NULL,
                GUAC_RDP_AUDIO_RATE,
                GUAC_RDP_AUDIO_CHANNELS,
                GUAC_RDP_AUDIO_BPS);

        /* Warn if no audio encoding is available */
        if (rdp_client->audio == NULL)
            guac_client_log(client, GUAC_LOG_INFO,
                    "No available audio encoding. Sound disabled.");

    } /* end if audio enabled */

    /* Load filesystem if drive enabled */
    if (rdp_client->settings->drive_enabled)
        rdp_client->filesystem =
            guac_rdp_fs_alloc(client, rdp_client->settings->drive_path,
                    rdp_client->settings->create_drive_path);

    /* If RDPSND/RDPDR required, load them */
    if (rdp_client->settings->printing_enabled
        || rdp_client->settings->drive_enabled
        || rdp_client->settings->audio_enabled) {

        /* Load RDPDR plugin */
        if (freerdp_channels_load_plugin(channels, instance->settings,
                    "guacdr", client))
            guac_client_log(client, GUAC_LOG_WARNING,
                    "Failed to load guacdr plugin. Drive redirection and "
                    "printing will not work. Sound MAY not work.");

        /* Load RDPSND plugin */
        if (freerdp_channels_load_plugin(channels, instance->settings,
                    "guacsnd", client))
            guac_client_log(client, GUAC_LOG_WARNING,
                    "Failed to load guacsnd alongside guacdr plugin. Sound "
                    "will not work. Drive redirection and printing MAY not "
                    "work.");

    }

    /* Load RAIL plugin if RemoteApp in use */
    if (rdp_client->settings->remote_app != NULL) {

#ifdef LEGACY_FREERDP
        RDP_PLUGIN_DATA* plugin_data = malloc(sizeof(RDP_PLUGIN_DATA) * 2);

        plugin_data[0].size = sizeof(RDP_PLUGIN_DATA);
        plugin_data[0].data[0] = rdp_client->settings->remote_app;
        plugin_data[0].data[1] = rdp_client->settings->remote_app_dir;
        plugin_data[0].data[2] = rdp_client->settings->remote_app_args; 
        plugin_data[0].data[3] = NULL;

        plugin_data[1].size = 0;

        /* Attempt to load rail */
        if (freerdp_channels_load_plugin(channels, instance->settings,
                    "rail", plugin_data))
            guac_client_log(client, GUAC_LOG_WARNING,
                    "Failed to load rail plugin. RemoteApp will not work.");
#else
        /* Attempt to load rail */
        if (freerdp_channels_load_plugin(channels, instance->settings,
                    "rail", instance->settings))
            guac_client_log(client, GUAC_LOG_WARNING,
                    "Failed to load rail plugin. RemoteApp will not work.");
#endif

    }

    /* Load SVC plugin instances for all static channels */
    if (rdp_client->settings->svc_names != NULL) {

        char** current = rdp_client->settings->svc_names;
        do {

            guac_rdp_svc* svc = guac_rdp_alloc_svc(client, *current);

            /* Attempt to load guacsvc plugin for new static channel */
            if (freerdp_channels_load_plugin(channels, instance->settings,
                        "guacsvc", svc)) {
                guac_client_log(client, GUAC_LOG_WARNING,
                        "Cannot create static channel \"%s\": failed to load guacsvc plugin.",
                        svc->name);
                guac_rdp_free_svc(svc);
            }

            /* Store and log on success */
            else {
                guac_rdp_add_svc(client, svc);
                guac_client_log(client, GUAC_LOG_INFO, "Created static channel \"%s\"...",
                        svc->name);
            }

        } while (*(++current) != NULL);

    }

    /* Init color conversion structure */
    clrconv = calloc(1, sizeof(CLRCONV));
    clrconv->alpha = 1;
    clrconv->invert = 0;
    clrconv->rgb555 = 0;
    clrconv->palette = calloc(1, sizeof(rdpPalette));
    ((rdp_freerdp_context*) context)->clrconv = clrconv;

    /* Init FreeRDP cache */
    instance->context->cache = cache_new(instance->settings);

    /* Set up bitmap handling */
    bitmap = calloc(1, sizeof(rdpBitmap));
    bitmap->size = sizeof(guac_rdp_bitmap);
    bitmap->New = guac_rdp_bitmap_new;
    bitmap->Free = guac_rdp_bitmap_free;
    bitmap->Paint = guac_rdp_bitmap_paint;
    bitmap->Decompress = guac_rdp_bitmap_decompress;
    bitmap->SetSurface = guac_rdp_bitmap_setsurface;
    graphics_register_bitmap(context->graphics, bitmap);
    free(bitmap);

    /* Set up glyph handling */
    glyph = calloc(1, sizeof(rdpGlyph));
    glyph->size = sizeof(guac_rdp_glyph);
    glyph->New = guac_rdp_glyph_new;
    glyph->Free = guac_rdp_glyph_free;
    glyph->Draw = guac_rdp_glyph_draw;
    glyph->BeginDraw = guac_rdp_glyph_begindraw;
    glyph->EndDraw = guac_rdp_glyph_enddraw;
    graphics_register_glyph(context->graphics, glyph);
    free(glyph);

    /* Set up pointer handling */
    pointer = calloc(1, sizeof(rdpPointer));
    pointer->size = sizeof(guac_rdp_pointer);
    pointer->New = guac_rdp_pointer_new;
    pointer->Free = guac_rdp_pointer_free;
    pointer->Set = guac_rdp_pointer_set;
#ifdef HAVE_RDPPOINTER_SETNULL
    pointer->SetNull = guac_rdp_pointer_set_null;
#endif
#ifdef HAVE_RDPPOINTER_SETDEFAULT
    pointer->SetDefault = guac_rdp_pointer_set_default;
#endif
    graphics_register_pointer(context->graphics, pointer);
    free(pointer);

    /* Set up GDI */
    instance->update->DesktopResize = guac_rdp_gdi_desktop_resize;
    instance->update->EndPaint = guac_rdp_gdi_end_paint;
    instance->update->Palette = guac_rdp_gdi_palette_update;
    instance->update->SetBounds = guac_rdp_gdi_set_bounds;

    primary = instance->update->primary;
    primary->DstBlt = guac_rdp_gdi_dstblt;
    primary->PatBlt = guac_rdp_gdi_patblt;
    primary->ScrBlt = guac_rdp_gdi_scrblt;
    primary->MemBlt = guac_rdp_gdi_memblt;
    primary->OpaqueRect = guac_rdp_gdi_opaquerect;

    pointer_cache_register_callbacks(instance->update);
    glyph_cache_register_callbacks(instance->update);
    brush_cache_register_callbacks(instance->update);
    bitmap_cache_register_callbacks(instance->update);
    offscreen_cache_register_callbacks(instance->update);
    palette_cache_register_callbacks(instance->update);

    /* Init channels (pre-connect) */
    if (freerdp_channels_pre_connect(channels, instance)) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR, "Error initializing RDP client channel manager");
        return FALSE;
    }

    return TRUE;

}

BOOL rdp_freerdp_post_connect(freerdp* instance) {

    rdpContext* context = instance->context;
    guac_client* client = ((rdp_freerdp_context*) context)->client;
    rdpChannels* channels = instance->context->channels;

    /* Init channels (post-connect) */
    if (freerdp_channels_post_connect(channels, instance)) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR, "Error initializing RDP client channel manager");
        return FALSE;
    }

    return TRUE;

}

BOOL rdp_freerdp_authenticate(freerdp* instance, char** username,
        char** password, char** domain) {

    rdpContext* context = instance->context;
    guac_client* client = ((rdp_freerdp_context*) context)->client;

    /* Warn if connection is likely to fail due to lack of credentials */
    guac_client_log(client, GUAC_LOG_INFO,
            "Authentication requested but username or password not given");
    return TRUE;

}

BOOL rdp_freerdp_verify_certificate(freerdp* instance, char* subject,
        char* issuer, char* fingerprint) {

    rdpContext* context = instance->context;
    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_rdp_client* rdp_client =
        (guac_rdp_client*) client->data;

    /* Bypass validation if ignore_certificate given */
    if (rdp_client->settings->ignore_certificate) {
        guac_client_log(client, GUAC_LOG_INFO, "Certificate validation bypassed");
        return TRUE;
    }

    guac_client_log(client, GUAC_LOG_INFO, "Certificate validation failed");
    return FALSE;

}

void rdp_freerdp_context_new(freerdp* instance, rdpContext* context) {
    context->channels = freerdp_channels_new();
}

void rdp_freerdp_context_free(freerdp* instance, rdpContext* context) {
    /* EMPTY */
}

void __guac_rdp_client_load_keymap(guac_client* client,
        const guac_rdp_keymap* keymap) {

    guac_rdp_client* rdp_client =
        (guac_rdp_client*) client->data;

    /* Get mapping */
    const guac_rdp_keysym_desc* mapping = keymap->mapping;

    /* If parent exists, load parent first */
    if (keymap->parent != NULL)
        __guac_rdp_client_load_keymap(client, keymap->parent);

    /* Log load */
    guac_client_log(client, GUAC_LOG_INFO, "Loading keymap \"%s\"", keymap->name);

    /* Load mapping into keymap */
    while (mapping->keysym != 0) {

        /* Copy mapping */
        GUAC_RDP_KEYSYM_LOOKUP(rdp_client->keymap, mapping->keysym) =
            *mapping;

        /* Next keysym */
        mapping++;

    }

}

/**
 * Waits for messages from the RDP server for the given number of microseconds.
 *
 * @param client The client associated with the current RDP session.
 * @param timeout_usecs The maximum amount of time to wait, in microseconds.
 *
 * @return
 *     A positive value if messages are ready, zero if the specified timeout
 *     period elapsed, or a negative value if an error occurs.
 */
static int rdp_guac_client_wait_for_messages(guac_client* client,
        int timeout_usecs) {

    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    freerdp* rdp_inst = rdp_client->rdp_inst;
    rdpChannels* channels = rdp_inst->context->channels;

    int result;
    int index;
    int max_fd, fd;
    void* read_fds[32];
    void* write_fds[32];
    int read_count = 0;
    int write_count = 0;
    fd_set rfds, wfds;

    struct timeval timeout = {
        .tv_sec  = 0,
        .tv_usec = timeout_usecs
    };

    /* Get RDP fds */
    if (!freerdp_get_fds(rdp_inst, read_fds, &read_count, write_fds, &write_count)) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR, "Unable to read RDP file descriptors.");
        return -1;
    }

    /* Get channel fds */
    if (!freerdp_channels_get_fds(channels, rdp_inst, read_fds, &read_count, write_fds,
                &write_count)) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR, "Unable to read RDP channel file descriptors.");
        return -1;
    }

    /* Construct read fd_set */
    max_fd = 0;
    FD_ZERO(&rfds);
    for (index = 0; index < read_count; index++) {
        fd = (int)(long) (read_fds[index]);
        if (fd > max_fd)
            max_fd = fd;
        FD_SET(fd, &rfds);
    }

    /* Construct write fd_set */
    FD_ZERO(&wfds);
    for (index = 0; index < write_count; index++) {
        fd = (int)(long) (write_fds[index]);
        if (fd > max_fd)
            max_fd = fd;
        FD_SET(fd, &wfds);
    }

    /* If no file descriptors, error */
    if (max_fd == 0) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR, "No file descriptors associated with RDP connection.");
        return -1;
    }

    /* Wait for all RDP file descriptors */
    result = select(max_fd + 1, &rfds, &wfds, NULL, &timeout);
    if (result < 0) {

        /* If error ignorable, pretend timout occurred */
        if (errno == EAGAIN
            || errno == EWOULDBLOCK
            || errno == EINPROGRESS
            || errno == EINTR)
            return 0;

        /* Otherwise, return as error */
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR, "Error waiting for file descriptor.");
        return -1;

    }

    /* Return wait result */
    return result;

}

void* guac_rdp_client_thread(void* data) {

    guac_client* client = (guac_client*) data;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    guac_rdp_settings* settings = rdp_client->settings;

    /* Init random number generator */
    srandom(time(NULL));

    /* Create display */
    rdp_client->display = guac_common_display_alloc(client,
            rdp_client->settings->width,
            rdp_client->settings->height);

    rdp_client->current_surface = rdp_client->display->default_surface;

#ifdef HAVE_FREERDP_CHANNELS_GLOBAL_INIT
    freerdp_channels_global_init();
#endif

    /* Init client */
    freerdp* rdp_inst = freerdp_new();
    rdp_inst->PreConnect = rdp_freerdp_pre_connect;
    rdp_inst->PostConnect = rdp_freerdp_post_connect;
    rdp_inst->Authenticate = rdp_freerdp_authenticate;
    rdp_inst->VerifyCertificate = rdp_freerdp_verify_certificate;
    rdp_inst->ReceiveChannelData = __guac_receive_channel_data;

    /* Allocate FreeRDP context */
#ifdef LEGACY_FREERDP
    rdp_inst->context_size = sizeof(rdp_freerdp_context);
#else
    rdp_inst->ContextSize = sizeof(rdp_freerdp_context);
#endif
    rdp_inst->ContextNew  = (pContextNew) rdp_freerdp_context_new;
    rdp_inst->ContextFree = (pContextFree) rdp_freerdp_context_free;

    freerdp_context_new(rdp_inst);
    ((rdp_freerdp_context*) rdp_inst->context)->client = client;

    /* Load keymap into client */
    __guac_rdp_client_load_keymap(client, settings->server_layout);

#ifdef ENABLE_COMMON_SSH
    guac_common_ssh_init(client);

    /* Connect via SSH if SFTP is enabled */
    if (settings->enable_sftp) {

        /* Abort if username is missing */
        if (settings->sftp_username == NULL)
            return NULL;

        guac_client_log(client, GUAC_LOG_DEBUG,
                "Connecting via SSH for SFTP filesystem access.");

        rdp_client->sftp_user =
            guac_common_ssh_create_user(settings->sftp_username);

        /* Import private key, if given */
        if (settings->sftp_private_key != NULL) {

            guac_client_log(client, GUAC_LOG_DEBUG,
                    "Authenticating with private key.");

            /* Abort if private key cannot be read */
            if (guac_common_ssh_user_import_key(rdp_client->sftp_user,
                        settings->sftp_private_key,
                        settings->sftp_passphrase)) {
                guac_common_ssh_destroy_user(rdp_client->sftp_user);
                return NULL;
            }

        }

        /* Otherwise, use specified password */
        else {

            guac_client_log(client, GUAC_LOG_DEBUG,
                    "Authenticating with password.");

            guac_common_ssh_user_set_password(rdp_client->sftp_user,
                    settings->sftp_password);

        }

        /* Attempt SSH connection */
        rdp_client->sftp_session =
            guac_common_ssh_create_session(client, settings->sftp_hostname,
                    settings->sftp_port, rdp_client->sftp_user);

        /* Fail if SSH connection does not succeed */
        if (rdp_client->sftp_session == NULL) {
            /* Already aborted within guac_common_ssh_create_session() */
            guac_common_ssh_destroy_user(rdp_client->sftp_user);
            return NULL;
        }

        /* Load and expose filesystem */
        rdp_client->sftp_filesystem =
            guac_common_ssh_create_sftp_filesystem(
                    rdp_client->sftp_session, "/");

        /* Expose filesystem to connection owner */
        guac_client_for_owner(client,
                guac_common_ssh_expose_sftp_filesystem,
                rdp_client->sftp_filesystem);

        /* Abort if SFTP connection fails */
        if (rdp_client->sftp_filesystem == NULL) {
            guac_common_ssh_destroy_session(rdp_client->sftp_session);
            guac_common_ssh_destroy_user(rdp_client->sftp_user);
            return NULL;
        }

        guac_client_log(client, GUAC_LOG_DEBUG,
                "SFTP connection succeeded.");

    }
#endif

    /* Send connection name */
    guac_protocol_send_name(client->socket, settings->hostname);

    /* Set default pointer */
    guac_common_cursor_set_pointer(rdp_client->display->cursor);

    /* Push desired settings to FreeRDP */
    guac_rdp_push_settings(settings, rdp_inst);

    /* Connect to RDP server */
    if (!freerdp_connect(rdp_inst)) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_UPSTREAM_ERROR,
                "Error connecting to RDP server");
        return NULL;
    }

    /* Connection complete */
    rdp_client->rdp_inst = rdp_inst;

    rdpChannels* channels = rdp_inst->context->channels;

    /* Handle messages from RDP server while client is running */
    while (client->state == GUAC_CLIENT_RUNNING) {

#ifdef HAVE_FREERDP_DISPLAY_UPDATE_SUPPORT
        /* Update remote display size */
        pthread_mutex_lock(&(rdp_client->rdp_lock));
        guac_rdp_disp_update_size(rdp_client->disp, rdp_inst->context);
        pthread_mutex_unlock(&(rdp_client->rdp_lock));
#endif

        /* Wait for data and construct a reasonable frame */
        int wait_result = rdp_guac_client_wait_for_messages(client, 250000);
        guac_timestamp frame_start = guac_timestamp_current();
        while (wait_result > 0) {

            guac_timestamp frame_end;
            int frame_remaining;

            pthread_mutex_lock(&(rdp_client->rdp_lock));

            /* Check the libfreerdp fds */
            if (!freerdp_check_fds(rdp_inst)) {
                guac_client_log(client, GUAC_LOG_DEBUG,
                        "Error handling RDP file descriptors");
                pthread_mutex_unlock(&(rdp_client->rdp_lock));
                return NULL;
            }

            /* Check channel fds */
            if (!freerdp_channels_check_fds(channels, rdp_inst)) {
                guac_client_log(client, GUAC_LOG_DEBUG,
                        "Error handling RDP channel file descriptors");
                pthread_mutex_unlock(&(rdp_client->rdp_lock));
                return NULL;
            }

            /* Check for channel events */
            wMessage* event = freerdp_channels_pop_event(channels);
            if (event) {

                /* Handle channel events (clipboard and RAIL) */
#ifdef LEGACY_EVENT
                if (event->event_class == CliprdrChannel_Class)
                    guac_rdp_process_cliprdr_event(client, event);
                else if (event->event_class == RailChannel_Class)
                    guac_rdp_process_rail_event(client, event);
#else
                if (GetMessageClass(event->id) == CliprdrChannel_Class)
                    guac_rdp_process_cliprdr_event(client, event);
                else if (GetMessageClass(event->id) == RailChannel_Class)
                    guac_rdp_process_rail_event(client, event);
#endif

                freerdp_event_free(event);

            }

            /* Handle RDP disconnect */
            if (freerdp_shall_disconnect(rdp_inst)) {
                guac_client_log(client, GUAC_LOG_INFO,
                        "RDP server closed connection");
                pthread_mutex_unlock(&(rdp_client->rdp_lock));
                return NULL;
            }

            pthread_mutex_unlock(&(rdp_client->rdp_lock));

            /* Calculate time remaining in frame */
            frame_end = guac_timestamp_current();
            frame_remaining = frame_start + GUAC_RDP_FRAME_DURATION - frame_end;

            /* Wait again if frame remaining */
            if (frame_remaining > 0)
                wait_result = rdp_guac_client_wait_for_messages(client,
                        GUAC_RDP_FRAME_TIMEOUT*1000);
            else
                break;

        }

        /* If an error occurred, fail */
        if (wait_result < 0)
            guac_client_abort(client, GUAC_PROTOCOL_STATUS_UPSTREAM_ERROR,
                    "Connection closed.");

        /* End of frame */
        guac_common_display_flush(rdp_client->display);
        guac_client_end_frame(client);
        guac_socket_flush(client->socket);

    }

    guac_client_log(client, GUAC_LOG_INFO, "Internal RDP client disconnected");
    return NULL;

}

