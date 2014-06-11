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


#ifndef _SSH_GUAC_SFTP_H
#define _SSH_GUAC_SFTP_H

#include "config.h"

#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <guacamole/stream.h>

/**
 * Maximum number of bytes per path.
 */
#define GUAC_SFTP_MAX_PATH 2048

/**
 * Handler for file messages which begins an SFTP data transfer (upload).
 */
int guac_sftp_file_handler(guac_client* client, guac_stream* stream,
        char* mimetype, char* filename);

/**
 * Handler for blob messages which continues an SFTP data transfer (upload).
 */
int guac_sftp_blob_handler(guac_client* client, guac_stream* stream,
        void* data, int length);

/**
 * Handler for end messages which ends an SFTP data transfer (upload).
 */
int guac_sftp_end_handler(guac_client* client, guac_stream* stream);

/**
 * Handler for ack messages which continues an SFTP download.
 */
int guac_sftp_ack_handler(guac_client* client, guac_stream* stream,
        char* message, guac_protocol_status status);

/**
 * Begins (and automatically continues) an SFTP file download to the user.
 */
guac_stream* guac_sftp_download_file(guac_client* client, char* filename);

/**
 * Set the destination directory for future uploads.
 */
void guac_sftp_set_upload_path(guac_client* client, char* path);

#endif

