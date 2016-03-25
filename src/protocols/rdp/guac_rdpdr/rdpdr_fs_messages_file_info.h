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


#ifndef __GUAC_RDPDR_FS_MESSAGES_FILE_INFO_H
#define __GUAC_RDPDR_FS_MESSAGES_FILE_INFO_H

/**
 * Handlers for file queries received over the RDPDR channel via the
 * IRP_MJ_QUERY_INFORMATION major function.
 *
 * @file rdpdr_fs_messages_file_info.h
 */

#include "config.h"

#include "rdpdr_service.h"

#ifdef ENABLE_WINPR
#include <winpr/stream.h>
#else
#include "compat/winpr-stream.h"
#endif

/**
 * Processes a query for FileBasicInformation. From the documentation, this is
 * "used to query a file for the times of creation, last access, last write,
 * and change, in addition to file attribute information."
 */
void guac_rdpdr_fs_process_query_basic_info(guac_rdpdr_device* device, wStream* input_stream,
        int file_id, int completion_id);

/**
 * Processes a query for FileStandardInformation. From the documentation, this
 * is "used to query for file information such as allocation size, end-of-file
 * position, and number of links."
 */
void guac_rdpdr_fs_process_query_standard_info(guac_rdpdr_device* device, wStream* input_stream,
        int file_id, int completion_id);

/**
 * Processes a query for FileAttributeTagInformation. From the documentation
 * this is "used to query for file attribute and reparse tag information."
 */
void guac_rdpdr_fs_process_query_attribute_tag_info(guac_rdpdr_device* device,
        wStream* input_stream, int file_id, int completion_id);

/**
 * Process a set operation for FileRenameInformation. From the documentation,
 * this operation is used to rename a file.
 */
void guac_rdpdr_fs_process_set_rename_info(guac_rdpdr_device* device,
        wStream* input_stream, int file_id, int completion_id, int length);

/**
 * Process a set operation for FileAllocationInformation. From the
 * documentation, this operation is used to set a file's allocation size.
 */
void guac_rdpdr_fs_process_set_allocation_info(guac_rdpdr_device* device,
        wStream* input_stream, int file_id, int completion_id, int length);

/**
 * Process a set operation for FileDispositionInformation. From the
 * documentation, this operation is used to mark a file for deletion.
 */
void guac_rdpdr_fs_process_set_disposition_info(guac_rdpdr_device* device,
        wStream* input_stream, int file_id, int completion_id, int length);

/**
 * Process a set operation for FileEndOfFileInformation. From the
 * documentation, this operation is used "to set end-of-file information for
 * a file."
 */
void guac_rdpdr_fs_process_set_end_of_file_info(guac_rdpdr_device* device,
        wStream* input_stream, int file_id, int completion_id, int length);

/**
 * Process a set operation for FileBasicInformation. From the documentation,
 * this is "used to set file information such as the times of creation, last
 * access, last write, and change, in addition to file attributes."
 */
void guac_rdpdr_fs_process_set_basic_info(guac_rdpdr_device* device,
        wStream* input_stream, int file_id, int completion_id, int length);

#endif
