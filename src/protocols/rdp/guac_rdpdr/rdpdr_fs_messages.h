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


#ifndef __GUAC_RDPDR_FS_MESSAGES_H
#define __GUAC_RDPDR_FS_MESSAGES_H

/**
 * Handlers for core drive I/O requests. Requests handled here may be simple
 * messages handled directly, or more complex multi-type messages handled
 * elsewhere.
 *
 * @file rdpdr_fs_messages.h
 */

#include "config.h"

#include "rdpdr_service.h"

#ifdef ENABLE_WINPR
#include <winpr/stream.h>
#else
#include "compat/winpr-stream.h"
#endif

/**
 * Handles a Server Create Drive Request. Despite its name, this request opens
 * a file.
 */
void guac_rdpdr_fs_process_create(guac_rdpdr_device* device,
        wStream* input_stream, int completion_id);

/**
 * Handles a Server Close Drive Reqiest. This request closes an open file.
 */
void guac_rdpdr_fs_process_close(guac_rdpdr_device* device,
        wStream* input_stream, int file_id, int completion_id);

/**
 * Handles a Server Drive Read Request. This request reads from a file.
 */
void guac_rdpdr_fs_process_read(guac_rdpdr_device* device,
        wStream* input_stream, int file_id, int completion_id);

/**
 * Handles a Server Drive Write Request. This request writes to a file.
 */
void guac_rdpdr_fs_process_write(guac_rdpdr_device* device,
        wStream* input_stream, int file_id, int completion_id);

/**
 * Handles a Server Drive Control Request. This request handles one of any
 * number of Windows FSCTL_* control functions.
 */
void guac_rdpdr_fs_process_device_control(guac_rdpdr_device* device, wStream* input_stream,
        int file_id, int completion_id);

/**
 * Handles a Server Drive Query Volume Information Request. This request
 * queries information about the redirected volume (drive). This request
 * has several query types which have their own handlers defined in a
 * separate file.
 */
void guac_rdpdr_fs_process_volume_info(guac_rdpdr_device* device, wStream* input_stream,
        int file_id, int completion_id);

/**
 * Handles a Server Drive Set Volume Information Request. Currently, this
 * RDPDR implementation does not support setting of volume information.
 */
void guac_rdpdr_fs_process_set_volume_info(guac_rdpdr_device* device, wStream* input_stream,
        int file_id, int completion_id);

/**
 * Handles a Server Drive Query Information Request. This request queries
 * information about a specific file. This request has several query types
 * which have their own handlers defined in a separate file.
 */
void guac_rdpdr_fs_process_file_info(guac_rdpdr_device* device, wStream* input_stream,
        int file_id, int completion_id);

/**
 * Handles a Server Drive Set Information Request. This request sets
 * information about a specific file. Currently, this RDPDR implementation does
 * not support setting of file information.
 */
void guac_rdpdr_fs_process_set_file_info(guac_rdpdr_device* device, wStream* input_stream,
        int file_id, int completion_id);

/**
 * Handles a Server Drive Query Directory Request. This request queries
 * information about a specific directory. This request has several query types
 * which have their own handlers defined in a separate file.
 */
void guac_rdpdr_fs_process_query_directory(guac_rdpdr_device* device, wStream* input_stream,
        int file_id, int completion_id);

/**
 * Handles a Server Drive NotifyChange Directory Request. This request requests
 * directory change notification.
 */
void guac_rdpdr_fs_process_notify_change_directory(guac_rdpdr_device* device,
        wStream* input_stream, int file_id, int completion_id);

/**
 * Handles a Server Drive Lock Control Request. This request locks or unlocks
 * portions of a file.
 */
void guac_rdpdr_fs_process_lock_control(guac_rdpdr_device* device, wStream* input_stream,
        int file_id, int completion_id);

#endif

