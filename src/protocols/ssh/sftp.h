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
#include <guacamole/stream.h>

/**
 * Handles an incoming stream from a Guacamole "file" instruction, saving the
 * contents of that stream to the file having the given name within the
 * upload directory set by guac_sftp_set_upload_path().
 *
 * @param client
 *     The client receiving the uploaded file.
 *
 * @param stream
 *     The stream through which the uploaded file data will be received.
 *
 * @param mimetype
 *     The mimetype of the data being received.
 *
 * @param filename
 *     The filename of the file to write to. This filename will always be taken
 *     relative to the upload path set by
 *     guac_common_ssh_sftp_set_upload_path().
 *
 * @return
 *     Zero if the incoming stream has been handled successfully, non-zero on
 *     failure.
 */
int guac_sftp_file_handler(guac_client* client, guac_stream* stream,
        char* mimetype, char* filename);

/**
 * Initiates an SFTP file download to the user via the Guacamole "file"
 * instruction. The download will be automatically monitored and continued
 * after this function terminates in response to "ack" instructions received by
 * the client.
 *
 * @param client
 *     The client receiving the file.
 *
 * @param filename
 *     The filename of the file to download, relative to the given filesystem.
 *
 * @return
 *     The file stream created for the file download, already configured to
 *     properly handle "ack" responses, etc. from the client.
 */
guac_stream* guac_sftp_download_file(guac_client* client, char* filename);

/**
 * Sets the destination directory for future uploads submitted via Guacamole
 * "file" instruction. This function has no bearing on the destination
 * directories of files uploaded with "put" instructions.
 *
 * @param client
 *     The client setting the upload path.
 *
 * @param path
 *     The path to use for future uploads submitted via "file" instruction.
 */
void guac_sftp_set_upload_path(guac_client* client, char* path);

#endif

