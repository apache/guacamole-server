
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
 * The Original Code is libguac-client-rdp.
 *
 * The Initial Developer of the Original Code is
 * Michael Jumper.
 * Portions created by the Initial Developer are Copyright (C) 2011
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

#include <sys/select.h>
#include <errno.h>

#include <freerdp/freerdp.h>
#include <freerdp/utils/memory.h>
#include <freerdp/cache/bitmap.h>
#include <freerdp/cache/brush.h>
#include <freerdp/cache/glyph.h>
#include <freerdp/cache/palette.h>
#include <freerdp/cache/pointer.h>
#include <freerdp/cache/offscreen.h>
#include <freerdp/channels/channels.h>
#include <freerdp/input.h>
#include <freerdp/constants.h>

#include <guacamole/socket.h>
#include <guacamole/protocol.h>
#include <guacamole/client.h>

#include "client.h"
#include "guac_handlers.h"
#include "rdp_keymap.h"
#include "rdp_bitmap.h"
#include "rdp_glyph.h"
#include "rdp_pointer.h"
#include "rdp_gdi.h"

/* Client plugin arguments */
const char* GUAC_CLIENT_ARGS[] = {
    "hostname",
    "port",
    NULL
};

boolean rdp_freerdp_pre_connect(freerdp* instance) {

    rdpContext* context = instance->context;
    guac_client* client = ((rdp_freerdp_context*) context)->client;
    rdpChannels* channels = context->channels;
    rdpBitmap* bitmap;
    rdpGlyph* glyph;
    rdpPointer* pointer;
    rdpPrimaryUpdate* primary;
    CLRCONV* clrconv;

    /* Init color conversion structure */
    clrconv = xnew(CLRCONV);
    clrconv->alpha = 1;
    clrconv->invert = 0;
    clrconv->rgb555 = 0;
    clrconv->palette = xnew(rdpPalette);
    ((rdp_freerdp_context*) context)->clrconv = clrconv;

    /* Init FreeRDP cache */
    instance->context->cache = cache_new(instance->settings);

    /* Set up bitmap handling */
    bitmap = xnew(rdpBitmap);
    bitmap->size = sizeof(guac_rdp_bitmap);
    bitmap->New = guac_rdp_bitmap_new;
    bitmap->Free = guac_rdp_bitmap_free;
    bitmap->Paint = guac_rdp_bitmap_paint;
    bitmap->Decompress = guac_rdp_bitmap_decompress;
    bitmap->SetSurface = guac_rdp_bitmap_setsurface;
    graphics_register_bitmap(context->graphics, bitmap);

    /* Set up glyph handling */
    glyph = xnew(rdpGlyph);
    glyph->size = sizeof(guac_rdp_glyph);
    glyph->New = guac_rdp_glyph_new;
    glyph->Free = guac_rdp_glyph_free;
    glyph->Draw = guac_rdp_glyph_draw;
    glyph->BeginDraw = guac_rdp_glyph_begindraw;
    glyph->EndDraw = guac_rdp_glyph_enddraw;
    graphics_register_glyph(context->graphics, glyph);

    /* Set up pointer handling */
    pointer = xnew(rdpPointer);
    pointer->size = sizeof(guac_rdp_pointer);
    pointer->New = guac_rdp_pointer_new;
    pointer->Free = guac_rdp_pointer_free;
    pointer->Set = guac_rdp_pointer_set;
    graphics_register_pointer(context->graphics, pointer);

    /* Set up GDI */
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
        guac_protocol_send_error(client->socket, "Error initializing RDP client channel manager");
        guac_socket_flush(client->socket);
        return false;
    }

    return true;

}

boolean rdp_freerdp_post_connect(freerdp* instance) {

    rdpContext* context = instance->context;
    guac_client* client = ((rdp_freerdp_context*) context)->client;
    rdpChannels* channels = instance->context->channels;

    /* Init channels (post-connect) */
    if (freerdp_channels_post_connect(channels, instance)) {
        guac_protocol_send_error(client->socket, "Error initializing RDP client channel manager");
        guac_socket_flush(client->socket);
        return false;
    }

    /* Client handlers */
    client->free_handler = rdp_guac_client_free_handler;
    client->handle_messages = rdp_guac_client_handle_messages;
    client->mouse_handler = rdp_guac_client_mouse_handler;
    client->key_handler = rdp_guac_client_key_handler;

    /* Send size */
    guac_protocol_send_size(client->socket,
            instance->settings->width, instance->settings->height);

    return true;

}

void rdp_freerdp_context_new(freerdp* instance, rdpContext* context) {
    context->channels = freerdp_channels_new();
}

void rdp_freerdp_context_free(freerdp* instance, rdpContext* context) {
    /* EMPTY */
}

int guac_client_init(guac_client* client, int argc, char** argv) {

    rdp_guac_client_data* guac_client_data;

    freerdp* rdp_inst;
	rdpSettings* settings;

    char* hostname;
    int port = RDP_DEFAULT_PORT;
    boolean bitmap_cache;

    if (argc < 2) {
        guac_protocol_send_error(client->socket, "Wrong argument count received.");
        guac_socket_flush(client->socket);
        return 1;
    }

    /* If port specified, use it */
    if (argv[1][0] != '\0')
        port = atoi(argv[1]);

    hostname = argv[0];

    /* Allocate client data */
    guac_client_data = malloc(sizeof(rdp_guac_client_data));

    /* Init client */
    freerdp_channels_global_init();
    rdp_inst = freerdp_new();
    rdp_inst->PreConnect = rdp_freerdp_pre_connect;
    rdp_inst->PostConnect = rdp_freerdp_post_connect;

    /* Allocate FreeRDP context */
    rdp_inst->context_size = sizeof(rdp_freerdp_context);
    rdp_inst->ContextNew  = (pContextNew) rdp_freerdp_context_new;
    rdp_inst->ContextFree = (pContextFree) rdp_freerdp_context_free;
    freerdp_context_new(rdp_inst);

    /* Set settings */
    settings = rdp_inst->settings;

    /* --no-auth */
    settings->authentication = false;

    /* --sec rdp */
    settings->rdp_security = true;
    settings->tls_security = false;
    settings->nla_security = false;
    settings->encryption = true;
    settings->encryption_method = ENCRYPTION_METHOD_40BIT | ENCRYPTION_METHOD_128BIT | ENCRYPTION_METHOD_FIPS;
    settings->encryption_level = ENCRYPTION_LEVEL_CLIENT_COMPATIBLE;

    /* Default size */
	settings->width = 1024;
	settings->height = 768;

    /* Set hostname */
    settings->hostname = strdup(hostname);
	settings->window_title = strdup(hostname);
	settings->username = "guest";

    /* Order support */
    bitmap_cache = settings->bitmap_cache;
    settings->os_major_type = OSMAJORTYPE_UNSPECIFIED;
    settings->os_minor_type = OSMINORTYPE_UNSPECIFIED;
    settings->order_support[NEG_DSTBLT_INDEX] = true;
    settings->order_support[NEG_PATBLT_INDEX] = true;
    settings->order_support[NEG_SCRBLT_INDEX] = true;
    settings->order_support[NEG_OPAQUE_RECT_INDEX] = true;
    settings->order_support[NEG_DRAWNINEGRID_INDEX] = false;
    settings->order_support[NEG_MULTIDSTBLT_INDEX] = false;
    settings->order_support[NEG_MULTIPATBLT_INDEX] = false;
    settings->order_support[NEG_MULTISCRBLT_INDEX] = false;
    settings->order_support[NEG_MULTIOPAQUERECT_INDEX] = false;
    settings->order_support[NEG_MULTI_DRAWNINEGRID_INDEX] = false;
    settings->order_support[NEG_LINETO_INDEX] = false;
    settings->order_support[NEG_POLYLINE_INDEX] = false;
    settings->order_support[NEG_MEMBLT_INDEX] = bitmap_cache;
    settings->order_support[NEG_MEM3BLT_INDEX] = false;
    settings->order_support[NEG_MEMBLT_V2_INDEX] = bitmap_cache;
    settings->order_support[NEG_MEM3BLT_V2_INDEX] = false;
    settings->order_support[NEG_SAVEBITMAP_INDEX] = false;
    settings->order_support[NEG_GLYPH_INDEX_INDEX] = false;
    settings->order_support[NEG_FAST_INDEX_INDEX] = false;
    settings->order_support[NEG_FAST_GLYPH_INDEX] = false;
    settings->order_support[NEG_POLYGON_SC_INDEX] = false;
    settings->order_support[NEG_POLYGON_CB_INDEX] = false;
    settings->order_support[NEG_ELLIPSE_SC_INDEX] = false;
    settings->order_support[NEG_ELLIPSE_CB_INDEX] = false;

    /* Store client data */
    guac_client_data->rdp_inst = rdp_inst;
    guac_client_data->mouse_button_mask = 0;
    guac_client_data->current_surface = GUAC_DEFAULT_LAYER;

    ((rdp_freerdp_context*) rdp_inst->context)->client = client;
    client->data = guac_client_data;

    /* Connect to RDP server */
    if (!freerdp_connect(rdp_inst)) {
        guac_protocol_send_error(client->socket, "Error connecting to RDP server");
        guac_socket_flush(client->socket);
        return 1;
    }

    /* Success */
    return 0;

}

