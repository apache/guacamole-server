
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

#ifndef __GUAC_RDP_FS_H
#define __GUAC_RDP_FS_H

/**
 * Functions and macros specific to filesystem handling and initialization
 * independent of RDP.  The functions here may deal with the filesystem device
 * directly, but their semantics must not deal with RDP protocol messaging.
 * Functions here represent a virtual Windows-style filesystem on top of UNIX
 * system calls and structures, using the guac_rdp_fs structure as a home
 * for common data.
 *
 * @file rdp_fs.h 
 */

#include <sys/types.h>
#include <stdint.h>
#include <dirent.h>

#include <guacamole/pool.h>

/**
 * The maximum number of file IDs to provide.
 */
#define GUAC_RDP_FS_MAX_FILES 128

/**
 * The maximum number of bytes in a path string.
 */
#define GUAC_RDP_FS_MAX_PATH 4096

/**
 * Error code returned when no more file IDs can be allocated.
 */
#define GUAC_RDP_FS_ENFILE -1

/**
 * Error code returned when no such file exists.
 */
#define GUAC_RDP_FS_ENOENT -2

/**
 * Error code returned when the operation required a directory
 * but the file was not a directory.
 */
#define GUAC_RDP_FS_ENOTDIR -3

/**
 * Error code returned when insufficient space exists to complete
 * the operation.
 */
#define GUAC_RDP_FS_ENOSPC -4

/**
 * Error code returned when the operation requires a normal file but
 * a directory was given.
 */
#define GUAC_RDP_FS_EISDIR -5

/**
 * Error code returned when permission is denied.
 */
#define GUAC_RDP_FS_EACCES -6

/**
 * Error code returned when the operation cannot be completed because the
 * file already exists.
 */
#define GUAC_RDP_FS_EEXIST -7

/**
 * Error code returned when invalid parameters were given.
 */
#define GUAC_RDP_FS_EINVAL -8

/**
 * Error code returned when the operation is not implemented.
 */
#define GUAC_RDP_FS_ENOSYS -9

/**
 * Error code returned when the operation is not supported.
 */
#define GUAC_RDP_FS_ENOTSUP -10

/*
 * Access constants.
 */
#define ACCESS_GENERIC_READ       0x80000000
#define ACCESS_GENERIC_WRITE      0x40000000
#define ACCESS_GENERIC_ALL        0x10000000
#define ACCESS_FILE_READ_DATA     0x00000001
#define ACCESS_FILE_WRITE_DATA    0x00000002
#define ACCESS_FILE_APPEND_DATA   0x00000004
#define ACCESS_DELETE             0x00010000

/*
 * Create disposition constants.
 */

#define DISP_FILE_SUPERSEDE    0x00000000
#define DISP_FILE_OPEN         0x00000001
#define DISP_FILE_CREATE       0x00000002
#define DISP_FILE_OPEN_IF      0x00000003
#define DISP_FILE_OVERWRITE    0x00000004
#define DISP_FILE_OVERWRITE_IF 0x00000005

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

/*
 * Filesystem attributes.
 */

#define FILE_UNICODE_ON_DISK 0x00000004

/*
 * File create options.
 */

#define FILE_DIRECTORY_FILE     0x00000001
#define FILE_NON_DIRECTORY_FILE 0x00000040

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
 * An arbitrary file on the virtual filesystem of the Guacamole drive.
 */
typedef struct guac_rdp_fs_file {

    /**
     * The absolute path, including filename, of this file.
     */
    char* absolute_path;

    /**
     * The real path of this file on the local filesystem.
     */
    char* real_path;

    /**
     * Associated local file descriptor.
     */
    int fd;

    /**
     * Associated directory stream, if any. This field only applies
     * if the file is being used as a directory.
     */
    DIR* dir;

    /**
     * The last read dirent structure. This is used if traversing the contents
     * of a directory.
     */
    struct dirent __dirent;

    /**
     * The pattern the check directory contents against, if any.
     */
    char dir_pattern[GUAC_RDP_FS_MAX_PATH];

    /**
     * Bitwise OR of all associated Windows file attributes.
     */
    int attributes;

    /**
     * The size of this file, in bytes.
     */
    int size;

    /**
     * The time this file was created, as a Windows timestamp.
     */
    uint64_t ctime;

    /**
     * The time this file was last modified, as a Windows timestamp.
     */
    uint64_t mtime;

    /**
     * The time this file was last accessed, as a Windows timestamp.
     */
    uint64_t atime;

} guac_rdp_fs_file;

/**
 * A virtual filesystem implementing RDP-style operations.
 */
typedef struct guac_rdp_fs {

    /**
     * The root of the filesystem.
     */
    char* drive_path;

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
    guac_rdp_fs_file files[GUAC_RDP_FS_MAX_FILES];

} guac_rdp_fs;

/**
 * Allocates a new filesystem given a root path.
 */
guac_rdp_fs* guac_rdp_fs_alloc(const char* drive_path);

/**
 * Frees the given filesystem.
 */
void guac_rdp_fs_free(guac_rdp_fs* fs);

/**
 * Converts the given relative path to an absolute path based on the given
 * parent path. If the path cannot be converted, non-zero is returned.
 */
int guac_rdp_fs_convert_path(const char* parent, const char* rel_path, char* abs_path);

/**
 * Translates the given errno error code to a GUAC_RDP_FS error code.
 */
int guac_rdp_fs_get_errorcode(int err);

/**
 * Teanslates the given GUAC_RDP_FS error code to an RDPDR status code.
 */
int guac_rdp_fs_get_status(int err);

/**
 * Returns the next available file ID, or an error code less than zero
 * if an error occurs.
 */
int guac_rdp_fs_open(guac_rdp_fs* fs, const char* path,
        int access, int file_attributes, int create_disposition,
        int create_options);

/**
 * Reads up to the given length of bytes from the given offset within the
 * file having the given ID. Returns the number of bytes read, zero on EOF,
 * and an error code if an error occurs.
 */
int guac_rdp_fs_read(guac_rdp_fs* fs, int file_id, int offset,
        void* buffer, int length);

/**
 * Writes up to the given length of bytes from the given offset within the
 * file having the given ID. Returns the number of bytes written, and an
 * error code if an error occurs.
 */
int guac_rdp_fs_write(guac_rdp_fs* fs, int file_id, int offset,
        void* buffer, int length);

/**
 * Renames (moves) the file with the given ID to the new path specified.
 * Returns zero on success, or an error code if an error occurs.
 */
int guac_rdp_fs_rename(guac_rdp_fs* fs, int file_id,
        const char* new_path);

/**
 * Frees the given file ID, allowing future open operations to reuse it.
 */
void guac_rdp_fs_close(guac_rdp_fs* fs, int file_id);

/**
 * Given an arbitrary path, which may contain ".." and ".", creates an
 * absolute path which does NOT contain ".." or ".".
 */
int guac_rdp_fs_normalize_path(const char* path, char* abs_path);

/**
 * Given a parent path and a relative path, produces a normalized absolute path.
 */
int guac_rdp_fs_convert_path(const char* parent, const char* rel_path, char* abs_path);

/**
 * Returns the next filename within the directory having the given file ID,
 * or NULL if no more files.
 */
const char* guac_rdp_fs_read_dir(guac_rdp_fs* fs, int file_id);

/**
 * Returns the file having the given ID, or NULL if no such file exists.
 */
guac_rdp_fs_file* guac_rdp_fs_get_file(guac_rdp_fs* fs, int file_id);

/**
 * Returns whether the given filename matches the given pattern.
 */
int guac_rdp_fs_matches(const char* filename, const char* pattern);

#endif

