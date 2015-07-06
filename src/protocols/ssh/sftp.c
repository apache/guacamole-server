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

#include "guac_json.h"

#include "client.h"
#include "sftp.h"

#include <fcntl.h>
#include <libgen.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libssh2_sftp.h>
#include <guacamole/client.h>
#include <guacamole/object.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/stream.h>

/**
 * Concatenates the given filename with the given path, separating the two
 * with a single forward slash. The full result must be no more than
 * GUAC_SFTP_MAX_PATH bytes long, counting null terminator.
 *
 * @param fullpath
 *     The buffer to store the result within. This buffer must be at least
 *     GUAC_SFTP_MAX_PATH bytes long.
 *
 * @param path
 *     The path to append the filename to.
 *
 * @param filename
 *     The filename to append to the path.
 *
 * @return
 *     true if the filename is valid and was successfully appended to the path,
 *     false otherwise.
 */
static bool guac_ssh_append_filename(char* fullpath, const char* path,
        const char* filename) {

    int i;

    /* Disallow "." as a filename */
    if (strcmp(filename, ".") == 0)
        return false;

    /* Disallow ".." as a filename */
    if (strcmp(filename, "..") == 0)
        return false;

    /* Copy path, append trailing slash */
    for (i=0; i<GUAC_SFTP_MAX_PATH; i++) {

        /*
         * Append trailing slash only if:
         *  1) Trailing slash is not already present
         *  2) Path is non-empty
         */

        char c = path[i];
        if (c == '\0') {
            if (i > 0 && path[i-1] != '/')
                fullpath[i++] = '/';
            break;
        }

        /* Copy character if not end of string */
        fullpath[i] = c;

    }

    /* Append filename */
    for (; i<GUAC_SFTP_MAX_PATH; i++) {

        char c = *(filename++);
        if (c == '\0')
            break;

        /* Filenames may not contain slashes */
        if (c == '\\' || c == '/')
            return false;

        /* Append each character within filename */
        fullpath[i] = c;

    }

    /* Verify path length is within maximum */
    if (i == GUAC_SFTP_MAX_PATH)
        return false;

    /* Terminate path string */
    fullpath[i] = '\0';

    /* Append was successful */
    return true;

}

int guac_sftp_file_handler(guac_client* client, guac_stream* stream,
        char* mimetype, char* filename) {

    ssh_guac_client_data* client_data = (ssh_guac_client_data*) client->data;
    char fullpath[GUAC_SFTP_MAX_PATH];
    LIBSSH2_SFTP_HANDLE* file;

    /* Concatenate filename with path */
    if (!guac_ssh_append_filename(fullpath, 
                client_data->sftp_upload_path, filename)) {

        guac_client_log(client, GUAC_LOG_DEBUG,
                "Filename \"%s\" is invalid or resulting path is too long",
                filename);

        /* Abort transfer - invalid filename */
        guac_protocol_send_ack(client->socket, stream, 
                "SFTP: Illegal filename",
                GUAC_PROTOCOL_STATUS_CLIENT_BAD_REQUEST);

        guac_socket_flush(client->socket);
        return 0;
    }

    /* Open file via SFTP */
    file = libssh2_sftp_open(client_data->sftp_session, fullpath,
            LIBSSH2_FXF_WRITE | LIBSSH2_FXF_CREAT | LIBSSH2_FXF_TRUNC,
            S_IRUSR | S_IWUSR);

    /* Inform of status */
    if (file != NULL) {

        guac_client_log(client, GUAC_LOG_DEBUG,
                "File \"%s\" opened",
                fullpath);

        guac_protocol_send_ack(client->socket, stream, "SFTP: File opened", GUAC_PROTOCOL_STATUS_SUCCESS);
        guac_socket_flush(client->socket);
    }
    else {
        guac_client_log(client, GUAC_LOG_INFO, "Unable to open file \"%s\": %s",
                fullpath, libssh2_sftp_last_error(client_data->sftp_session));
        guac_protocol_send_ack(client->socket, stream, "SFTP: Open failed", GUAC_PROTOCOL_STATUS_RESOURCE_NOT_FOUND);
        guac_socket_flush(client->socket);
    }

    /* Set handlers for file stream */
    stream->blob_handler = guac_sftp_blob_handler;
    stream->end_handler = guac_sftp_end_handler;

    /* Store file within stream */
    stream->data = file;
    return 0;

}

int guac_sftp_blob_handler(guac_client* client, guac_stream* stream,
        void* data, int length) {

    /* Pull file from stream */
    ssh_guac_client_data* client_data = (ssh_guac_client_data*) client->data;
    LIBSSH2_SFTP_HANDLE* file = (LIBSSH2_SFTP_HANDLE*) stream->data;

    /* Attempt write */
    if (libssh2_sftp_write(file, data, length) == length) {
        guac_client_log(client, GUAC_LOG_DEBUG, "%i bytes written", length);
        guac_protocol_send_ack(client->socket, stream, "SFTP: OK", GUAC_PROTOCOL_STATUS_SUCCESS);
        guac_socket_flush(client->socket);
    }

    /* Inform of any errors */
    else {
        guac_client_log(client, GUAC_LOG_INFO, "Unable to write to file: %s",
                libssh2_sftp_last_error(client_data->sftp_session));
        guac_protocol_send_ack(client->socket, stream, "SFTP: Write failed", GUAC_PROTOCOL_STATUS_SERVER_ERROR);
        guac_socket_flush(client->socket);
    }

    return 0;

}

int guac_sftp_end_handler(guac_client* client, guac_stream* stream) {

    /* Pull file from stream */
    LIBSSH2_SFTP_HANDLE* file = (LIBSSH2_SFTP_HANDLE*) stream->data;

    /* Attempt to close file */
    if (libssh2_sftp_close(file) == 0) {
        guac_client_log(client, GUAC_LOG_DEBUG, "File closed");
        guac_protocol_send_ack(client->socket, stream, "SFTP: OK", GUAC_PROTOCOL_STATUS_SUCCESS);
        guac_socket_flush(client->socket);
    }
    else {
        guac_client_log(client, GUAC_LOG_INFO, "Unable to close file");
        guac_protocol_send_ack(client->socket, stream, "SFTP: Close failed", GUAC_PROTOCOL_STATUS_SERVER_ERROR);
        guac_socket_flush(client->socket);
    }

    return 0;

}

int guac_sftp_ack_handler(guac_client* client, guac_stream* stream,
        char* message, guac_protocol_status status) {

    ssh_guac_client_data* client_data = (ssh_guac_client_data*) client->data;
    LIBSSH2_SFTP_HANDLE* file = (LIBSSH2_SFTP_HANDLE*) stream->data;

    /* If successful, read data */
    if (status == GUAC_PROTOCOL_STATUS_SUCCESS) {

        /* Attempt read into buffer */
        char buffer[4096];
        int bytes_read = libssh2_sftp_read(file, buffer, sizeof(buffer)); 

        /* If bytes read, send as blob */
        if (bytes_read > 0) {
            guac_protocol_send_blob(client->socket, stream,
                    buffer, bytes_read);

            guac_client_log(client, GUAC_LOG_DEBUG, "%i bytes sent to client",
                    bytes_read);

        }

        /* If EOF, send end */
        else if (bytes_read == 0) {
            guac_client_log(client, GUAC_LOG_DEBUG, "File sent");
            guac_protocol_send_end(client->socket, stream);
            guac_client_free_stream(client, stream);
        }

        /* Otherwise, fail stream */
        else {
            guac_client_log(client, GUAC_LOG_INFO, "Error reading file: %s",
                    libssh2_sftp_last_error(client_data->sftp_session));
            guac_protocol_send_end(client->socket, stream);
            guac_client_free_stream(client, stream);
        }

        guac_socket_flush(client->socket);

    }

    /* Otherwise, return stream to client */
    else
        guac_client_free_stream(client, stream);

    return 0;
}

guac_stream* guac_sftp_download_file(guac_client* client,
        char* filename) {

    ssh_guac_client_data* client_data = (ssh_guac_client_data*) client->data;
    guac_stream* stream;
    LIBSSH2_SFTP_HANDLE* file;

    /* Attempt to open file for reading */
    file = libssh2_sftp_open(client_data->sftp_session, filename,
            LIBSSH2_FXF_READ, 0);
    if (file == NULL) {
        guac_client_log(client, GUAC_LOG_INFO, "Unable to read file \"%s\": %s",
                filename,
                libssh2_sftp_last_error(client_data->sftp_session));
        return NULL;
    }

    /* Allocate stream */
    stream = guac_client_alloc_stream(client);
    stream->ack_handler = guac_sftp_ack_handler;
    stream->data = file;

    /* Send stream start, strip name */
    filename = basename(filename);
    guac_protocol_send_file(client->socket, stream,
            "application/octet-stream", filename);
    guac_socket_flush(client->socket);

    guac_client_log(client, GUAC_LOG_DEBUG, "Sending file \"%s\"", filename);
    return stream;

}

void guac_sftp_set_upload_path(guac_client* client, char* path) {

    ssh_guac_client_data* client_data = (ssh_guac_client_data*) client->data;
    int length = strnlen(path, GUAC_SFTP_MAX_PATH);

    /* Ignore requests which exceed maximum-allowed path */
    if (length > GUAC_SFTP_MAX_PATH) {
        guac_client_log(client, GUAC_LOG_ERROR,
                "Submitted path exceeds limit of %i bytes",
                GUAC_SFTP_MAX_PATH);
        return;
    }

    /* Copy path */
    memcpy(client_data->sftp_upload_path, path, length);
    guac_client_log(client, GUAC_LOG_DEBUG, "Upload path set to \"%s\"", path);

}

guac_object* guac_sftp_expose_filesystem(guac_client* client) {

    /* Init filesystem */
    guac_object* filesystem = guac_client_alloc_object(client);
    filesystem->get_handler = guac_sftp_get_handler;
    filesystem->put_handler = guac_sftp_put_handler;

    /* Send filesystem to client */
    guac_protocol_send_filesystem(client->socket, filesystem, "/");
    guac_socket_flush(client->socket);

    /* Return allocated filesystem */
    return filesystem;

}

int guac_sftp_ls_ack_handler(guac_client* client, guac_stream* stream,
        char* message, guac_protocol_status status) {

    int bytes_read;
    bool blob_written = false;

    char filename[GUAC_SFTP_MAX_PATH];
    LIBSSH2_SFTP_ATTRIBUTES attributes;

    ssh_guac_client_data* client_data = (ssh_guac_client_data*) client->data;
    LIBSSH2_SFTP* sftp = client_data->sftp_session;
    guac_sftp_ls_state* list_state = (guac_sftp_ls_state*) stream->data;

    /* If unsuccessful, free stream and abort */
    if (status != GUAC_PROTOCOL_STATUS_SUCCESS) {
        libssh2_sftp_closedir(list_state->directory);
        guac_client_free_stream(client, stream);
        free(list_state);
        return 0;
    }

    /* While directory entries remain */
    while ((bytes_read = libssh2_sftp_readdir(list_state->directory,
                filename, sizeof(filename), &attributes)) > 0
            && !blob_written) {

        char absolute_path[GUAC_SFTP_MAX_PATH];

        /* Skip current and parent directory entries */
        if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0)
            continue;

        /* Concatenate into absolute path - skip if invalid */
        if (!guac_ssh_append_filename(absolute_path, 
                    list_state->directory_name, filename)) {

            guac_client_log(client, GUAC_LOG_DEBUG,
                    "Skipping filename \"%s\" - filename is invalid or "
                    "resulting path is too long", filename);

            continue;
        }

        /* Stat explicitly if symbolic link (might point to directory) */
        if (LIBSSH2_SFTP_S_ISLNK(attributes.permissions))
            libssh2_sftp_stat(sftp, absolute_path, &attributes);

        /* Determine mimetype */
        const char* mimetype;
        if (LIBSSH2_SFTP_S_ISDIR(attributes.permissions))
            mimetype = GUAC_CLIENT_STREAM_INDEX_MIMETYPE;
        else
            mimetype = "application/octet-stream";

        /* Write entry */
        blob_written |= guac_common_json_write_property(client, stream,
                &list_state->json_state, absolute_path, mimetype);

    }

    /* Complete JSON and cleanup at end of directory */
    if (bytes_read <= 0) {

        /* Complete JSON object */
        guac_common_json_end_object(client, stream, &list_state->json_state);
        guac_common_json_flush(client, stream, &list_state->json_state);

        /* Clean up resources */
        libssh2_sftp_closedir(list_state->directory);
        free(list_state);

        /* Signal of stream */
        guac_protocol_send_end(client->socket, stream);
        guac_client_free_stream(client, stream);

    }

    guac_socket_flush(client->socket);
    return 0;

}

int guac_sftp_get_handler(guac_client* client, guac_object* object,
        char* name) {

    ssh_guac_client_data* client_data = (ssh_guac_client_data*) client->data;
    LIBSSH2_SFTP* sftp = client_data->sftp_session;
    LIBSSH2_SFTP_ATTRIBUTES attributes;

    /* Attempt to read file information */
    if (libssh2_sftp_stat(sftp, name, &attributes)) {
        guac_client_log(client, GUAC_LOG_INFO, "Unable to read file \"%s\"",
                name);
        return 0;
    }

    /* If directory, send contents of directory */
    if (LIBSSH2_SFTP_S_ISDIR(attributes.permissions)) {

        /* Open as directory */
        LIBSSH2_SFTP_HANDLE* dir = libssh2_sftp_opendir(sftp, name);
        if (dir == NULL) {
            guac_client_log(client, GUAC_LOG_INFO,
                    "Unable to read directory \"%s\": %s",
                    name, libssh2_sftp_last_error(sftp));
            return 0;
        }

        /* Init directory listing state */
        guac_sftp_ls_state* list_state = malloc(sizeof(guac_sftp_ls_state));

        list_state->directory = dir;
        strncpy(list_state->directory_name, name,
                sizeof(list_state->directory_name));

        /* Allocate stream for body */
        guac_stream* stream = guac_client_alloc_stream(client);
        stream->ack_handler = guac_sftp_ls_ack_handler;
        stream->data = list_state;

        /* Init JSON object state */
        guac_common_json_begin_object(client, stream, &list_state->json_state);

        /* Associate new stream with get request */
        guac_protocol_send_body(client->socket, object, stream,
                GUAC_CLIENT_STREAM_INDEX_MIMETYPE, name);

    }

    /* Otherwise, send file contents */
    else {

        /* Open as normal file */
        LIBSSH2_SFTP_HANDLE* file = libssh2_sftp_open(sftp, name,
            LIBSSH2_FXF_READ, 0);
        if (file == NULL) {
            guac_client_log(client, GUAC_LOG_INFO,
                    "Unable to read file \"%s\": %s",
                    name, libssh2_sftp_last_error(sftp));
            return 0;
        }

        /* Allocate stream for body */
        guac_stream* stream = guac_client_alloc_stream(client);
        stream->ack_handler = guac_sftp_ack_handler;
        stream->data = file;

        /* Associate new stream with get request */
        guac_protocol_send_body(client->socket, object, stream,
                "application/octet-stream", name);

    }

    guac_socket_flush(client->socket);
    return 0;
}

int guac_sftp_put_handler(guac_client* client, guac_object* object,
        guac_stream* stream, char* mimetype, char* name) {

    ssh_guac_client_data* client_data = (ssh_guac_client_data*) client->data;
    LIBSSH2_SFTP* sftp = client_data->sftp_session;

    /* Open file via SFTP */
    LIBSSH2_SFTP_HANDLE* file = libssh2_sftp_open(sftp, name,
            LIBSSH2_FXF_WRITE | LIBSSH2_FXF_CREAT | LIBSSH2_FXF_TRUNC,
            S_IRUSR | S_IWUSR);

    /* Acknowledge stream if successful */
    if (file != NULL) {
        guac_client_log(client, GUAC_LOG_DEBUG, "File \"%s\" opened", name);
        guac_protocol_send_ack(client->socket, stream, "SFTP: File opened",
                GUAC_PROTOCOL_STATUS_SUCCESS);
    }

    /* Abort on failure */
    else {
        guac_client_log(client, GUAC_LOG_INFO, "Unable to open file \"%s\": %s",
                name, libssh2_sftp_last_error(sftp));
        guac_protocol_send_ack(client->socket, stream, "SFTP: Open failed",
                GUAC_PROTOCOL_STATUS_RESOURCE_NOT_FOUND);
    }

    /* Set handlers for file stream */
    stream->blob_handler = guac_sftp_blob_handler;
    stream->end_handler = guac_sftp_end_handler;

    /* Store file within stream */
    stream->data = file;

    guac_socket_flush(client->socket);
    return 0;
}

