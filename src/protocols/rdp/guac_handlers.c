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
#include "guac_clipboard.h"
#include "guac_handlers.h"
#include "guac_list.h"
#include "guac_surface.h"
#include "rdp_cliprdr.h"
#include "rdp_keymap.h"
#include "rdp_fs.h"
#include "rdp_rail.h"
#include "rdp_stream.h"

#include <freerdp/cache/cache.h>
#include <freerdp/channels/channels.h>
#include <freerdp/codec/color.h>
#include <freerdp/freerdp.h>
#include <freerdp/input.h>
#include <freerdp/utils/event.h>
#include <guacamole/client.h>
#include <guacamole/error.h>
#include <guacamole/protocol.h>
#include <guacamole/timestamp.h>

#ifdef HAVE_FREERDP_CLIENT_CLIPRDR_H
#include <freerdp/client/cliprdr.h>
#else
#include "compat/client-cliprdr.h"
#endif

#ifdef LEGACY_FREERDP
#include "compat/rail.h"
#else
#include <freerdp/rail.h>
#endif

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/time.h>

void __guac_rdp_update_keysyms(guac_client* client, const int* keysym_string, int from, int to);
int __guac_rdp_send_keysym(guac_client* client, int keysym, int pressed);

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

    /* Clean up filesystem, if allocated */
    if (guac_client_data->filesystem != NULL)
        guac_rdp_fs_free(guac_client_data->filesystem);

    /* Free SVC list */
    guac_common_list_free(guac_client_data->available_svc);

    /* Free client data */
    guac_common_clipboard_free(guac_client_data->clipboard);
    guac_common_surface_free(guac_client_data->default_surface);
    free(guac_client_data);

    return 0;

}

static int rdp_guac_client_wait_for_messages(guac_client* client, int timeout_usecs) {

    rdp_guac_client_data* guac_client_data = (rdp_guac_client_data*) client->data;
    freerdp* rdp_inst = guac_client_data->rdp_inst;
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

int rdp_guac_client_handle_messages(guac_client* client) {

    rdp_guac_client_data* guac_client_data = (rdp_guac_client_data*) client->data;
    freerdp* rdp_inst = guac_client_data->rdp_inst;
    rdpChannels* channels = rdp_inst->context->channels;
    wMessage* event;

    /* Wait for messages */
    int wait_result = rdp_guac_client_wait_for_messages(client, 250000);
    guac_timestamp frame_start = guac_timestamp_current();
    while (wait_result > 0) {

        guac_timestamp frame_end;
        int frame_remaining;

        pthread_mutex_lock(&(guac_client_data->rdp_lock));

        /* Check the libfreerdp fds */
        if (!freerdp_check_fds(rdp_inst)) {
            guac_client_log(client, GUAC_LOG_DEBUG, "Error handling RDP file descriptors");
            pthread_mutex_unlock(&(guac_client_data->rdp_lock));
            return 1;
        }

        /* Check channel fds */
        if (!freerdp_channels_check_fds(channels, rdp_inst)) {
            guac_client_log(client, GUAC_LOG_DEBUG, "Error handling RDP channel file descriptors");
            pthread_mutex_unlock(&(guac_client_data->rdp_lock));
            return 1;
        }

        /* Check for channel events */
        event = freerdp_channels_pop_event(channels);
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
            guac_client_log(client, GUAC_LOG_INFO, "RDP server closed connection");
            pthread_mutex_unlock(&(guac_client_data->rdp_lock));
            return 1;
        }

        pthread_mutex_unlock(&(guac_client_data->rdp_lock));

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
        return 1;

    /* Success */
    guac_common_surface_flush(guac_client_data->default_surface);
    return 0;

}

int rdp_guac_client_mouse_handler(guac_client* client, int x, int y, int mask) {

    rdp_guac_client_data* guac_client_data = (rdp_guac_client_data*) client->data;
    freerdp* rdp_inst = guac_client_data->rdp_inst;

    pthread_mutex_lock(&(guac_client_data->rdp_lock));

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

    pthread_mutex_unlock(&(guac_client_data->rdp_lock));

    return 0;
}

int __guac_rdp_send_keysym(guac_client* client, int keysym, int pressed) {

    rdp_guac_client_data* guac_client_data = (rdp_guac_client_data*) client->data;
    freerdp* rdp_inst = guac_client_data->rdp_inst;

    /* If keysym can be in lookup table */
    if (GUAC_RDP_KEYSYM_STORABLE(keysym)) {

        int pressed_flags;

        /* Look up scancode mapping */
        const guac_rdp_keysym_desc* keysym_desc =
            &GUAC_RDP_KEYSYM_LOOKUP(guac_client_data->keymap, keysym);

        /* If defined, send event */
        if (keysym_desc->scancode != 0) {

            pthread_mutex_lock(&(guac_client_data->rdp_lock));

            /* If defined, send any prerequesite keys that must be set */
            if (keysym_desc->set_keysyms != NULL)
                __guac_rdp_update_keysyms(client, keysym_desc->set_keysyms, 0, 1);

            /* If defined, release any keys that must be cleared */
            if (keysym_desc->clear_keysyms != NULL)
                __guac_rdp_update_keysyms(client, keysym_desc->clear_keysyms, 1, 0);

            /* Determine proper event flag for pressed state */
            if (pressed)
                pressed_flags = KBD_FLAGS_DOWN;
            else
                pressed_flags = KBD_FLAGS_RELEASE;

            /* Send actual key */
            rdp_inst->input->KeyboardEvent(rdp_inst->input, keysym_desc->flags | pressed_flags,
                    keysym_desc->scancode);

            /* If defined, release any keys that were originally released */
            if (keysym_desc->set_keysyms != NULL)
                __guac_rdp_update_keysyms(client, keysym_desc->set_keysyms, 0, 0);

            /* If defined, send any keys that were originally set */
            if (keysym_desc->clear_keysyms != NULL)
                __guac_rdp_update_keysyms(client, keysym_desc->clear_keysyms, 1, 1);

            pthread_mutex_unlock(&(guac_client_data->rdp_lock));

            return 0;

        }
    }

    /* Fall back to unicode events if undefined inside current keymap */

    /* Only send when key pressed - Unicode events do not have
     * DOWN/RELEASE flags */
    if (pressed) {

        /* Translate keysym into codepoint */
        int codepoint;
        if (keysym <= 0xFF)
            codepoint = keysym;
        else if (keysym >= 0x1000000)
            codepoint = keysym & 0xFFFFFF;
        else {
            guac_client_log(client, GUAC_LOG_INFO,
                    "Unmapped keysym has no equivalent unicode "
                    "value: 0x%x", keysym);
            return 0;
        }

        pthread_mutex_lock(&(guac_client_data->rdp_lock));

        /* Send Unicode event */
        rdp_inst->input->UnicodeKeyboardEvent(
                rdp_inst->input,
                0, codepoint);

        pthread_mutex_unlock(&(guac_client_data->rdp_lock));

    }
    
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
    if (GUAC_RDP_KEYSYM_STORABLE(keysym))
        GUAC_RDP_KEYSYM_LOOKUP(guac_client_data->keysym_state, keysym) = pressed;

    return __guac_rdp_send_keysym(client, keysym, pressed);

}

