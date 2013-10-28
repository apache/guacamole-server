
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
 * The Original Code is libguac-client-ssh.
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


#include <unistd.h>
#include <libgen.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>

#include <libssh/libssh.h>
#include <libssh/sftp.h>

#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <guacamole/stream.h>

#include "client.h"

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
    sftp_file file;

    /* Ensure filename is a valid filename and not a path */
    if (!__ssh_guac_valid_filename(filename)) {
        guac_protocol_send_ack(client->socket, stream,
                "SFTP: Illegal filename",
                GUAC_PROTOCOL_STATUS_INVALID_PARAMETER);
        guac_socket_flush(client->socket);
        return 0;
    }

    /* Open file via SFTP */
    file = sftp_open(client_data->sftp_session, filename,
            O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

    /* Inform of status */
    if (file != NULL) {
        guac_protocol_send_ack(client->socket, stream, "SFTP: File opened",
                GUAC_PROTOCOL_STATUS_SUCCESS);
        guac_socket_flush(client->socket);
    }
    else {
        guac_client_log_error(client, "Unable to open file: %s",
                ssh_get_error(client_data->sftp_ssh_session));
        guac_protocol_send_ack(client->socket, stream, "SFTP: Open failed",
                GUAC_PROTOCOL_STATUS_INTERNAL_ERROR);
        guac_socket_flush(client->socket);
    }

    /* Store file within stream */
    stream->data = file;
    return 0;

}

int guac_sftp_blob_handler(guac_client* client, guac_stream* stream,
        void* data, int length) {

    /* Pull file from stream */
    ssh_guac_client_data* client_data = (ssh_guac_client_data*) client->data;
    sftp_file file = (sftp_file) stream->data;

    /* Attempt write */
    if (sftp_write(file, data, length) == length) {
        guac_protocol_send_ack(client->socket, stream, "SFTP: OK",
                GUAC_PROTOCOL_STATUS_SUCCESS);
        guac_socket_flush(client->socket);
    }

    /* Inform of any errors */
    else {
        guac_client_log_error(client, "Unable to write to file: %s",
                ssh_get_error(client_data->sftp_ssh_session));
        guac_protocol_send_ack(client->socket, stream, "SFTP: Write failed",
                GUAC_PROTOCOL_STATUS_INTERNAL_ERROR);
        guac_socket_flush(client->socket);
    }

    return 0;

}

int guac_sftp_end_handler(guac_client* client, guac_stream* stream) {

    /* Pull file from stream */
    sftp_file file = (sftp_file) stream->data;

    /* Attempt to close file */
    if (sftp_close(file) == SSH_OK) {
        guac_protocol_send_ack(client->socket, stream, "SFTP: OK",
                GUAC_PROTOCOL_STATUS_SUCCESS);
        guac_socket_flush(client->socket);
    }
    else {
        guac_client_log_error(client, "Unable to close file");
        guac_protocol_send_ack(client->socket, stream, "SFTP: Close failed",
                GUAC_PROTOCOL_STATUS_INTERNAL_ERROR);
        guac_socket_flush(client->socket);
    }

    return 0;

}

int guac_sftp_ack_handler(guac_client* client, guac_stream* stream,
        char* message, guac_protocol_status status) {

    ssh_guac_client_data* client_data = (ssh_guac_client_data*) client->data;
    sftp_file file = (sftp_file) stream->data;

    /* If successful, read data */
    if (status == GUAC_PROTOCOL_STATUS_SUCCESS) {

        /* Attempt read into buffer */
        char buffer[4096];
        int bytes_read = sftp_read(file, buffer, sizeof(buffer)); 

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
            guac_client_log_error(client, "Error reading file: %s",
                    ssh_get_error(client_data->sftp_ssh_session));
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
    sftp_file file;

    /* Attempt to open file for reading */
    file = sftp_open(client_data->sftp_session, filename, O_RDONLY, 0);
    if (file == NULL) {
        guac_client_log_error(client, "Unable to read file \"%s\": %s",
                filename, ssh_get_error(client_data->sftp_ssh_session));
        return NULL;
    }

    /* Allocate stream */
    stream = guac_client_alloc_stream(client);
    stream->data = file;

    /* Send stream start, strip name */
    filename = basename(filename);
    guac_protocol_send_file(client->socket, stream,
            "application/octet-stream", filename);
    guac_socket_flush(client->socket);

    return stream;

}

