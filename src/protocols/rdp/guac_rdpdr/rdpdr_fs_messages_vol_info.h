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


#ifndef __GUAC_RDPDR_FS_MESSAGES_VOL_INFO_H
#define __GUAC_RDPDR_FS_MESSAGES_VOL_INFO_H

/**
 * Handlers for directory queries received over the RDPDR channel via the
 * IRP_MJ_DIRECTORY_CONTROL major function and the IRP_MN_QUERY_DIRECTORY minor
 * function.
 *
 * @file rdpdr_fs_messages_vol_info.h
 */

#include "config.h"

#include "rdpdr_service.h"

#ifdef ENABLE_WINPR
#include <winpr/stream.h>
#else
#include "compat/winpr-stream.h"
#endif

/**
 * Processes a query request for FileFsVolumeInformation. According to the
 * documentation, this is "used to query information for a volume on which a
 * file system is mounted."
 */
void guac_rdpdr_fs_process_query_volume_info(guac_rdpdr_device* device, wStream* input_stream,
        int file_id, int completion_id);

/**
 * Processes a query request for FileFsSizeInformation.
 */
void guac_rdpdr_fs_process_query_size_info(guac_rdpdr_device* device, wStream* input_stream,
        int file_id, int completion_id);

/**
 * Processes a query request for FileFsAttributeInformation.
 */
void guac_rdpdr_fs_process_query_attribute_info(guac_rdpdr_device* device, wStream* input_stream,
        int file_id, int completion_id);

/**
 * Processes a query request for FileFsFullSizeInformation.
 */
void guac_rdpdr_fs_process_query_full_size_info(guac_rdpdr_device* device, wStream* input_stream,
        int file_id, int completion_id);

/**
 * Processes a query request for FileFsDeviceInformation.
 */
void guac_rdpdr_fs_process_query_device_info(guac_rdpdr_device* device, wStream* input_stream,
        int file_id, int completion_id);

#endif

