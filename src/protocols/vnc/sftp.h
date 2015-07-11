/*
 * Copyright (C) 2015 Glyptodon LLC
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

#ifndef GUAC_VNC_SFTP_H
#define GUAC_VNC_SFTP_H

#include "config.h"

#include <guacamole/client.h>
#include <guacamole/stream.h>

/**
 * Handles an incoming stream from a Guacamole "file" instruction, saving the
 * contents of that stream to the file having the given name.
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
 *     The filename of the file to write to.
 *
 * @return
 *     Zero if the incoming stream has been handled successfully, non-zero on
 *     failure.
 */
int guac_vnc_sftp_file_handler(guac_client* client, guac_stream* stream,
        char* mimetype, char* filename);

#endif

