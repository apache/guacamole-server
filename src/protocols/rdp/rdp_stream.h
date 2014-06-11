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
#include "rdp_svc.h"

#include <guacamole/client.h>
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
     * Associated SVC instance. Only valid for GUAC_RDP_INBOUND_SVC_STREAM.
     */
    guac_rdp_svc* svc;

} guac_rdp_stream;

/**
 * Handler for inbound files related to file uploads.
 */
int guac_rdp_upload_file_handler(guac_client* client, guac_stream* stream,
        char* mimetype, char* filename);

/**
 * Handler for inbound pipes related to static virtual channels.
 */
int guac_rdp_svc_pipe_handler(guac_client* client, guac_stream* stream,
        char* mimetype, char* name);

/**
 * Handler for inbound clipboard data.
 */
int guac_rdp_clipboard_handler(guac_client* client, guac_stream* stream,
        char* mimetype);

/**
 * Handler for stream data related to file uploads.
 */
int guac_rdp_upload_blob_handler(guac_client* client, guac_stream* stream,
        void* data, int length);

/**
 * Handler for stream data related to static virtual channels.
 */
int guac_rdp_svc_blob_handler(guac_client* client, guac_stream* stream,
        void* data, int length);

/**
 * Handler for stream data related to clipboard.
 */
int guac_rdp_clipboard_blob_handler(guac_client* client, guac_stream* stream,
        void* data, int length);

/**
 * Handler for end-of-stream related to file uploads.
 */
int guac_rdp_upload_end_handler(guac_client* client, guac_stream* stream);

/**
 * Handler for end-of-stream related to clipboard.
 */
int guac_rdp_clipboard_end_handler(guac_client* client, guac_stream* stream);

/**
 * Handler for acknowledgements of receipt of data related to file downloads.
 */
int guac_rdp_download_ack_handler(guac_client* client, guac_stream* stream,
        char* message, guac_protocol_status status);

#endif

