
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is libguac-client-rdp.
 *
 * The Initial Developer of the Original Code is
 * Michael Jumper.
 * Portions created by the Initial Developer are Copyright (C) 2011
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef __GUAC_RDPDR_FS_MESSAGES_H
#define __GUAC_RDPDR_FS_MESSAGES_H

/**
 * Handlers for core drive I/O requests. Requests handled here may be simple
 * messages handled directly, or more complex multi-type messages handled
 * elsewhere.
 *
 * @file rdpdr_fs_messages.h
 */

#ifdef ENABLE_WINPR
#include <winpr/stream.h>
#else
#include "compat/winpr-stream.h"
#endif

#include <guacamole/pool.h>

#include "rdpdr_service.h"

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
        int completion_id);

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

