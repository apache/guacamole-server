
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
#include <freerdp/chanman.h>
#include <freerdp/constants_ui.h>

#include <guacamole/log.h>
#include <guacamole/guacio.h>
#include <guacamole/protocol.h>
#include <guacamole/client.h>

#include "rdp_handlers.h"
#include "rdp_client.h"
#include "rdp_keymap.h"

/* Client plugin arguments */
const char* GUAC_CLIENT_ARGS[] = {
    "hostname",
    "port",
    NULL
};

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

int rdp_guac_client_handle_messages(guac_client* client) {

    rdp_guac_client_data* guac_client_data = (rdp_guac_client_data*) client->data;
    rdpInst* rdp_inst = guac_client_data->rdp_inst;
    rdpChanMan* chanman = guac_client_data->chanman;

    int index;
    int max_fd, fd;
    void* read_fds[32];
    void* write_fds[32];
    int read_count = 0;
    int write_count = 0;

    fd_set rfds, wfds;

    /* get rdp fds */
    if (rdp_inst->rdp_get_fds(rdp_inst, read_fds, &read_count, write_fds, &write_count) != 0) {
        guac_log_error("Unable to read RDP file descriptors.");
        return 1;
    }

    /* get channel fds */
    if (freerdp_chanman_get_fds(chanman, rdp_inst, read_fds, &read_count, write_fds, &write_count) != 0) {
        guac_log_error("Unable to read RDP channel file descriptors.");
        return 1;
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
        guac_log_error("No file descriptors");
        return 1;
    }

    /* Otherwise, wait for file descriptors given */
    if (select(max_fd + 1, &rfds, &wfds, NULL, NULL) == -1) {
        /* these are not really errors */
        if (!((errno == EAGAIN) ||
            (errno == EWOULDBLOCK) ||
            (errno == EINPROGRESS) ||
            (errno == EINTR))) /* signal occurred */
        {
            guac_log_error("Error waiting for file descriptor.");
            return 1;
        }
    }

    /* check the libfreerdp fds */
    if (rdp_inst->rdp_check_fds(rdp_inst) != 0) {
        guac_log_error("Error handling RDP file descriptors.");
        return 1;
    }

    /* check channel fds */
    if (freerdp_chanman_check_fds(chanman, rdp_inst) != 0) {
        guac_log_error("Error handling RDP channel file descriptors.");
        return 1;
    }

    /* Success */
    return 0;

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
	settings->performanceflags =
		PERF_DISABLE_WALLPAPER
        | PERF_DISABLE_FULLWINDOWDRAG
        | PERF_DISABLE_MENUANIMATIONS;
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
    guac_client_data->mouse_button_mask = 0;
    guac_client_data->current_surface = GUAC_DEFAULT_LAYER;

    /* Store client data */
    rdp_inst->param1 = client;
    client->data = guac_client_data;

    /* RDP handlers */
    rdp_inst->ui_error = guac_rdp_ui_error;
	rdp_inst->ui_warning = guac_rdp_ui_warning;
	rdp_inst->ui_unimpl = guac_rdp_ui_unimpl;
	rdp_inst->ui_begin_update = guac_rdp_ui_begin_update;
	rdp_inst->ui_end_update = guac_rdp_ui_end_update;
	rdp_inst->ui_desktop_save = guac_rdp_ui_desktop_save;
	rdp_inst->ui_desktop_restore = guac_rdp_ui_desktop_restore;
	rdp_inst->ui_create_bitmap = guac_rdp_ui_create_bitmap;
	rdp_inst->ui_paint_bitmap = guac_rdp_ui_paint_bitmap;
	rdp_inst->ui_destroy_bitmap = guac_rdp_ui_destroy_bitmap;
	rdp_inst->ui_line = guac_rdp_ui_line;
	rdp_inst->ui_rect = guac_rdp_ui_rect;
	rdp_inst->ui_polygon = guac_rdp_ui_polygon;
	rdp_inst->ui_polyline = guac_rdp_ui_polyline;
	rdp_inst->ui_ellipse = guac_rdp_ui_ellipse;
	rdp_inst->ui_start_draw_glyphs = guac_rdp_ui_start_draw_glyphs;
	rdp_inst->ui_draw_glyph = guac_rdp_ui_draw_glyph;
	rdp_inst->ui_end_draw_glyphs = guac_rdp_ui_end_draw_glyphs;
	rdp_inst->ui_get_toggle_keys_state = guac_rdp_ui_get_toggle_keys_state;
	rdp_inst->ui_bell = guac_rdp_ui_bell;
	rdp_inst->ui_destblt = guac_rdp_ui_destblt;
	rdp_inst->ui_patblt = guac_rdp_ui_patblt;
	rdp_inst->ui_screenblt = guac_rdp_ui_screenblt;
	rdp_inst->ui_memblt = guac_rdp_ui_memblt;
	rdp_inst->ui_triblt = guac_rdp_ui_triblt;
	rdp_inst->ui_create_glyph = guac_rdp_ui_create_glyph;
	rdp_inst->ui_destroy_glyph = guac_rdp_ui_destroy_glyph;
    rdp_inst->ui_select = guac_rdp_ui_select;
	rdp_inst->ui_set_clip = guac_rdp_ui_set_clip;
	rdp_inst->ui_reset_clip = guac_rdp_ui_reset_clip;
	rdp_inst->ui_resize_window = guac_rdp_ui_resize_window;
	rdp_inst->ui_set_cursor = guac_rdp_ui_set_cursor;
	rdp_inst->ui_destroy_cursor = guac_rdp_ui_destroy_cursor;
	rdp_inst->ui_create_cursor = guac_rdp_ui_create_cursor;
	rdp_inst->ui_set_null_cursor = guac_rdp_ui_set_null_cursor;
	rdp_inst->ui_set_default_cursor = guac_rdp_ui_set_default_cursor;
	rdp_inst->ui_create_colormap = guac_rdp_ui_create_colormap;
	rdp_inst->ui_move_pointer = guac_rdp_ui_move_pointer;
	rdp_inst->ui_set_colormap = guac_rdp_ui_set_colormap;
	rdp_inst->ui_create_surface = guac_rdp_ui_create_surface;
	rdp_inst->ui_set_surface = guac_rdp_ui_set_surface;
	rdp_inst->ui_destroy_surface = guac_rdp_ui_destroy_surface;
	rdp_inst->ui_channel_data = guac_rdp_ui_channel_data;

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
    client->handle_messages = rdp_guac_client_handle_messages;
    client->mouse_handler = rdp_guac_client_mouse_handler;
    client->key_handler = rdp_guac_client_key_handler;

    /* Success */
    return 0;

}

int rdp_guac_client_mouse_handler(guac_client* client, int x, int y, int mask) {

    rdp_guac_client_data* guac_client_data = (rdp_guac_client_data*) client->data;
    rdpInst* rdp_inst = guac_client_data->rdp_inst;

    /* If button mask unchanged, just send move event */
    if (mask == guac_client_data->mouse_button_mask)
        rdp_inst->rdp_send_input(rdp_inst, RDP_INPUT_MOUSE, PTRFLAGS_MOVE, x, y);

    /* Otherwise, send events describing button change */
    else {

        /* Release event */
        if (mask == 0)
            rdp_inst->rdp_send_input(rdp_inst, RDP_INPUT_MOUSE, PTRFLAGS_BUTTON1, x, y);

        /* Press event */
        else
            rdp_inst->rdp_send_input(rdp_inst, RDP_INPUT_MOUSE, PTRFLAGS_DOWN | PTRFLAGS_BUTTON1, x, y);

        guac_client_data->mouse_button_mask = mask;
    }

    return 0;
}

int rdp_guac_client_key_handler(guac_client* client, int keysym, int pressed) {

    rdp_guac_client_data* guac_client_data = (rdp_guac_client_data*) client->data;
    rdpInst* rdp_inst = guac_client_data->rdp_inst;

    /* If keysym can be in lookup table */
    if (keysym <= 0xFFFF) {

        /* Look up scancode */
        const guac_rdp_keymap* keymap = 
            &guac_rdp_keysym_scancode[(keysym & 0xFF00) >> 8][keysym & 0xFF];

        /* If defined, send event */
        if (keymap->scancode != 0)
            rdp_inst->rdp_send_input(
                    rdp_inst, RDP_INPUT_SCANCODE,
                    pressed ? RDP_KEYPRESS : RDP_KEYRELEASE,
                    keymap->scancode, 
                    keymap->flags);

    }

    return 0;
}

