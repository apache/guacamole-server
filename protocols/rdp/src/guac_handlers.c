
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
 * Matt Hortman
 * Jocelyn DELALANDE <j.delalande@ulteo.com> Ulteo SAS - http://www.ulteo.com
 *
 * Portions created by Ulteo SAS employees are Copyright (C) 2012 Ulteo SAS
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
#include <freerdp/channels/channels.h>
#include <freerdp/input.h>
#include <freerdp/codec/color.h>
#include <freerdp/cache/cache.h>
#include <freerdp/utils/event.h>
#include <freerdp/plugins/cliprdr.h>

#include <guacamole/socket.h>
#include <guacamole/protocol.h>
#include <guacamole/client.h>
#include <guacamole/error.h>

#include "client.h"
#include "rdp_keymap.h"
#include "rdp_cliprdr.h"
#include "guac_handlers.h"

void __guac_rdp_update_keysyms(guac_client* client, const int* keysym_string, int from, int to);
int __guac_rdp_send_keysym(guac_client* client, int keysym, int pressed);
void __guac_rdp_send_altcode(guac_client* client, int altcode);


int rdp_guac_client_free_handler(guac_client* client) {

    rdp_guac_client_data* guac_client_data =
        (rdp_guac_client_data*) client->data;

    freerdp* rdp_inst = guac_client_data->rdp_inst;
    rdpChannels* channels = rdp_inst->context->channels;

    /* Clean up RDP client */
	freerdp_channels_close(channels, rdp_inst);
	freerdp_channels_free(channels);
	freerdp_disconnect(rdp_inst);
    freerdp_clrconv_free(((rdp_freerdp_context*) rdp_inst->context)->clrconv);
    cache_free(rdp_inst->context->cache);
    freerdp_free(rdp_inst);

    /* Free client data */
    cairo_surface_destroy(guac_client_data->opaque_glyph_surface);
    cairo_surface_destroy(guac_client_data->trans_glyph_surface);
    free(guac_client_data->clipboard);
    free(guac_client_data);

    return 0;

}

int rdp_guac_client_handle_messages(guac_client* client) {

    rdp_guac_client_data* guac_client_data = (rdp_guac_client_data*) client->data;
    freerdp* rdp_inst = guac_client_data->rdp_inst;
    rdpChannels* channels = rdp_inst->context->channels;

    int index;
    int max_fd, fd;
    void* read_fds[32];
    void* write_fds[32];
    int read_count = 0;
    int write_count = 0;
    fd_set rfds, wfds;
    RDP_EVENT* event;

    struct timeval timeout = {
        .tv_sec = 0,
        .tv_usec = 250000
    };

    /* get rdp fds */
    if (!freerdp_get_fds(rdp_inst, read_fds, &read_count, write_fds, &write_count)) {
        guac_error = GUAC_STATUS_BAD_STATE;
        guac_error_message = "Unable to read RDP file descriptors";
        return 1;
    }

    /* get channel fds */
    if (!freerdp_channels_get_fds(channels, rdp_inst, read_fds, &read_count, write_fds, &write_count)) {
        guac_error = GUAC_STATUS_BAD_STATE;
        guac_error_message = "Unable to read RDP channel file descriptors";
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
        guac_error = GUAC_STATUS_BAD_STATE;
        guac_error_message = "No file descriptors";
        return 1;
    }

    /* Otherwise, wait for file descriptors given */
    if (select(max_fd + 1, &rfds, &wfds, NULL, &timeout) == -1) {
        /* these are not really errors */
        if (!((errno == EAGAIN) ||
            (errno == EWOULDBLOCK) ||
            (errno == EINPROGRESS) ||
            (errno == EINTR))) /* signal occurred */
        {
            guac_error = GUAC_STATUS_SEE_ERRNO;
            guac_error_message = "Error waiting for file descriptor";
            return 1;
        }
    }

    /* Check the libfreerdp fds */
    if (!freerdp_check_fds(rdp_inst)) {
        guac_error = GUAC_STATUS_BAD_STATE;
        guac_error_message = "Error handling RDP file descriptors";
        return 1;
    }

    /* Check channel fds */
    if (!freerdp_channels_check_fds(channels, rdp_inst)) {
        guac_error = GUAC_STATUS_BAD_STATE;
        guac_error_message = "Error handling RDP channel file descriptors";
        return 1;
    }

    /* Check for channel events */
    event = freerdp_channels_pop_event(channels);
    if (event) {

        /* Handle clipboard events */
        if (event->event_class == RDP_EVENT_CLASS_CLIPRDR)
            guac_rdp_process_cliprdr_event(client, event);

        freerdp_event_free(event);

    }

    /* Handle RDP disconnect */
    if (freerdp_shall_disconnect(rdp_inst)) {
        guac_error = GUAC_STATUS_NO_INPUT;
        guac_error_message = "RDP server closed connection";
        return 1;
    }

    /* Success */
    return 0;

}

int rdp_guac_client_mouse_handler(guac_client* client, int x, int y, int mask) {

    rdp_guac_client_data* guac_client_data = (rdp_guac_client_data*) client->data;
    freerdp* rdp_inst = guac_client_data->rdp_inst;

    /* If button mask unchanged, just send move event */
    if (mask == guac_client_data->mouse_button_mask)
        rdp_inst->input->MouseEvent(rdp_inst->input, PTR_FLAGS_MOVE, x, y);

    /* Otherwise, send events describing button change */
    else {

        /* Mouse buttons which have JUST become released */
        int released_mask =  guac_client_data->mouse_button_mask & ~mask;

        /* Mouse buttons which have JUST become pressed */
        int pressed_mask  = ~guac_client_data->mouse_button_mask &  mask;

        /* Release event */
        if (released_mask & 0x07) {

            /* Calculate flags */
            int flags = 0;
            if (released_mask & 0x01) flags |= PTR_FLAGS_BUTTON1;
            if (released_mask & 0x02) flags |= PTR_FLAGS_BUTTON3;
            if (released_mask & 0x04) flags |= PTR_FLAGS_BUTTON2;

            rdp_inst->input->MouseEvent(rdp_inst->input, flags, x, y);

        }

        /* Press event */
        if (pressed_mask & 0x07) {

            /* Calculate flags */
            int flags = PTR_FLAGS_DOWN;
            if (pressed_mask & 0x01) flags |= PTR_FLAGS_BUTTON1;
            if (pressed_mask & 0x02) flags |= PTR_FLAGS_BUTTON3;
            if (pressed_mask & 0x04) flags |= PTR_FLAGS_BUTTON2;
            if (pressed_mask & 0x08) flags |= PTR_FLAGS_WHEEL | 0x78;
            if (pressed_mask & 0x10) flags |= PTR_FLAGS_WHEEL | PTR_FLAGS_WHEEL_NEGATIVE | 0x88;

            /* Send event */
            rdp_inst->input->MouseEvent(rdp_inst->input, flags, x, y);

        }

        /* Scroll event */
        if (pressed_mask & 0x18) {

            /* Down */
            if (pressed_mask & 0x08)
                rdp_inst->input->MouseEvent(
                        rdp_inst->input,
                        PTR_FLAGS_WHEEL | 0x78,
                        x, y);

            /* Up */
            if (pressed_mask & 0x10)
                rdp_inst->input->MouseEvent(
                        rdp_inst->input,
                        PTR_FLAGS_WHEEL | PTR_FLAGS_WHEEL_NEGATIVE | 0x88,
                        x, y);

        }


        guac_client_data->mouse_button_mask = mask;
    }

    return 0;
}

void __guac_rdp_send_altcode(guac_client* client, int altcode) {

    rdp_guac_client_data* guac_client_data = (rdp_guac_client_data*) client->data;
    freerdp* rdp_inst = guac_client_data->rdp_inst;
    int i;

    /* Lookup scancode for Alt */
    int alt = GUAC_RDP_KEYSYM_LOOKUP(
            guac_client_data->keymap,
            0xFFE9 /* Alt_L */).scancode;

    /* Release all pressed modifiers */
    __guac_rdp_update_keysyms(client, GUAC_KEYSYMS_ALL_MODIFIERS, 1, 0);

    /* Press Alt */
    rdp_inst->input->KeyboardEvent(rdp_inst->input, KBD_FLAGS_DOWN, alt);

    /* For each character in four-digit Alt-code ... */
    for (i=0; i<4; i++) {

        /* Get scancode of keypad digit */
        int scancode = GUAC_RDP_KEYSYM_LOOKUP(
                guac_client_data->keymap,
                0xFFB0 + (altcode / 1000)
        ).scancode;

        /* Press and release digit */
        rdp_inst->input->KeyboardEvent(rdp_inst->input, KBD_FLAGS_DOWN, scancode);
        rdp_inst->input->KeyboardEvent(rdp_inst->input, KBD_FLAGS_RELEASE, scancode);

        /* Shift digits left by one place */
        altcode = (altcode * 10) % 10000;

    }

    /* Release Alt */
    rdp_inst->input->KeyboardEvent(rdp_inst->input, KBD_FLAGS_RELEASE, alt);

    /* Press all originally pressed modifiers */
    __guac_rdp_update_keysyms(client, GUAC_KEYSYMS_ALL_MODIFIERS, 1, 1);

}

int __guac_rdp_send_keysym(guac_client* client, int keysym, int pressed) {

    rdp_guac_client_data* guac_client_data = (rdp_guac_client_data*) client->data;
    freerdp* rdp_inst = guac_client_data->rdp_inst;

    /* If keysym can be in lookup table */
    if (keysym <= 0xFFFF) {

        /* Look up scancode mapping */
        const guac_rdp_keysym_desc* keysym_desc =
            &GUAC_RDP_KEYSYM_LOOKUP(guac_client_data->keymap, keysym);

        /* If defined, send event */
        if (keysym_desc->scancode != 0) {

            /* If defined, send any prerequesite keys that must be set */
            if (keysym_desc->set_keysyms != NULL)
                __guac_rdp_update_keysyms(client, keysym_desc->set_keysyms, 0, 1);

            /* If defined, release any keys that must be cleared */
            if (keysym_desc->clear_keysyms != NULL)
                __guac_rdp_update_keysyms(client, keysym_desc->clear_keysyms, 1, 0);

            /* Send actual key */
            rdp_inst->input->KeyboardEvent(
                    rdp_inst->input,
                    keysym_desc->flags
                        | (pressed ? KBD_FLAGS_DOWN : KBD_FLAGS_RELEASE),
                    keysym_desc->scancode);

            guac_client_log_info(client, "Base flags are %d", keysym_desc->flags);

            /* If defined, release any keys that were originally released */
            if (keysym_desc->set_keysyms != NULL)
                __guac_rdp_update_keysyms(client, keysym_desc->set_keysyms, 0, 0);

            /* If defined, send any keys that were originally set */
            if (keysym_desc->clear_keysyms != NULL)
                __guac_rdp_update_keysyms(client, keysym_desc->clear_keysyms, 1, 1);

            return 0;

        }
    }

    /* Fall back to unicode events if undefined inside current keymap */

    /* Only send when key pressed - Unicode events do not have DOWN/RELEASE flags */
    if (pressed) {

        /* Translate keysym into codepoint */
        int codepoint;
        if (keysym <= 0xFF)
            codepoint = keysym;
        else
            codepoint = keysym & 0xFFFFFF;

        guac_client_log_info(client, "Translated keysym 0x%x to U+%04X", keysym, codepoint);

        /* Send Unicode event */
        rdp_inst->input->UnicodeKeyboardEvent(
                rdp_inst->input,
                0, codepoint);
    }
    
    else
        guac_client_log_info(client, "Ignoring key release (Unicode event)");

    return 0;
}

void __guac_rdp_update_keysyms(guac_client* client, const int* keysym_string, int from, int to) {

    rdp_guac_client_data* guac_client_data = (rdp_guac_client_data*) client->data;
    int keysym;

    /* Send all keysyms in string, NULL terminated */
    while ((keysym = *keysym_string) != 0) {

        /* Get current keysym state */
        int current_state = GUAC_RDP_KEYSYM_LOOKUP(guac_client_data->keysym_state, keysym);

        /* If key is currently in given state, send event for changing it to specified "to" state */
        if (current_state == from)
            __guac_rdp_send_keysym(client, *keysym_string, to);

        /* Next keysym */
        keysym_string++;

    }

}

int rdp_guac_client_key_handler(guac_client* client, int keysym, int pressed) {

    rdp_guac_client_data* guac_client_data = (rdp_guac_client_data*) client->data;

    /* Update keysym state */
    GUAC_RDP_KEYSYM_LOOKUP(guac_client_data->keysym_state, keysym) = pressed;

    return __guac_rdp_send_keysym(client, keysym, pressed);

}

int rdp_guac_client_clipboard_handler(guac_client* client, char* data) {

    rdpChannels* channels = 
        ((rdp_guac_client_data*) client->data)->rdp_inst->context->channels;

    RDP_CB_FORMAT_LIST_EVENT* format_list =
        (RDP_CB_FORMAT_LIST_EVENT*) freerdp_event_new(
            RDP_EVENT_CLASS_CLIPRDR,
            RDP_EVENT_TYPE_CB_FORMAT_LIST,
            NULL, NULL);

    /* Free existing data */
    free(((rdp_guac_client_data*) client->data)->clipboard);

    /* Store data in client */
    ((rdp_guac_client_data*) client->data)->clipboard = strdup(data);

    /* Notify server that text data is now available */
    format_list->formats = (uint32*) malloc(sizeof(uint32));
    format_list->formats[0] = CB_FORMAT_TEXT;
    format_list->num_formats = 1;

    freerdp_channels_send_event(channels, (RDP_EVENT*) format_list);

    return 0;

}

