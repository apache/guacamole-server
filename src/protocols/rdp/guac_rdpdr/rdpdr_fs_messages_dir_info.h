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

