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


#ifndef __GUAC_RDPDR_FS_MESSAGES_DIR_INFO_H
#define __GUAC_RDPDR_FS_MESSAGES_DIR_INFO_H

/**
 * Handlers for directory queries received over the RDPDR channel via the
 * IRP_MJ_DIRECTORY_CONTROL major function and the IRP_MN_QUERY_DIRECTORY minor
 * function.
 *
 * @file rdpdr_fs_messages_dir_info.h
 */

#include "config.h"

#include "rdpdr_service.h"

#ifdef ENABLE_WINPR
#include <winpr/stream.h>
#else
#include "compat/winpr-stream.h"
#endif

/**
 * Processes a query request for FileDirectoryInformation. From the
 * documentation this is "defined as the file's name, time stamp, and size, or its
 * attributes."
 */
void guac_rdpdr_fs_process_query_directory_info(guac_rdpdr_device* device,
        const char* entry_name, int file_id, int completion_id);

/**
 * Processes a query request for FileFullDirectoryInformation. From the
 * documentation, this is "defined as all the basic information, plus extended
 * attribute size."
 */
void guac_rdpdr_fs_process_query_full_directory_info(guac_rdpdr_device* device,
        const char* entry_name, int file_id, int completion_id);

/**
 * Processes a query request for FileBothDirectoryInformation. From the
 * documentation, this absurdly-named request is "basic information plus
 * extended attribute size and short name about a file or directory."
 */
void guac_rdpdr_fs_process_query_both_directory_info(guac_rdpdr_device* device,
        const char* entry_name, int file_id, int completion_id);

/**
 * Processes a query request for FileNamesInformation. From the documentation,
 * this is "detailed information on the names of files in a directory."
 */
void guac_rdpdr_fs_process_query_names_info(guac_rdpdr_device* device,
        const char* entry_name, int file_id, int completion_id);

#endif

