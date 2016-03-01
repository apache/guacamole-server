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

#include "guac_sftp.h"
#include "sftp.h"
#include "ssh.h"

#include <guacamole/client.h>
#include <guacamole/stream.h>
#include <guacamole/user.h>

int guac_sftp_file_handler(guac_user* user, guac_stream* stream,
        char* mimetype, char* filename) {

    guac_client* client = user->client;
    guac_ssh_client* ssh_client = (guac_ssh_client*) client->data;
    guac_common_ssh_sftp_filesystem* filesystem = ssh_client->sftp_filesystem;

    /* Handle file upload */
    return guac_common_ssh_sftp_handle_file_stream(filesystem, user, stream,
            mimetype, filename);

}

/**
 * Callback invoked on the current connection owner (if any) when a file
 * download is being initiated through the terminal.
 *
 * @param owner
 *     The guac_user that is the owner of the connection, or NULL if the
 *     connection owner has left.
 *
 * @param data
 *     The filename of the file that should be downloaded.
 *
 * @return
 *     The stream allocated for the file download, or NULL if no stream could
 *     be allocated.
 */
static void* guac_sftp_download_to_owner(guac_user* owner, void* data) {

    /* Do not bother attempting the download if the owner has left */
    if (owner == NULL)
        return NULL;

    guac_client* client = owner->client;
    guac_ssh_client* ssh_client = (guac_ssh_client*) client->data;
    guac_common_ssh_sftp_filesystem* filesystem = ssh_client->sftp_filesystem;

    /* Ignore download if filesystem has been unloaded */
    if (filesystem == NULL)
        return NULL;

    char* filename = (char*) data;

    /* Initiate download of requested file */
    return guac_common_ssh_sftp_download_file(filesystem, owner, filename);

}

guac_stream* guac_sftp_download_file(guac_client* client, char* filename) {

    /* Initiate download to the owner of the connection */
    return (guac_stream*) guac_client_for_owner(client,
            guac_sftp_download_to_owner, filename);

}

void guac_sftp_set_upload_path(guac_client* client, char* path) {

    guac_ssh_client* ssh_client = (guac_ssh_client*) client->data;
    guac_common_ssh_sftp_filesystem* filesystem = ssh_client->sftp_filesystem;

    /* Set upload path as specified */
    guac_common_ssh_sftp_set_upload_path(filesystem, path);

}

