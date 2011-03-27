
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

#include <freerdp/freerdp.h>
#include <freerdp/chanman.h>

#include <guacamole/log.h>
#include <guacamole/guacio.h>
#include <guacamole/protocol.h>
#include <guacamole/client.h>

#define RDP_DEFAULT_PORT 3389

/* Client plugin arguments */
const char* GUAC_CLIENT_ARGS[] = {
    "hostname",
    "port",
    NULL
};

typedef struct rdp_guac_client_data {

    rdpInst* rdp_inst;
    rdpChanMan* chanman;
	rdpSet* settings;

} rdp_guac_client_data;

int rdp_guac_client_free_handler(guac_client* client) {

    rdp_guac_client_data* guac_client_data = (rdp_guac_client_data*) client->data;

    /* Disconnect client */
    guac_client_data->rdp_inst->rdp_disconnect(guac_client_data->rdp_inst);

    /* Free RDP client */
    freerdp_free(guac_client_data->rdp_inst);
    freerdp_chanman_free(guac_client_data->chanman);
    free(guac_client_data->settings);

    /* Free guac client data */
    free(guac_client_data);

    return 0;

}

int guac_rdp_ui_select(rdpInst* inst, int socket) {
    return 1;
}

void guac_rdp_ui_error(rdpInst* inst, char* text) {

    guac_client* client = (guac_client*) inst->param1;
    GUACIO* io = client->io;

    guac_send_error(io, text);
    guac_flush(io);

}

int guac_client_init(guac_client* client, int argc, char** argv) {

    rdp_guac_client_data* guac_client_data;

    rdpInst* rdp_inst;
    rdpChanMan* chanman;
	rdpSet* settings;

    char* hostname;
    int port = RDP_DEFAULT_PORT;

    if (argc < 2) {
        guac_send_error(client->io, "Wrong argument count received.");
        guac_flush(client->io);
        return 1;
    }

    /* If port specified, use it */
    if (argv[1][0] != '\0')
        port = atoi(argv[1]);

    hostname = argv[0];

    /* Allocate client data */
    guac_client_data = malloc(sizeof(rdp_guac_client_data));

    /* Get channel manager */
    chanman = freerdp_chanman_new();
    guac_client_data->chanman = chanman;

    /* INIT SETTINGS */
    settings = malloc(sizeof(rdpSet));
	memset(settings, 0, sizeof(rdpSet));
    guac_client_data->settings = settings;

    /* Set hostname */
    strncpy(settings->hostname, hostname, sizeof(settings->hostname) - 1);

    /* Default size */
	settings->width = 1024;
	settings->height = 768;

	strncpy(settings->server, hostname, sizeof(settings->server));
	strcpy(settings->username, "guest");

	settings->tcp_port_rdp = port;
	settings->encryption = 1;
	settings->server_depth = 16;
	settings->bitmap_cache = 1;
	settings->bitmap_compression = 1;
	settings->desktop_save = 0;
	settings->rdp5_performanceflags =
		RDP5_NO_WALLPAPER | RDP5_NO_FULLWINDOWDRAG | RDP5_NO_MENUANIMATIONS;
	settings->off_screen_bitmaps = 1;
	settings->triblt = 0;
	settings->new_cursors = 1;
	settings->rdp_version = 5;

    /* Init client */
    rdp_inst = freerdp_new(settings);
    if (rdp_inst == NULL) {
        guac_send_error(client->io, "Error initializing RDP client");
        guac_flush(client->io);
        return 1;
    }
    guac_client_data->rdp_inst = rdp_inst;

    /* Store client data */
    rdp_inst->param1 = client;
    client->data = guac_client_data;

    /* RDP handlers */
    rdp_inst->ui_error = guac_rdp_ui_error;
	/* rdp_inst->ui_warning = PLACEHOLDER */
	/* rdp_inst->ui_unimpl = PLACEHOLDER */
	/* rdp_inst->ui_begin_update = PLACEHOLDER */
	/* rdp_inst->ui_end_update = PLACEHOLDER */
	/* rdp_inst->ui_desktop_save = PLACEHOLDER */
	/* rdp_inst->ui_desktop_restore = PLACEHOLDER */
	/* rdp_inst->ui_create_bitmap = PLACEHOLDER */
	/* rdp_inst->ui_paint_bitmap = PLACEHOLDER */
	/* rdp_inst->ui_destroy_bitmap = PLACEHOLDER */
	/* rdp_inst->ui_line = PLACEHOLDER */
	/* rdp_inst->ui_rect = PLACEHOLDER */
	/* rdp_inst->ui_polygon = PLACEHOLDER */
	/* rdp_inst->ui_polyline = PLACEHOLDER */
	/* rdp_inst->ui_ellipse = PLACEHOLDER */
	/* rdp_inst->ui_start_draw_glyphs = PLACEHOLDER */
	/* rdp_inst->ui_draw_glyph = PLACEHOLDER */
	/* rdp_inst->ui_end_draw_glyphs = PLACEHOLDER */
	/* rdp_inst->ui_get_toggle_keys_state = PLACEHOLDER */
	/* rdp_inst->ui_bell = PLACEHOLDER */
	/* rdp_inst->ui_destblt = PLACEHOLDER */
	/* rdp_inst->ui_patblt = PLACEHOLDER */
	/* rdp_inst->ui_screenblt = PLACEHOLDER */
	/* rdp_inst->ui_memblt = PLACEHOLDER */
	/* rdp_inst->ui_triblt = PLACEHOLDER */
	/* rdp_inst->ui_create_glyph = PLACEHOLDER */
	/* rdp_inst->ui_destroy_glyph = PLACEHOLDER */
    rdp_inst->ui_select = guac_rdp_ui_select;
	/* rdp_inst->ui_set_clip = PLACEHOLDER */
	/* rdp_inst->ui_reset_clip = PLACEHOLDER */
	/* rdp_inst->ui_resize_window = PLACEHOLDER */
	/* rdp_inst->ui_set_cursor = PLACEHOLDER */
	/* rdp_inst->ui_destroy_cursor = PLACEHOLDER */
	/* rdp_inst->ui_create_cursor = PLACEHOLDER */
	/* rdp_inst->ui_set_null_cursor = PLACEHOLDER */
	/* rdp_inst->ui_set_default_cursor = PLACEHOLDER */
	/* rdp_inst->ui_create_colourmap = PLACEHOLDER */
	/* rdp_inst->ui_move_pointer = PLACEHOLDER */
	/* rdp_inst->ui_set_colourmap = PLACEHOLDER */
	/* rdp_inst->ui_create_surface = PLACEHOLDER */
	/* rdp_inst->ui_set_surface = PLACEHOLDER */
	/* rdp_inst->ui_destroy_surface = PLACEHOLDER */
	/* rdp_inst->ui_channel_data = PLACEHOLDER */

    /* Init chanman (pre-connect) */
    if (freerdp_chanman_pre_connect(chanman, rdp_inst)) {
        guac_send_error(client->io, "Error initializing RDP client channel manager");
        guac_flush(client->io);
        return 1;
    }

    /* Connect to RDP server */
    if (rdp_inst->rdp_connect(rdp_inst)) {
        guac_send_error(client->io, "Error connecting to RDP server");
        guac_flush(client->io);
        return 1;
    }

    /* Init chanman (post-connect) */
    if (freerdp_chanman_post_connect(chanman, rdp_inst)) {
        guac_send_error(client->io, "Error initializing RDP client channel manager");
        guac_flush(client->io);
        return 1;
    }

    /* Client handlers */
    client->free_handler = rdp_guac_client_free_handler;

    /* STUB */
    guac_send_error(client->io, "STUB");
    return 1;

}

