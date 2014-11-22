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
#include "guac_handlers.h"
#include "guac_pointer_cursor.h"
#include "guac_string.h"
#include "rdp_bitmap.h"
#include "rdp_gdi.h"
#include "rdp_glyph.h"
#include "rdp_keymap.h"
#include "rdp_pointer.h"
#include "rdp_stream.h"
#include "rdp_svc.h"

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

#ifdef HAVE_FREERDP_CLIENT_CLIPRDR_H
#include <freerdp/client/cliprdr.h>
#else
#include "compat/client-cliprdr.h"
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

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Client plugin arguments */
const char* GUAC_CLIENT_ARGS[] = {
    "hostname",
    "port",
    "domain",
    "username",
    "password",
    "width",
    "height",
    "dpi",
    "initial-program",
    "color-depth",
    "disable-audio",
    "enable-printing",
    "enable-drive",
    "drive-path",
    "console",
    "console-audio",
    "server-layout",
    "security",
    "ignore-cert",
    "disable-auth",
    "remote-app",
    "remote-app-dir",
    "remote-app-args",
    "static-channels",
    NULL
};

enum RDP_ARGS_IDX {

    IDX_HOSTNAME,
    IDX_PORT,
    IDX_DOMAIN,
    IDX_USERNAME,
    IDX_PASSWORD,
    IDX_WIDTH,
    IDX_HEIGHT,
    IDX_DPI,
    IDX_INITIAL_PROGRAM,
    IDX_COLOR_DEPTH,
    IDX_DISABLE_AUDIO,
    IDX_ENABLE_PRINTING,
    IDX_ENABLE_DRIVE,
    IDX_DRIVE_PATH,
    IDX_CONSOLE,
    IDX_CONSOLE_AUDIO,
    IDX_SERVER_LAYOUT,
    IDX_SECURITY,
    IDX_IGNORE_CERT,
    IDX_DISABLE_AUTH,
    IDX_REMOTE_APP,
    IDX_REMOTE_APP_DIR,
    IDX_REMOTE_APP_ARGS,
    IDX_STATIC_CHANNELS,
    RDP_ARGS_COUNT
};

#if defined(FREERDP_VERSION_MAJOR) && (FREERDP_VERSION_MAJOR > 1 || FREERDP_VERSION_MINOR >= 2)
int __guac_receive_channel_data(freerdp* rdp_inst, UINT16 channelId, BYTE* data, int size, int flags, int total_size) {
#else
int __guac_receive_channel_data(freerdp* rdp_inst, int channelId, UINT8* data, int size, int flags, int total_size) {
#endif
    return freerdp_channels_data(rdp_inst, channelId, data, size, flags, total_size);
}

BOOL rdp_freerdp_pre_connect(freerdp* instance) {

    rdpContext* context = instance->context;
    guac_client* client = ((rdp_freerdp_context*) context)->client;
    rdpChannels* channels = context->channels;
    rdpBitmap* bitmap;
    rdpGlyph* glyph;
    rdpPointer* pointer;
    rdpPrimaryUpdate* primary;
    CLRCONV* clrconv;

    rdp_guac_client_data* guac_client_data =
        (rdp_guac_client_data*) client->data;

#ifdef HAVE_FREERDP_REGISTER_ADDIN_PROVIDER
    /* Init FreeRDP add-in provider */
    freerdp_register_addin_provider(freerdp_channels_load_static_addin_entry, 0);
#endif

    /* Load virtual channel management plugin */
    if (freerdp_channels_load_plugin(channels, instance->settings,
                "drdynvc", instance->settings))
        guac_client_log(client, GUAC_LOG_WARNING,
                "Failed to load drdynvc plugin.");

#ifdef HAVE_FREERDP_DISPLAY_UPDATE_SUPPORT
    /* Init display update plugin */
    guac_client_data->disp = NULL;
    guac_rdp_disp_load_plugin(instance->context);
#endif

    /* Load clipboard plugin */
    if (freerdp_channels_load_plugin(channels, instance->settings,
                "cliprdr", NULL))
        guac_client_log(client, GUAC_LOG_WARNING,
                "Failed to load cliprdr plugin. Clipboard will not work.");

    /* If audio enabled, choose an encoder */
    if (guac_client_data->settings.audio_enabled) {

        guac_client_data->audio = guac_audio_stream_alloc(client, NULL);

        /* If an encoding is available, load the sound plugin */
        if (guac_client_data->audio != NULL) {

            /* Load sound plugin */
            if (freerdp_channels_load_plugin(channels, instance->settings,
                        "guacsnd", guac_client_data->audio))
                guac_client_log(client, GUAC_LOG_WARNING,
                        "Failed to load guacsnd plugin. Audio will not work.");

        }
        else
            guac_client_log(client, GUAC_LOG_INFO,
                    "No available audio encoding. Sound disabled.");

    } /* end if audio enabled */

    /* Load filesystem if drive enabled */
    if (guac_client_data->settings.drive_enabled) {
        guac_client_data->filesystem =
            guac_rdp_fs_alloc(client, guac_client_data->settings.drive_path);
    }

    /* If RDPDR required, load it */
    if (guac_client_data->settings.printing_enabled
        || guac_client_data->settings.drive_enabled
        || guac_client_data->settings.audio_enabled) {

        /* Load RDPDR plugin */
        if (freerdp_channels_load_plugin(channels, instance->settings,
                    "guacdr", client))
            guac_client_log(client, GUAC_LOG_WARNING,
                    "Failed to load guacdr plugin. Drive redirection and printing will not work.");

    }

    /* Load RAIL plugin if RemoteApp in use */
    if (guac_client_data->settings.remote_app != NULL) {

#ifdef LEGACY_FREERDP
        RDP_PLUGIN_DATA* plugin_data = malloc(sizeof(RDP_PLUGIN_DATA) * 2);

        plugin_data[0].size = sizeof(RDP_PLUGIN_DATA);
        plugin_data[0].data[0] = guac_client_data->settings.remote_app;
        plugin_data[0].data[1] = guac_client_data->settings.remote_app_dir;
        plugin_data[0].data[2] = guac_client_data->settings.remote_app_args; 
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
    if (guac_client_data->settings.svc_names != NULL) {

        char** current = guac_client_data->settings.svc_names;
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

    /* Client handlers */
    client->free_handler = rdp_guac_client_free_handler;
    client->handle_messages = rdp_guac_client_handle_messages;
    client->mouse_handler = rdp_guac_client_mouse_handler;
    client->key_handler = rdp_guac_client_key_handler;
    client->size_handler = rdp_guac_client_size_handler;

    /* Stream handlers */
    client->clipboard_handler = guac_rdp_clipboard_handler;
    client->file_handler = guac_rdp_upload_file_handler;
    client->pipe_handler = guac_rdp_svc_pipe_handler;

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
    rdp_guac_client_data* guac_client_data =
        (rdp_guac_client_data*) client->data;

    /* Bypass validation if ignore_certificate given */
    if (guac_client_data->settings.ignore_certificate) {
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

    rdp_guac_client_data* guac_client_data =
        (rdp_guac_client_data*) client->data;

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
        GUAC_RDP_KEYSYM_LOOKUP(guac_client_data->keymap, mapping->keysym) =
            *mapping;

        /* Next keysym */
        mapping++;

    }

}

/**
 * Reduces the resolution of the client to the given resolution in DPI if
 * doing so is reasonable. This function returns non-zero if the resolution
 * was successfully reduced to the given DPI, and zero if reduction failed.
 */
static int __guac_rdp_reduce_resolution(guac_client* client, int resolution) {

    int width  = client->info.optimal_width  * resolution / client->info.optimal_resolution;
    int height = client->info.optimal_height * resolution / client->info.optimal_resolution;

    /* Reduced resolution if result is reasonably sized */
    if (width*height >= GUAC_RDP_REASONABLE_AREA) {
        client->info.optimal_width = width;
        client->info.optimal_height = height;
        client->info.optimal_resolution = resolution;
        guac_client_log(client, GUAC_LOG_INFO,
                "Reducing resolution to %i DPI (%ix%i)", resolution, width, height);
        return 1;
    }

    /* No reduction performed */
    return 0;

}

/**
 * Forces the client resolution to the given value, without checking whether
 * the resulting width and height are reasonable.
 */
static void __guac_rdp_force_resolution(guac_client* client, int resolution) {

    int width  = client->info.optimal_width  * resolution / client->info.optimal_resolution;
    int height = client->info.optimal_height * resolution / client->info.optimal_resolution;

    /* Do not force DPI to zero */
    if (resolution == 0) {
        guac_client_log(client, GUAC_LOG_WARNING,
                "Cowardly refusing to force resolution to %i DPI", resolution);
        return;
    }

    /* Reduced resolution if result is reasonably sized */
    client->info.optimal_width = width;
    client->info.optimal_height = height;
    client->info.optimal_resolution = resolution;
    guac_client_log(client, GUAC_LOG_INFO,
            "Resolution forced to %i DPI (%ix%i)", resolution, width, height);

}

int guac_client_init(guac_client* client, int argc, char** argv) {

    rdp_guac_client_data* guac_client_data;
    guac_rdp_settings* settings;

    freerdp* rdp_inst;

    /* Validate number of arguments received */
    if (argc != RDP_ARGS_COUNT) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR, "Wrong argument count received.");
        return 1;
    }

    /* Allocate client data */
    guac_client_data = malloc(sizeof(rdp_guac_client_data));

    /* Init random number generator */
    srandom(time(NULL));

    /* Init client */
#ifdef HAVE_FREERDP_CHANNELS_GLOBAL_INIT
    freerdp_channels_global_init();
#endif
    rdp_inst = freerdp_new();
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

    /* Set settings */
    settings = &(guac_client_data->settings);

    /* Console */
    settings->console         = (strcmp(argv[IDX_CONSOLE], "true") == 0);
    settings->console_audio   = (strcmp(argv[IDX_CONSOLE_AUDIO], "true") == 0);

    /* Certificate and auth */
    settings->ignore_certificate = (strcmp(argv[IDX_IGNORE_CERT], "true") == 0);
    settings->disable_authentication = (strcmp(argv[IDX_DISABLE_AUTH], "true") == 0);

    /* NLA security */
    if (strcmp(argv[IDX_SECURITY], "nla") == 0) {
        guac_client_log(client, GUAC_LOG_INFO, "Security mode: NLA");
        settings->security_mode = GUAC_SECURITY_NLA;
    }

    /* TLS security */
    else if (strcmp(argv[IDX_SECURITY], "tls") == 0) {
        guac_client_log(client, GUAC_LOG_INFO, "Security mode: TLS");
        settings->security_mode = GUAC_SECURITY_TLS;
    }

    /* RDP security */
    else if (strcmp(argv[IDX_SECURITY], "rdp") == 0) {
        guac_client_log(client, GUAC_LOG_INFO, "Security mode: RDP");
        settings->security_mode = GUAC_SECURITY_RDP;
    }

    /* ANY security (allow server to choose) */
    else if (strcmp(argv[IDX_SECURITY], "any") == 0) {
        guac_client_log(client, GUAC_LOG_INFO, "Security mode: ANY");
        settings->security_mode = GUAC_SECURITY_ANY;
    }

    /* If nothing given, default to RDP */
    else {
        guac_client_log(client, GUAC_LOG_INFO, "No security mode specified. Defaulting to RDP.");
        settings->security_mode = GUAC_SECURITY_RDP;
    }

    /* Set hostname */
    settings->hostname = strdup(argv[IDX_HOSTNAME]);

    /* If port specified, use it */
    settings->port = RDP_DEFAULT_PORT;
    if (argv[IDX_PORT][0] != '\0')
        settings->port = atoi(argv[IDX_PORT]);

    guac_client_log(client, GUAC_LOG_INFO, "Client resolution is %ix%i at %i DPI",
            client->info.optimal_width,
            client->info.optimal_height,
            client->info.optimal_resolution);

    /* Use optimal resolution unless overridden */
    if (argv[IDX_DPI][0] != '\0')
        __guac_rdp_force_resolution(client, atoi(argv[IDX_DPI]));

    /* If resolution not forced, attempt to reduce resolution for high DPI */
    else if (client->info.optimal_resolution > GUAC_RDP_NATIVE_RESOLUTION
            && !__guac_rdp_reduce_resolution(client, GUAC_RDP_NATIVE_RESOLUTION)
            && !__guac_rdp_reduce_resolution(client, GUAC_RDP_HIGH_RESOLUTION))
        guac_client_log(client, GUAC_LOG_INFO, "No reasonable lower resolution");

    /* Use optimal width unless overridden */
    settings->width = client->info.optimal_width;
    if (argv[IDX_WIDTH][0] != '\0')
        settings->width = atoi(argv[IDX_WIDTH]);

    /* Use default width if given width is invalid. */
    if (settings->width <= 0) {
        settings->width = RDP_DEFAULT_WIDTH;
        guac_client_log(client, GUAC_LOG_ERROR,
                "Invalid width: \"%s\". Using default of %i.",
                argv[IDX_WIDTH], settings->width);
    }

    /* Round width down to nearest multiple of 4 */
    settings->width = settings->width & ~0x3;

    /* Use optimal height unless overridden */
    settings->height = client->info.optimal_height;
    if (argv[IDX_HEIGHT][0] != '\0')
        settings->height = atoi(argv[IDX_HEIGHT]);

    /* Use default height if given height is invalid. */
    if (settings->height <= 0) {
        settings->height = RDP_DEFAULT_HEIGHT;
        guac_client_log(client, GUAC_LOG_ERROR,
                "Invalid height: \"%s\". Using default of %i.",
                argv[IDX_WIDTH], settings->height);
    }

    /* Domain */
    settings->domain = NULL;
    if (argv[IDX_DOMAIN][0] != '\0')
        settings->domain = strdup(argv[IDX_DOMAIN]);

    /* Username */
    settings->username = NULL;
    if (argv[IDX_USERNAME][0] != '\0')
        settings->username = strdup(argv[IDX_USERNAME]);

    /* Password */
    settings->password = NULL;
    if (argv[IDX_PASSWORD][0] != '\0')
        settings->password = strdup(argv[IDX_PASSWORD]);

    /* Initial program */
    settings->initial_program = NULL;
    if (argv[IDX_INITIAL_PROGRAM][0] != '\0')
        settings->initial_program = strdup(argv[IDX_INITIAL_PROGRAM]);

    /* RemoteApp program */
    settings->remote_app = NULL;
    if (argv[IDX_REMOTE_APP][0] != '\0')
        settings->remote_app = strdup(argv[IDX_REMOTE_APP]);

    /* RemoteApp working directory */
    settings->remote_app_dir = NULL;
    if (argv[IDX_REMOTE_APP_DIR][0] != '\0')
        settings->remote_app_dir = strdup(argv[IDX_REMOTE_APP_DIR]);

    /* RemoteApp arguments */
    settings->remote_app_args = NULL;
    if (argv[IDX_REMOTE_APP_ARGS][0] != '\0')
        settings->remote_app_args = strdup(argv[IDX_REMOTE_APP_ARGS]);

    /* Static virtual channels */
    settings->svc_names = NULL;
    if (argv[IDX_STATIC_CHANNELS][0] != '\0')
        settings->svc_names = guac_split(argv[IDX_STATIC_CHANNELS], ',');

    /* Session color depth */
    settings->color_depth = RDP_DEFAULT_DEPTH;
    if (argv[IDX_COLOR_DEPTH][0] != '\0')
        settings->color_depth = atoi(argv[IDX_COLOR_DEPTH]);

    /* Use default depth if given depth is invalid. */
    if (settings->color_depth == 0) {
        settings->color_depth = RDP_DEFAULT_DEPTH;
        guac_client_log(client, GUAC_LOG_ERROR,
                "Invalid color-depth: \"%s\". Using default of %i.",
                argv[IDX_WIDTH], settings->color_depth);
    }

    /* Audio enable/disable */
    guac_client_data->settings.audio_enabled =
        (strcmp(argv[IDX_DISABLE_AUDIO], "true") != 0);

    /* Printing enable/disable */
    guac_client_data->settings.printing_enabled =
        (strcmp(argv[IDX_ENABLE_PRINTING], "true") == 0);

    /* Drive enable/disable */
    guac_client_data->settings.drive_enabled =
        (strcmp(argv[IDX_ENABLE_DRIVE], "true") == 0);

    guac_client_data->settings.drive_path = strdup(argv[IDX_DRIVE_PATH]);

    /* Store client data */
    guac_client_data->rdp_inst = rdp_inst;
    guac_client_data->mouse_button_mask = 0;
    guac_client_data->clipboard = guac_common_clipboard_alloc(GUAC_RDP_CLIPBOARD_MAX_LENGTH);
    guac_client_data->requested_clipboard_format = CB_FORMAT_TEXT;
    guac_client_data->audio = NULL;
    guac_client_data->filesystem = NULL;
    guac_client_data->available_svc = guac_common_list_alloc();

    /* Main socket needs to be threadsafe */
    guac_socket_require_threadsafe(client->socket);

    /* Recursive attribute for locks */
    pthread_mutexattr_init(&(guac_client_data->attributes));
    pthread_mutexattr_settype(&(guac_client_data->attributes),
            PTHREAD_MUTEX_RECURSIVE);

    /* Init RDP lock */
    pthread_mutex_init(&(guac_client_data->rdp_lock),
           &(guac_client_data->attributes));

    /* Clear keysym state mapping and keymap */
    memset(guac_client_data->keysym_state, 0,
            sizeof(guac_rdp_keysym_state_map));

    memset(guac_client_data->keymap, 0,
            sizeof(guac_rdp_static_keymap));

    client->data = guac_client_data;
    ((rdp_freerdp_context*) rdp_inst->context)->client = client;

    /* Pick keymap based on argument */
    settings->server_layout = NULL;
    if (argv[IDX_SERVER_LAYOUT][0] != '\0')
        settings->server_layout =
            guac_rdp_keymap_find(argv[IDX_SERVER_LAYOUT]);

    /* If no keymap requested, use default */
    if (settings->server_layout == NULL)
        settings->server_layout = guac_rdp_keymap_find(GUAC_DEFAULT_KEYMAP);

    /* Load keymap into client */
    __guac_rdp_client_load_keymap(client, settings->server_layout);

    /* Create default surface */
    guac_client_data->default_surface = guac_common_surface_alloc(client->socket, GUAC_DEFAULT_LAYER,
                                                                  settings->width, settings->height);
    guac_client_data->current_surface = guac_client_data->default_surface;

    /* Send connection name */
    guac_protocol_send_name(client->socket, settings->hostname);

    /* Set default pointer */
    guac_common_set_pointer_cursor(client);

    /* Push desired settings to FreeRDP */
    guac_rdp_push_settings(settings, rdp_inst);

    /* Connect to RDP server */
    if (!freerdp_connect(rdp_inst)) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_UPSTREAM_ERROR, "Error connecting to RDP server");
        return 1;
    }

    /* Success */
    return 0;

}

