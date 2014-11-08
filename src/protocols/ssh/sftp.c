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
#include "sftp.h"

#include <fcntl.h>
#include <libgen.h>
#include <stdbool.h>
#include <string.h>

#include <libssh2_sftp.h>
#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/stream.h>

static bool __ssh_guac_valid_filename(char* filename) {

    /* Disallow "." as a filename */
    if (strcmp(filename, ".") == 0)
        return false;

    /* Disallow ".." as a filename */
    if (strcmp(filename, "..") == 0)
        return false;

    /* Search for path separator characters */
    for (;;) {

        char c = *(filename++);
        if (c == '\0')
            break;

        /* Replace slashes with underscores */
        if (c == '\\' || c == '/')
            return false;

    }

    /* If filename does not contain a path, it's ok */
    return true;

}

int guac_sftp_file_handler(guac_client* client, guac_stream* stream,
        char* mimetype, char* filename) {

    ssh_guac_client_data* client_data = (ssh_guac_client_data*) client->data;
    char fullpath[GUAC_SFTP_MAX_PATH];
    LIBSSH2_SFTP_HANDLE* file;
    int i;

    /* Ensure filename is a valid filename and not a path */
    if (!__ssh_guac_valid_filename(filename)) {
        guac_protocol_send_ack(client->socket, stream, "SFTP: Illegal filename",
                GUAC_PROTOCOL_STATUS_CLIENT_BAD_REQUEST);
        guac_socket_flush(client->socket);
        return 0;
    }

    /* Copy upload path, append trailing slash */
    for (i=0; i<GUAC_SFTP_MAX_PATH; i++) {
        char c = client_data->sftp_upload_path[i];
        if (c == '\0') {
            fullpath[i++] = '/';
            break;
        }

        fullpath[i] = c;
    }

    /* Append filename */
    for (; i<GUAC_SFTP_MAX_PATH; i++) {
        char c = *(filename++);
        if (c == '\0')
            break;

        fullpath[i] = c;
    }

    /* If path + filename exceeds max length, abort */
    if (i == GUAC_SFTP_MAX_PATH) {
        guac_protocol_send_ack(client->socket, stream, "SFTP: Name too long", GUAC_PROTOCOL_STATUS_CLIENT_OVERRUN);
        guac_socket_flush(client->socket);
        return 0;
    }

    /* Terminate path string */
    fullpath[i] = '\0';

    /* Open file via SFTP */
    file = libssh2_sftp_open(client_data->sftp_session, fullpath,
            LIBSSH2_FXF_WRITE | LIBSSH2_FXF_CREAT | LIBSSH2_FXF_TRUNC,
            S_IRUSR | S_IWUSR);

    /* Inform of status */
    if (file != NULL) {
        guac_protocol_send_ack(client->socket, stream, "SFTP: File opened", GUAC_PROTOCOL_STATUS_SUCCESS);
        guac_socket_flush(client->socket);
    }
    else {
        guac_client_log(client, GUAC_LOG_ERROR, "Unable to open file \"%s\": %s",
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
        guac_protocol_send_ack(client->socket, stream, "SFTP: OK", GUAC_PROTOCOL_STATUS_SUCCESS);
        guac_socket_flush(client->socket);
    }

    /* Inform of any errors */
    else {
        guac_client_log(client, GUAC_LOG_ERROR, "Unable to write to file: %s",
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
        guac_protocol_send_ack(client->socket, stream, "SFTP: OK", GUAC_PROTOCOL_STATUS_SUCCESS);
        guac_socket_flush(client->socket);
    }
    else {
        guac_client_log(client, GUAC_LOG_ERROR, "Unable to close file");
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
        if (bytes_read > 0)
            guac_protocol_send_blob(client->socket, stream,
                    buffer, bytes_read);

        /* If EOF, send end */
        else if (bytes_read == 0) {
            guac_protocol_send_end(client->socket, stream);
            guac_client_free_stream(client, stream);
        }

        /* Otherwise, fail stream */
        else {
            guac_client_log(client, GUAC_LOG_ERROR, "Error reading file: %s",
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
        guac_client_log(client, GUAC_LOG_ERROR, "Unable to read file \"%s\": %s",
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

}

