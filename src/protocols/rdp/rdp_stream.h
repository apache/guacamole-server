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


#ifndef _GUAC_RDP_STREAM_H
#define _GUAC_RDP_STREAM_H

#include "config.h"
#include "guac_json.h"
#include "rdp_svc.h"

#include <guacamole/user.h>
#include <guacamole/protocol.h>
#include <guacamole/stream.h>

#include <stdint.h>

/**
 * The transfer status of a file being downloaded.
 */
typedef struct guac_rdp_download_status {

    /**
     * The file ID of the file being downloaded.
     */
    int file_id;

    /**
     * The current position within the file.
     */
    uint64_t offset;

} guac_rdp_download_status;

/**
 * Structure which represents the current state of an upload.
 */
typedef struct guac_rdp_upload_status {

    /**
     * The overall offset within the file that the next write should
     * occur at.
     */
    int offset;

    /**
     * The ID of the file being written to.
     */
    int file_id;

} guac_rdp_upload_status;

/**
 * The current state of a directory listing operation.
 */
typedef struct guac_rdp_ls_status {

    /**
     * The filesystem associated with the directory being listed.
     */
    guac_rdp_fs* fs;

    /**
     * The file ID of the directory being listed.
     */
    int file_id;

    /**
     * The absolute path of the directory being listed.
     */
    char directory_name[GUAC_RDP_FS_MAX_PATH];

    /**
     * The current state of the JSON directory object being written.
     */
    guac_common_json_state json_state;

} guac_rdp_ls_status;

/**
 * All available stream types.
 */
typedef enum guac_rdp_stream_type {

    /**
     * An in-progress file upload.
     */
    GUAC_RDP_UPLOAD_STREAM,

    /**
     * An in-progress file download.
     */
    GUAC_RDP_DOWNLOAD_STREAM,

    /**
     * An in-progress stream of a directory listing.
     */
    GUAC_RDP_LS_STREAM,

    /**
     * The inbound half of a static virtual channel.
     */
    GUAC_RDP_INBOUND_SVC_STREAM,

    /**
     * An inbound stream of clipboard data.
     */
    GUAC_RDP_INBOUND_CLIPBOARD_STREAM

} guac_rdp_stream_type;

/**
 * Variable-typed stream data.
 */
typedef struct guac_rdp_stream {

    /**
     * The type of this stream.
     */
    guac_rdp_stream_type type;

    /**
     * The file upload status. Only valid for GUAC_RDP_UPLOAD_STREAM.
     */
    guac_rdp_upload_status upload_status;

    /**
     * The file upload status. Only valid for GUAC_RDP_DOWNLOAD_STREAM.
     */
    guac_rdp_download_status download_status;

    /**
     * The directory list status. Only valid for GUAC_RDP_LS_STREAM.
     */
    guac_rdp_ls_status ls_status;

    /**
     * Associated SVC instance. Only valid for GUAC_RDP_INBOUND_SVC_STREAM.
     */
    guac_rdp_svc* svc;

} guac_rdp_stream;

/**
 * Handler for inbound files related to file uploads.
 *
 * @param user The user creating the file stream.
 * @param stream The newly-created file stream.
 * @param mimetype The mimetype of the file being received.
 * @param filename The name of the file being received.
 * @return Zero if successful, non-zero if an error occurs.
 */
int guac_rdp_upload_file_handler(guac_user* user, guac_stream* stream,
        char* mimetype, char* filename);

/**
 * Handler for inbound pipes related to static virtual channels.
 *
 * @param user The user creating the pipe stream.
 * @param stream The newly-created pipe stream.
 * @param mimetype The mimetype of the pipe being received.
 * @param name The name of the pipe being received.
 * @return Zero if successful, non-zero if an error occurs.
 */
int guac_rdp_svc_pipe_handler(guac_user* user, guac_stream* stream,
        char* mimetype, char* name);

/**
 * Handler for inbound clipboard data.
 *
 * @param user The user creating the clipboard stream.
 * @param stream The newly-created clipboard stream.
 * @param mimetype The mimetype of the clipboard data being received.
 * @return Zero if successful, non-zero if an error occurs.
 */
int guac_rdp_clipboard_handler(guac_user* user, guac_stream* stream,
        char* mimetype);

/**
 * Handler for stream data related to file uploads.
 *
 * @param user The user sending data.
 * @param stream The file stream receiving the given data.
 * @param data The received data.
 * @param length The number of bytes received.
 * @return Zero if successful, non-zero if an error occurs.
 */
int guac_rdp_upload_blob_handler(guac_user* user, guac_stream* stream,
        void* data, int length);

/**
 * Handler for stream data related to static virtual channels.
 *
 * @param user The user sending data.
 * @param stream The pipe stream receiving the given data.
 * @param data The received data.
 * @param length The number of bytes received.
 * @return Zero if successful, non-zero if an error occurs.
 */
int guac_rdp_svc_blob_handler(guac_user* user, guac_stream* stream,
        void* data, int length);

/**
 * Handler for stream data related to clipboard.
 *
 * @param user The user sending data.
 * @param stream The clipboard stream receiving the given data.
 * @param data The received data.
 * @param length The number of bytes received.
 * @return Zero if successful, non-zero if an error occurs.
 */
int guac_rdp_clipboard_blob_handler(guac_user* user, guac_stream* stream,
        void* data, int length);

/**
 * Handler for end-of-stream related to file uploads.
 *
 * @param user The user closing the stream.
 * @param stream The file stream being closed.
 * @return Zero if successful, non-zero if an error occurs.
 */
int guac_rdp_upload_end_handler(guac_user* user, guac_stream* stream);

/**
 * Handler for end-of-stream related to clipboard.
 *
 * @param user The user closing the stream.
 * @param stream The clipboard stream being closed.
 * @return Zero if successful, non-zero if an error occurs.
 */
int guac_rdp_clipboard_end_handler(guac_user* user, guac_stream* stream);

/**
 * Handler for acknowledgements of receipt of data related to file downloads.
 *
 * @param user The user acknowledging data receipt.
 * @param stream The file stream associated with the data being acknowledged.
 * @param message An arbitrary human-readable message acknowledging receipt.
 * @param status
 *     The status code representing whether the data was received successfully.
 * 
 * @return Zero if successful, non-zero if an error occurs.
 */
int guac_rdp_download_ack_handler(guac_user* user, guac_stream* stream,
        char* message, guac_protocol_status status);

/**
 * Handler for ack messages received due to receipt of a "body" or "blob"
 * instruction associated with a directory list operation.
 *
 * @param user
 *     The user receiving the ack message.
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
int guac_rdp_ls_ack_handler(guac_user* user, guac_stream* stream,
        char* message, guac_protocol_status status);

/**
 * Handler for get messages. In context of downloads and the filesystem exposed
 * via the Guacamole protocol, get messages request the body of a file within
 * the filesystem.
 *
 * @param user
 *     The user receiving the get message.
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
int guac_rdp_download_get_handler(guac_user* user, guac_object* object,
        char* name);

/**
 * Handler for put messages. In context of uploads and the filesystem exposed
 * via the Guacamole protocol, put messages request write access to a file
 * within the filesystem.
 *
 * @param user
 *     The user receiving the put message.
 *
 * @param object
 *     The Guacamole protocol object associated with the put request itself.
 *
 * @param stream
 *     The Guacamole protocol stream along which the user will be sending
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
int guac_rdp_upload_put_handler(guac_user* user, guac_object* object,
        guac_stream* stream, char* mimetype, char* name);

#endif

