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

