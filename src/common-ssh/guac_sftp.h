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

#ifndef GUAC_COMMON_SSH_SFTP_H
#define GUAC_COMMON_SSH_SFTP_H

#include <guacamole/client.h>
#include <guacamole/object.h>
#include <libssh2.h>

/**
 * Creates a new Guacamole filesystem object which provides access to files
 * and directories via SFTP using the given SSH session. When the filesystem
 * will no longer be used, it must be explicitly destroyed with
 * guac_common_ssh_destroy_sftp_filesystem().
 *
 * @param client
 *     The Guacamole client which will be associated with the new filesystem
 *     object.
 *
 * @param name
 *     The name to send as the name of the filesystem.
 *
 * @param session
 *     The session to use to provide SFTP.
 *
 * @return
 *     A new Guacamole filesystem object, already configured to use SFTP for
 *     uploading and downloading files.
 */
guac_object* guac_common_ssh_create_sftp_filesystem(guac_client* client,
         const char* name, LIBSSH2_SESSION* session);

/**
 * Destroys the given filesystem object, disconnecting from SFTP and freeing
 * and associated resources.
 *
 * @param client
 *     The client associated with the filesystem object.
 *
 * @param object
 *     The filesystem object to destroy.
 */
void guac_common_ssh_destroy_sftp_filesystem(guac_client* client,
        guac_object* filesystem);

#endif

