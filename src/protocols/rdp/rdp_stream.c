/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */


#include "config.h"
#include "client.h"
#include "common/clipboard.h"
#include "rdp.h"
#include "rdp_fs.h"
#include "rdp_svc.h"
#include "rdp_stream.h"

#include <freerdp/freerdp.h>
#include <freerdp/channels/channels.h>
#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/stream.h>
#include <guacamole/string.h>

#ifdef HAVE_FREERDP_CLIENT_CLIPRDR_H
#include <freerdp/client/cliprdr.h>
#else
#include "compat/client-cliprdr.h"
#endif

#ifdef ENABLE_WINPR
#include <winpr/stream.h>
#include <winpr/wtypes.h>
#else
#include "compat/winpr-stream.h"
#include "compat/winpr-wtypes.h"
#endif

#include <stdlib.h>

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

int guac_rdp_upload_file_handler(guac_user* user, guac_stream* stream,
        char* mimetype, char* filename) {

    guac_client* client = user->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    int file_id;
    guac_rdp_stream* rdp_stream;
    char file_path[GUAC_RDP_FS_MAX_PATH];

    /* Get filesystem, return error if no filesystem */
    guac_rdp_fs* fs = rdp_client->filesystem;
    if (fs == NULL) {
        guac_protocol_send_ack(user->socket, stream, "FAIL (NO FS)",
                GUAC_PROTOCOL_STATUS_SERVER_ERROR);
        guac_socket_flush(user->socket);
        return 0;
    }

    /* Translate name */
    __generate_upload_path(filename, file_path);

    /* Open file */
    file_id = guac_rdp_fs_open(fs, file_path, ACCESS_GENERIC_WRITE, 0,
            DISP_FILE_OVERWRITE_IF, 0);
    if (file_id < 0) {
        guac_protocol_send_ack(user->socket, stream, "FAIL (CANNOT OPEN)",
                GUAC_PROTOCOL_STATUS_CLIENT_FORBIDDEN);
        guac_socket_flush(user->socket);
        return 0;
    }

    /* Init upload status */
    rdp_stream = malloc(sizeof(guac_rdp_stream));
    rdp_stream->type = GUAC_RDP_UPLOAD_STREAM;
    rdp_stream->upload_status.offset = 0;
    rdp_stream->upload_status.file_id = file_id;
    stream->data = rdp_stream;
    stream->blob_handler = guac_rdp_upload_blob_handler;
    stream->end_handler = guac_rdp_upload_end_handler;

    guac_protocol_send_ack(user->socket, stream, "OK (STREAM BEGIN)",
            GUAC_PROTOCOL_STATUS_SUCCESS);
    guac_socket_flush(user->socket);
    return 0;

}

int guac_rdp_svc_pipe_handler(guac_user* user, guac_stream* stream,
        char* mimetype, char* name) {

    guac_rdp_stream* rdp_stream;
    guac_rdp_svc* svc = guac_rdp_get_svc(user->client, name);

    /* Fail if no such SVC */
    if (svc == NULL) {
        guac_user_log(user, GUAC_LOG_ERROR,
                "Requested non-existent pipe: \"%s\".",
                name);
        guac_protocol_send_ack(user->socket, stream, "FAIL (NO SUCH PIPE)",
                GUAC_PROTOCOL_STATUS_CLIENT_BAD_REQUEST);
        guac_socket_flush(user->socket);
        return 0;
    }
    else
        guac_user_log(user, GUAC_LOG_ERROR,
                "Inbound half of channel \"%s\" connected.",
                name);

    /* Init stream data */
    stream->data = rdp_stream = malloc(sizeof(guac_rdp_stream));
    stream->blob_handler = guac_rdp_svc_blob_handler;
    rdp_stream->type = GUAC_RDP_INBOUND_SVC_STREAM;
    rdp_stream->svc = svc;

    return 0;

}

int guac_rdp_clipboard_handler(guac_user* user, guac_stream* stream,
        char* mimetype) {

    guac_client* client = user->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    guac_rdp_stream* rdp_stream;

    /* Init stream data */
    stream->data = rdp_stream = malloc(sizeof(guac_rdp_stream));
    stream->blob_handler = guac_rdp_clipboard_blob_handler;
    stream->end_handler = guac_rdp_clipboard_end_handler;
    rdp_stream->type = GUAC_RDP_INBOUND_CLIPBOARD_STREAM;

    guac_common_clipboard_reset(rdp_client->clipboard, mimetype);
    return 0;

}

int guac_rdp_upload_blob_handler(guac_user* user, guac_stream* stream,
        void* data, int length) {

    int bytes_written;
    guac_rdp_stream* rdp_stream = (guac_rdp_stream*) stream->data;

    /* Get filesystem, return error if no filesystem 0*/
    guac_client* client = user->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    guac_rdp_fs* fs = rdp_client->filesystem;
    if (fs == NULL) {
        guac_protocol_send_ack(user->socket, stream, "FAIL (NO FS)",
                GUAC_PROTOCOL_STATUS_SERVER_ERROR);
        guac_socket_flush(user->socket);
        return 0;
    }

    /* Write entire block */
    while (length > 0) {

        /* Attempt write */
        bytes_written = guac_rdp_fs_write(fs,
                rdp_stream->upload_status.file_id,
                rdp_stream->upload_status.offset,
                data, length);

        /* On error, abort */
        if (bytes_written < 0) {
            guac_protocol_send_ack(user->socket, stream,
                    "FAIL (BAD WRITE)",
                    GUAC_PROTOCOL_STATUS_CLIENT_FORBIDDEN);
            guac_socket_flush(user->socket);
            return 0;
        }

        /* Update counters */
        rdp_stream->upload_status.offset += bytes_written;
        data += bytes_written;
        length -= bytes_written;

    }

    guac_protocol_send_ack(user->socket, stream, "OK (DATA RECEIVED)",
            GUAC_PROTOCOL_STATUS_SUCCESS);
    guac_socket_flush(user->socket);
    return 0;

}

int guac_rdp_svc_blob_handler(guac_user* user, guac_stream* stream,
        void* data, int length) {

    guac_rdp_stream* rdp_stream = (guac_rdp_stream*) stream->data;

    /* Write blob data to SVC directly */
    guac_rdp_svc_write(rdp_stream->svc, data, length);

    guac_protocol_send_ack(user->socket, stream, "OK (DATA RECEIVED)",
            GUAC_PROTOCOL_STATUS_SUCCESS);
    guac_socket_flush(user->socket);
    return 0;

}

int guac_rdp_clipboard_blob_handler(guac_user* user, guac_stream* stream,
        void* data, int length) {

    guac_client* client = user->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    guac_common_clipboard_append(rdp_client->clipboard, (char*) data, length);

    return 0;
}

int guac_rdp_upload_end_handler(guac_user* user, guac_stream* stream) {

    guac_client* client = user->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    guac_rdp_stream* rdp_stream = (guac_rdp_stream*) stream->data;

    /* Get filesystem, return error if no filesystem */
    guac_rdp_fs* fs = rdp_client->filesystem;
    if (fs == NULL) {
        guac_protocol_send_ack(user->socket, stream, "FAIL (NO FS)",
                GUAC_PROTOCOL_STATUS_SERVER_ERROR);
        guac_socket_flush(user->socket);
        return 0;
    }

    /* Close file */
    guac_rdp_fs_close(fs, rdp_stream->upload_status.file_id);

    /* Acknowledge stream end */
    guac_protocol_send_ack(user->socket, stream, "OK (STREAM END)",
            GUAC_PROTOCOL_STATUS_SUCCESS);
    guac_socket_flush(user->socket);

    free(rdp_stream);
    return 0;

}

int guac_rdp_clipboard_end_handler(guac_user* user, guac_stream* stream) {

    guac_client* client = user->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    /* Terminate clipboard data with NULL */
    guac_common_clipboard_append(rdp_client->clipboard, "", 1);

    /* Notify RDP server of new data, if connected */
    freerdp* rdp_inst = rdp_client->rdp_inst;
    if (rdp_inst != NULL) {

        rdpChannels* channels = rdp_inst->context->channels;

        RDP_CB_FORMAT_LIST_EVENT* format_list =
            (RDP_CB_FORMAT_LIST_EVENT*) freerdp_event_new(
                CliprdrChannel_Class,
                CliprdrChannel_FormatList,
                NULL, NULL);

        /* Notify server that text data is now available */
        format_list->formats = (UINT32*) malloc(sizeof(UINT32) * 2);
        format_list->formats[0] = CB_FORMAT_TEXT;
        format_list->formats[1] = CB_FORMAT_UNICODETEXT;
        format_list->num_formats = 2;

        freerdp_channels_send_event(channels, (wMessage*) format_list);

    }

    return 0;
}

int guac_rdp_download_ack_handler(guac_user* user, guac_stream* stream,
        char* message, guac_protocol_status status) {

    guac_client* client = user->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    guac_rdp_stream* rdp_stream = (guac_rdp_stream*) stream->data;

    /* Get filesystem, return error if no filesystem */
    guac_rdp_fs* fs = rdp_client->filesystem;
    if (fs == NULL) {
        guac_protocol_send_ack(user->socket, stream, "FAIL (NO FS)",
                GUAC_PROTOCOL_STATUS_SERVER_ERROR);
        guac_socket_flush(user->socket);
        return 0;
    }

    /* If successful, read data */
    if (status == GUAC_PROTOCOL_STATUS_SUCCESS) {

        /* Attempt read into buffer */
        char buffer[4096];
        int bytes_read = guac_rdp_fs_read(fs,
                rdp_stream->download_status.file_id,
                rdp_stream->download_status.offset, buffer, sizeof(buffer));

        /* If bytes read, send as blob */
        if (bytes_read > 0) {
            rdp_stream->download_status.offset += bytes_read;
            guac_protocol_send_blob(user->socket, stream,
                    buffer, bytes_read);
        }

        /* If EOF, send end */
        else if (bytes_read == 0) {
            guac_protocol_send_end(user->socket, stream);
            guac_user_free_stream(user, stream);
            free(rdp_stream);
        }

        /* Otherwise, fail stream */
        else {
            guac_user_log(user, GUAC_LOG_ERROR,
                    "Error reading file for download");
            guac_protocol_send_end(user->socket, stream);
            guac_user_free_stream(user, stream);
            free(rdp_stream);
        }

        guac_socket_flush(user->socket);

    }

    /* Otherwise, return stream to user */
    else
        guac_user_free_stream(user, stream);

    return 0;

}

int guac_rdp_ls_ack_handler(guac_user* user, guac_stream* stream,
        char* message, guac_protocol_status status) {

    int blob_written = 0;
    const char* filename;

    guac_rdp_stream* rdp_stream = (guac_rdp_stream*) stream->data;

    /* If unsuccessful, free stream and abort */
    if (status != GUAC_PROTOCOL_STATUS_SUCCESS) {
        guac_rdp_fs_close(rdp_stream->ls_status.fs,
                rdp_stream->ls_status.file_id);
        guac_user_free_stream(user, stream);
        free(rdp_stream);
        return 0;
    }

    /* While directory entries remain */
    while ((filename = guac_rdp_fs_read_dir(rdp_stream->ls_status.fs,
                    rdp_stream->ls_status.file_id)) != NULL
            && !blob_written) {

        char absolute_path[GUAC_RDP_FS_MAX_PATH];

        /* Skip current and parent directory entries */
        if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0)
            continue;

        /* Concatenate into absolute path - skip if invalid */
        if (!guac_rdp_fs_append_filename(absolute_path,
                    rdp_stream->ls_status.directory_name, filename)) {

            guac_user_log(user, GUAC_LOG_DEBUG,
                    "Skipping filename \"%s\" - filename is invalid or "
                    "resulting path is too long", filename);

            continue;
        }

        /* Attempt to open file to determine type */
        int file_id = guac_rdp_fs_open(rdp_stream->ls_status.fs, absolute_path,
                ACCESS_GENERIC_READ, 0, DISP_FILE_OPEN, 0);
        if (file_id < 0)
            continue;

        /* Get opened file */
        guac_rdp_fs_file* file = guac_rdp_fs_get_file(rdp_stream->ls_status.fs,
                file_id);
        if (file == NULL) {
            guac_client_log(rdp_stream->ls_status.fs->client, GUAC_LOG_DEBUG,
                    "%s: Successful open produced bad file_id: %i",
                    __func__, file_id);
            return 0;
        }

        /* Determine mimetype */
        const char* mimetype;
        if (file->attributes & FILE_ATTRIBUTE_DIRECTORY)
            mimetype = GUAC_USER_STREAM_INDEX_MIMETYPE;
        else
            mimetype = "application/octet-stream";

        /* Write entry */
        blob_written |= guac_common_json_write_property(user, stream,
                &rdp_stream->ls_status.json_state, absolute_path, mimetype);

        guac_rdp_fs_close(rdp_stream->ls_status.fs, file_id);

    }

    /* Complete JSON and cleanup at end of directory */
    if (filename == NULL) {

        /* Complete JSON object */
        guac_common_json_end_object(user, stream,
                &rdp_stream->ls_status.json_state);
        guac_common_json_flush(user, stream,
                &rdp_stream->ls_status.json_state);

        /* Clean up resources */
        guac_rdp_fs_close(rdp_stream->ls_status.fs,
                rdp_stream->ls_status.file_id);
        free(rdp_stream);

        /* Signal of stream */
        guac_protocol_send_end(user->socket, stream);
        guac_user_free_stream(user, stream);

    }

    guac_socket_flush(user->socket);
    return 0;

}

int guac_rdp_download_get_handler(guac_user* user, guac_object* object,
        char* name) {

    guac_client* client = user->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    /* Get filesystem, ignore request if no filesystem */
    guac_rdp_fs* fs = rdp_client->filesystem;
    if (fs == NULL)
        return 0;

    /* Attempt to open file for reading */
    int file_id = guac_rdp_fs_open(fs, name, ACCESS_GENERIC_READ, 0,
            DISP_FILE_OPEN, 0);
    if (file_id < 0) {
        guac_user_log(user, GUAC_LOG_INFO, "Unable to read file \"%s\"",
                name);
        return 0;
    }

    /* Get opened file */
    guac_rdp_fs_file* file = guac_rdp_fs_get_file(fs, file_id);
    if (file == NULL) {
        guac_client_log(fs->client, GUAC_LOG_DEBUG,
                "%s: Successful open produced bad file_id: %i",
                __func__, file_id);
        return 0;
    }

    /* If directory, send contents of directory */
    if (file->attributes & FILE_ATTRIBUTE_DIRECTORY) {

        /* Create stream data */
        guac_rdp_stream* rdp_stream = malloc(sizeof(guac_rdp_stream));
        rdp_stream->type = GUAC_RDP_LS_STREAM;
        rdp_stream->ls_status.fs = fs;
        rdp_stream->ls_status.file_id = file_id;
        guac_strlcpy(rdp_stream->ls_status.directory_name, name,
                sizeof(rdp_stream->ls_status.directory_name));

        /* Allocate stream for body */
        guac_stream* stream = guac_user_alloc_stream(user);
        stream->ack_handler = guac_rdp_ls_ack_handler;
        stream->data = rdp_stream;

        /* Init JSON object state */
        guac_common_json_begin_object(user, stream,
                &rdp_stream->ls_status.json_state);

        /* Associate new stream with get request */
        guac_protocol_send_body(user->socket, object, stream,
                GUAC_USER_STREAM_INDEX_MIMETYPE, name);

    }

    /* Otherwise, send file contents */
    else {

        /* Create stream data */
        guac_rdp_stream* rdp_stream = malloc(sizeof(guac_rdp_stream));
        rdp_stream->type = GUAC_RDP_DOWNLOAD_STREAM;
        rdp_stream->download_status.file_id = file_id;
        rdp_stream->download_status.offset = 0;

        /* Allocate stream for body */
        guac_stream* stream = guac_user_alloc_stream(user);
        stream->data = rdp_stream;
        stream->ack_handler = guac_rdp_download_ack_handler;

        /* Associate new stream with get request */
        guac_protocol_send_body(user->socket, object, stream,
                "application/octet-stream", name);

    }

    guac_socket_flush(user->socket);
    return 0;
}

int guac_rdp_upload_put_handler(guac_user* user, guac_object* object,
        guac_stream* stream, char* mimetype, char* name) {

    guac_client* client = user->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    /* Get filesystem, return error if no filesystem */
    guac_rdp_fs* fs = rdp_client->filesystem;
    if (fs == NULL) {
        guac_protocol_send_ack(user->socket, stream, "FAIL (NO FS)",
                GUAC_PROTOCOL_STATUS_SERVER_ERROR);
        guac_socket_flush(user->socket);
        return 0;
    }

    /* Open file */
    int file_id = guac_rdp_fs_open(fs, name, ACCESS_GENERIC_WRITE, 0,
            DISP_FILE_OVERWRITE_IF, 0);

    /* Abort on failure */
    if (file_id < 0) {
        guac_protocol_send_ack(user->socket, stream, "FAIL (CANNOT OPEN)",
                GUAC_PROTOCOL_STATUS_CLIENT_FORBIDDEN);
        guac_socket_flush(user->socket);
        return 0;
    }

    /* Init upload stream data */
    guac_rdp_stream* rdp_stream = malloc(sizeof(guac_rdp_stream));
    rdp_stream->type = GUAC_RDP_UPLOAD_STREAM;
    rdp_stream->upload_status.offset = 0;
    rdp_stream->upload_status.file_id = file_id;

    /* Allocate stream, init for file upload */
    stream->data = rdp_stream;
    stream->blob_handler = guac_rdp_upload_blob_handler;
    stream->end_handler = guac_rdp_upload_end_handler;

    /* Acknowledge stream creation */
    guac_protocol_send_ack(user->socket, stream, "OK (STREAM BEGIN)",
            GUAC_PROTOCOL_STATUS_SUCCESS);
    guac_socket_flush(user->socket);
    return 0;
}

