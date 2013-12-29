
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

#include <pthread.h>
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

    /* Free client data */
    cairo_surface_destroy(guac_client_data->opaque_glyph_surface);
    cairo_surface_destroy(guac_client_data->trans_glyph_surface);
    free(guac_client_data->clipboard);
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
        guac_error = GUAC_STATUS_BAD_STATE;
        guac_error_message = "Unable to read RDP file descriptors";
        return -1;
    }

    /* Get channel fds */
    if (!freerdp_channels_get_fds(channels, rdp_inst, read_fds, &read_count, write_fds,
                &write_count)) {
        guac_error = GUAC_STATUS_BAD_STATE;
        guac_error_message = "Unable to read RDP channel file descriptors";
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
        guac_error = GUAC_STATUS_BAD_STATE;
        guac_error_message = "No file descriptors";
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
        guac_error = GUAC_STATUS_SEE_ERRNO;
        guac_error_message = "Error waiting for file descriptor";
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
            guac_error = GUAC_STATUS_BAD_STATE;
            guac_error_message = "Error handling RDP file descriptors";
            pthread_mutex_unlock(&(guac_client_data->rdp_lock));
            return 1;
        }

        /* Check channel fds */
        if (!freerdp_channels_check_fds(channels, rdp_inst)) {
            guac_error = GUAC_STATUS_BAD_STATE;
            guac_error_message = "Error handling RDP channel file descriptors";
            pthread_mutex_unlock(&(guac_client_data->rdp_lock));
            return 1;
        }

        /* Check for channel events */
        event = freerdp_channels_pop_event(channels);
        if (event) {

            /* Handle clipboard events */
#ifdef LEGACY_EVENT
            if (event->event_class == CliprdrChannel_Class)
                guac_rdp_process_cliprdr_event(client, event);
#else
            if (GetMessageClass(event->id) == CliprdrChannel_Class)
                guac_rdp_process_cliprdr_event(client, event);
#endif

            freerdp_event_free(event);

        }

        /* Handle RDP disconnect */
        if (freerdp_shall_disconnect(rdp_inst)) {
            guac_error = GUAC_STATUS_NO_INPUT;
            guac_error_message = "RDP server closed connection";
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
            guac_client_log_info(client,
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

int rdp_guac_client_clipboard_handler(guac_client* client, char* data) {

    rdpChannels* channels = 
        ((rdp_guac_client_data*) client->data)->rdp_inst->context->channels;

    RDP_CB_FORMAT_LIST_EVENT* format_list =
        (RDP_CB_FORMAT_LIST_EVENT*) freerdp_event_new(
            CliprdrChannel_Class,
            CliprdrChannel_FormatList,
            NULL, NULL);

    /* Free existing data */
    free(((rdp_guac_client_data*) client->data)->clipboard);

    /* Store data in client */
    ((rdp_guac_client_data*) client->data)->clipboard = strdup(data);

    /* Notify server that text data is now available */
    format_list->formats = (UINT32*) malloc(sizeof(UINT32));
    format_list->formats[0] = CB_FORMAT_TEXT;
    format_list->num_formats = 1;

    freerdp_channels_send_event(channels, (wMessage*) format_list);

    return 0;

}

/**
 * Writes the given filename to the given upload path, sanitizing the filename
 * and translating the filename to the root directory.
 */
static void __generate_upload_path(const char* filename, char* path) {

    int i;

    /* Add initial backslash */
    *(path++) = '\\';

    for (i=1; i<GUAC_RDP_FS_MAX_PATH; i++) {

        /* Get current, stop at end */
        char c = *(filename++);
        if (c == '\0')
            break;

        /* Replace special characters with underscores */
        if (c == '/' || c == '\\')
            c = '_';

        *(path++) = c;

    }

    /* Terminate path */
    *path = '\0';

}

int rdp_guac_client_file_handler(guac_client* client, guac_stream* stream,
        char* mimetype, char* filename) {

    int file_id;
    guac_rdp_upload_stat* stat;
    char file_path[GUAC_RDP_FS_MAX_PATH];

    /* Get filesystem, return error if no filesystem */
    guac_rdp_fs* fs = ((rdp_guac_client_data*) client->data)->filesystem;
    if (fs == NULL) {
        guac_protocol_send_ack(client->socket, stream, "FAIL (NO FS)",
                GUAC_PROTOCOL_STATUS_INTERNAL_ERROR);
        guac_socket_flush(client->socket);
        return 0;
    }

    /* Translate name */
    __generate_upload_path(filename, file_path);

    /* Open file */
    file_id = guac_rdp_fs_open(fs, file_path, ACCESS_GENERIC_WRITE, 0,
            DISP_FILE_OVERWRITE_IF, 0);
    if (file_id < 0) {
        guac_protocol_send_ack(client->socket, stream, "FAIL (CANNOT OPEN)",
                GUAC_PROTOCOL_STATUS_PERMISSION_DENIED);
        guac_socket_flush(client->socket);
        return 0;
    }

    /* Init upload status */
    stat = malloc(sizeof(guac_rdp_upload_stat));
    stat->offset = 0;
    stat->file_id = file_id;
    stream->data = stat;

    guac_protocol_send_ack(client->socket, stream, "OK (STREAM BEGIN)",
            GUAC_PROTOCOL_STATUS_SUCCESS);
    guac_socket_flush(client->socket);
    return 0;
}

int rdp_guac_client_blob_handler(guac_client* client, guac_stream* stream,
        void* data, int length) {

    int bytes_written;
    guac_rdp_upload_stat* stat;

    /* Get filesystem, return error if no filesystem */
    guac_rdp_fs* fs = ((rdp_guac_client_data*) client->data)->filesystem;
    if (fs == NULL) {
        guac_protocol_send_ack(client->socket, stream, "FAIL (NO FS)",
                GUAC_PROTOCOL_STATUS_INTERNAL_ERROR);
        guac_socket_flush(client->socket);
        return 0;
    }

    /* Get upload status */
    stat = (guac_rdp_upload_stat*) stream->data;

    /* Write entire block */
    while (length > 0) {

        /* Attempt write */
        bytes_written = guac_rdp_fs_write(fs, stat->file_id, stat->offset,
                data, length);

        /* On error, abort */
        if (bytes_written < 0) {
            guac_protocol_send_ack(client->socket, stream, "FAIL (BAD WRITE)",
                    GUAC_PROTOCOL_STATUS_PERMISSION_DENIED);
            guac_socket_flush(client->socket);
            return 0;
        }

        /* Update counters */
        stat->offset += bytes_written;
        data += bytes_written;
        length -= bytes_written;

    }

    guac_protocol_send_ack(client->socket, stream, "OK (DATA WRITTEN)",
            GUAC_PROTOCOL_STATUS_SUCCESS);
    guac_socket_flush(client->socket);
    return 0;
}

int rdp_guac_client_end_handler(guac_client* client, guac_stream* stream) {

    guac_rdp_upload_stat* stat;

    /* Get filesystem, return error if no filesystem */
    guac_rdp_fs* fs = ((rdp_guac_client_data*) client->data)->filesystem;
    if (fs == NULL) {
        guac_protocol_send_ack(client->socket, stream, "FAIL (NO FS)",
                GUAC_PROTOCOL_STATUS_INTERNAL_ERROR);
        guac_socket_flush(client->socket);
        return 0;
    }

    /* Get upload status */
    stat = (guac_rdp_upload_stat*) stream->data;

    /* Close file */
    guac_rdp_fs_close(fs, stat->file_id);
    free(stat);

    guac_protocol_send_ack(client->socket, stream, "OK (STREAM END)",
            GUAC_PROTOCOL_STATUS_SUCCESS);
    guac_socket_flush(client->socket);
    return 0;
}

int rdp_guac_client_ack_handler(guac_client* client, guac_stream* stream,
        char* message, guac_protocol_status status) {

    /* Get status */
    guac_rdp_download_status* download =
        (guac_rdp_download_status*) stream->data;

    /* Ignore acks for non-download stream data */
    if (download == NULL)
        return 0;

    /* Get filesystem, return error if no filesystem */
    guac_rdp_fs* fs = ((rdp_guac_client_data*) client->data)->filesystem;
    if (fs == NULL) {
        guac_protocol_send_ack(client->socket, stream, "FAIL (NO FS)",
                GUAC_PROTOCOL_STATUS_INTERNAL_ERROR);
        guac_socket_flush(client->socket);
        return 0;
    }

    /* If successful, read data */
    if (status == GUAC_PROTOCOL_STATUS_SUCCESS) {

        /* Attempt read into buffer */
        char buffer[4096];
        int bytes_read = guac_rdp_fs_read(fs, download->file_id,
                download->offset, buffer, sizeof(buffer)); 

        /* If bytes read, send as blob */
        if (bytes_read > 0) {
            download->offset += bytes_read;
            guac_protocol_send_blob(client->socket, stream,
                    buffer, bytes_read);
        }

        /* If EOF, send end */
        else if (bytes_read == 0) {
            guac_protocol_send_end(client->socket, stream);
            guac_client_free_stream(client, stream);
            free(download);
        }

        /* Otherwise, fail stream */
        else {
            guac_client_log_error(client, "Error reading file for download");
            guac_protocol_send_end(client->socket, stream);
            guac_client_free_stream(client, stream);
            free(download);
        }

        guac_socket_flush(client->socket);

    }

    /* Otherwise, return stream to client */
    else
        guac_client_free_stream(client, stream);

    return 0;


}

