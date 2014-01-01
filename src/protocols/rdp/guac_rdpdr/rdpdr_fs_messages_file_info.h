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
