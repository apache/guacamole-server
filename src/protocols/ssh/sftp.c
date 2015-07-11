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
#include "guac_sftp.h"
#include "sftp.h"

#include <guacamole/client.h>
#include <guacamole/stream.h>

int guac_sftp_file_handler(guac_client* client, guac_stream* stream,
        char* mimetype, char* filename) {

    ssh_guac_client_data* client_data = (ssh_guac_client_data*) client->data;
    guac_object* filesystem = client_data->sftp_filesystem;

    /* Handle file upload */
    return guac_common_ssh_sftp_handle_file_stream(filesystem, stream,
            mimetype, filename);

}

guac_stream* guac_sftp_download_file(guac_client* client,
        char* filename) {

    ssh_guac_client_data* client_data = (ssh_guac_client_data*) client->data;
    guac_object* filesystem = client_data->sftp_filesystem;

    /* Initiate download of requested file */
    return guac_common_ssh_sftp_download_file(filesystem, filename);

}

void guac_sftp_set_upload_path(guac_client* client, char* path) {

    ssh_guac_client_data* client_data = (ssh_guac_client_data*) client->data;
    guac_object* filesystem = client_data->sftp_filesystem;

    /* Set upload path as specified */
    guac_common_ssh_sftp_set_upload_path(filesystem, path);

}

