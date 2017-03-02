/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */


#ifndef _GUAC_RDP_STREAM_H
#define _GUAC_RDP_STREAM_H

#include "config.h"
#include "common/json.h"
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
 */
guac_user_file_handler guac_rdp_upload_file_handler;

/**
 * Handler for inbound pipes related to static virtual channels.
 */
guac_user_pipe_handler guac_rdp_svc_pipe_handler;

/**
 * Handler for inbound clipboard data.
 */
guac_user_clipboard_handler guac_rdp_clipboard_handler;

/**
 * Handler for stream data related to file uploads.
 */
guac_user_blob_handler guac_rdp_upload_blob_handler;

/**
 * Handler for stream data related to static virtual channels.
 */
guac_user_blob_handler guac_rdp_svc_blob_handler;

/**
 * Handler for stream data related to clipboard.
 */
guac_user_blob_handler guac_rdp_clipboard_blob_handler;

/**
 * Handler for end-of-stream related to file uploads.
 */
guac_user_end_handler guac_rdp_upload_end_handler;

/**
 * Handler for end-of-stream related to clipboard.
 */
guac_user_end_handler guac_rdp_clipboard_end_handler;

/**
 * Handler for acknowledgements of receipt of data related to file downloads.
 */
guac_user_ack_handler guac_rdp_download_ack_handler;

/**
 * Handler for ack messages received due to receipt of a "body" or "blob"
 * instruction associated with a directory list operation.
 */
guac_user_ack_handler guac_rdp_ls_ack_handler;

/**
 * Handler for get messages. In context of downloads and the filesystem exposed
 * via the Guacamole protocol, get messages request the body of a file within
 * the filesystem.
 */
guac_user_get_handler guac_rdp_download_get_handler;

/**
 * Handler for put messages. In context of uploads and the filesystem exposed
 * via the Guacamole protocol, put messages request write access to a file
 * within the filesystem.
 */
guac_user_put_handler guac_rdp_upload_put_handler;

#endif

