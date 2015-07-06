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

#include "guac_json.h"

#include <libssh2.h>
#include <libssh2_sftp.h>

#include <guacamole/client.h>
#include <guacamole/object.h>
#include <guacamole/protocol.h>
#include <guacamole/stream.h>

/**
 * Maximum number of bytes per path.
 */
#define GUAC_SFTP_MAX_PATH 2048

/**
 * The current state of a directory listing operation.
 */
typedef struct guac_sftp_ls_state {

    /**
     * Reference to the directory currently being listed over SFTP. This
     * directory must already be open from a call to libssh2_sftp_opendir().
     */
    LIBSSH2_SFTP_HANDLE* directory;

    /**
     * The absolute path of the directory being listed.
     */
    char directory_name[GUAC_SFTP_MAX_PATH];

    /**
     * The current state of the JSON directory object being written.
     */
    guac_common_json_state json_state;

} guac_sftp_ls_state;

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

/**
 * Exposes access to SFTP via a filesystem object, returning that object. The
 * object returned must eventually be explicitly freed through a call to
 * guac_client_free_object().
 *
 * @param client
 *     The Guacamole client to expose the filesystem to.
 *
 * @return
 *     The resulting Guacamole filesystem object, initialized and exposed to
 *     the client.
 */
guac_object* guac_sftp_expose_filesystem(guac_client* client);

/**
 * Handler for get messages. In context of SFTP and the filesystem exposed via
 * the Guacamole protocol, get messages request the body of a file within the
 * filesystem.
 *
 * @param client
 *     The client receiving the get message.
 *
 * @param object
 *     The Guacamole protocol object associated with the get request itself.
 *
 * @param name
 *     The name of the input stream (file) being requested.
 *
 * @return
 *     Zero on success, non-zero on error.
 */
int guac_sftp_get_handler(guac_client* client, guac_object* object,
        char* name);

/**
 * Handler for put messages. In context of SFTP and the filesystem exposed via
 * the Guacamole protocol, put messages request write access to a file within
 * the filesystem.
 *
 * @param client
 *     The client receiving the put message.
 *
 * @param object
 *     The Guacamole protocol object associated with the put request itself.
 *
 * @param stream
 *     The Guacamole protocol stream along which the client will be sending
 *     file data.
 *
 * @param mimetype
 *     The mimetype of the data being send along the stream.
 *
 * @param name
 *     The name of the input stream (file) being requested.
 *
 * @return
 *     Zero on success, non-zero on error.
 */
int guac_sftp_put_handler(guac_client* client, guac_object* object,
        guac_stream* stream, char* mimetype, char* name);

/**
 * Handler for ack messages received due to receipt of a "body" or "blob"
 * instruction associated with a SFTP directory list operation.
 *
 * @param client
 *     The client receiving the ack message.
 *
 * @param stream
 *     The Guacamole protocol stream associated with the received ack message.
 *
 * @param message
 *     An arbitrary human-readable message describing the nature of the
 *     success or failure denoted by this ack message.
 *
 * @param status
 *     The status code associated with this ack message, which may indicate
 *     success or an error.
 *
 * @return
 *     Zero on success, non-zero on error.
 */
int guac_sftp_ls_ack_handler(guac_client* client, guac_stream* stream,
        char* message, guac_protocol_status status);

#endif

