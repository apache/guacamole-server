
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

#ifndef __GUAC_RDPDR_FS_H
#define __GUAC_RDPDR_FS_H

#ifdef ENABLE_WINPR
#include <winpr/stream.h>
#else
#include "compat/winpr-stream.h"
#endif

#include <guacamole/pool.h>

#include "rdpdr_service.h"

#include <freerdp/utils/svc_plugin.h>

/**
 * The index of the blob to use when sending files.
 */
#define GUAC_RDPDR_FS_BLOB 1

/**
 * The maximum number of file IDs to provide.
 */
#define GUAC_RDPDR_FS_MAX_FILES 128

/**
 * Error code returned when no more file IDs can be allocated.
 */
#define GUAC_RDPDR_FS_ENFILE -1

/**
 * Error code returned with no such file exists.
 */
#define GUAC_RDPDR_FS_ENOENT -2

/*
 * Information constants.
 */

#define FILE_SUPERSEDED   0x00000000
#define FILE_OPENED       0x00000001
#define FILE_OVERWRITTEN  0x00000003a

/*
 * File attributes.
 */

#define FILE_ATTRIBUTE_READONLY  0x00000001 
#define FILE_ATTRIBUTE_DIRECTORY 0x00000010
#define FILE_ATTRIBUTE_NORMAL    0x00000080

#define SEC_TO_UNIX_EPOCH 11644473600

/**
 * Converts a Windows timestamp (100 nanosecond intervals since Jan 1, 1601
 * UTC) to UNIX timestamp (seconds since Jan 1, 1970 UTC).
 *
 * This conversion is lossy.
 */
#define UNIX_TIME(t)    ((time_t) ((t / 10000000 + ((uint64_t) 11644473600))))

/**
 * Converts a UNIX timestamp (seconds since Jan 1, 1970 UTC) to Windows
 * timestamp (100 nanosecond intervals since Jan 1, 1601 UTC).
 */
#define WINDOWS_TIME(t) ((t - ((uint64_t) 11644473600)) * 10000000)

/**
 * Enumeration of all supported file types.
 */
typedef enum guac_rdpdr_fs_file_type {

    /**
     * A regular file - either a file or directory.
     */
    GUAC_RDPDR_FS_FILE,

    /**
     * A disk device - here, this is always virtual, and always the containing
     * volume (the Guacamole drive).
     */
    GUAC_RDPDR_FS_VOLUME,

} guac_rdpdr_fs_file_type;

/**
 * An arbitrary file on the virtual filesystem of the Guacamole drive.
 */
typedef struct guac_rdpdr_fs_file {

    /**
     * The type of this file - either a FILE (file or directory) or a
     * VOLUME (virtual file, represents the virtual device represented by
     * the Guacamole drive).
     */
    guac_rdpdr_fs_file_type type;

    /**
     * Associated local file descriptor.
     */
    int fd;

} guac_rdpdr_fs_file;

/**
 * Data specific to an instance of the printer device.
 */
typedef struct guac_rdpdr_fs_data {

    /**
     * The number of currently open files.
     */
    int open_files;

    /**
     * Pool of file IDs.
     */
    guac_pool* file_id_pool;

    /**
     * All available file structures.
     */
    guac_rdpdr_fs_file files[GUAC_RDPDR_FS_MAX_FILES];

} guac_rdpdr_fs_data;

/**
 * Registers a new filesystem device within the RDPDR plugin. This must be done
 * before RDPDR connection finishes.
 */
void guac_rdpdr_register_fs(guac_rdpdrPlugin* rdpdr);

/**
 * Returns the next available file ID, or -1 if none available.
 */
int guac_rdpdr_fs_open(guac_rdpdr_device* device, const char* path);

/**
 * Frees the given file ID, allowing future open operations to reuse it.
 */
void guac_rdpdr_fs_close(guac_rdpdr_device* device, int file_id);

#endif

