
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
#include <freerdp/channels/channels.h>
#include <freerdp/input.h>

#include <guacamole/socket.h>
#include <guacamole/protocol.h>
#include <guacamole/client.h>

#include "client.h"
#include "guac_handlers.h"
#include "rdp_keymap.h"
#include "rdp_bitmap.h"
#include "rdp_glyph.h"
#include "rdp_pointer.h"

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

    /* Set up bitmap handling */
    bitmap = xnew(rdpBitmap);
    bitmap->size = sizeof(guac_rdp_bitmap);
    bitmap->New = guac_rdp_bitmap_new;
    bitmap->Free = guac_rdp_bitmap_free;
    bitmap->Paint = guac_rdp_bitmap_paint;
    /* bitmap->Decompress = guac_rdp_bitmap_decompress; */
    bitmap->SetSurface = guac_rdp_bitmap_setsurface;
    graphics_register_bitmap(context->graphics, bitmap);

    /* Set up glyph handling */
    glyph = xnew(rdpGlyph);
    glyph->size = sizeof(guac_rdp_glyph);
    glyph->New = guac_rdp_glyph_new;
    glyph->Free = guac_rdp_glyph_free;
    glyph->Draw = guac_rdp_glyph_draw;
    graphics_register_glyph(context->graphics, glyph);

    /* Set up pointer handling */
    pointer = xnew(rdpPointer);
    pointer->size = sizeof(guac_rdp_pointer);
    pointer->New = guac_rdp_pointer_new;
    pointer->Free = guac_rdp_pointer_free;
    pointer->Set = guac_rdp_pointer_set;
    graphics_register_pointer(context->graphics, pointer);

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

