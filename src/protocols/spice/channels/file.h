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


#ifndef GUAC_SPICE_FILE_H
#define GUAC_SPICE_FILE_H

#include "config.h"

#include "spice-constants.h"

#include <guacamole/client.h>
#include <guacamole/object.h>
#include <guacamole/pool.h>
#include <guacamole/user.h>

#include <dirent.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <spice-client-glib-2.0/spice-client.h>

/**
 * An arbitrary file on the shared folder.
 */
typedef struct guac_spice_folder_file {

    /**
     * The ID of this file.
     */
    int id;

    /**
     * The absolute path, including filename, of this file on the simulated filesystem.
     */
    char* absolute_path;

    /**
     * The real path, including filename, of this file on the local filesystem.
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
     * The pattern the check directory contents against, if any.
     */
    char dir_pattern[GUAC_SPICE_FOLDER_MAX_PATH];

    /**
     * The size of this file, in bytes.
     */
    uint64_t size;

    /**
     * The time this file was created, as a UNIX timestamp.
     */
    uint64_t ctime;

    /**
     * The time this file was last modified, as a UNIX timestamp.
     */
    uint64_t mtime;

    /**
     * The time this file was last accessed, as a UNIX timestamp.
     */
    uint64_t atime;

    /**
     * THe mode field of the file, as retrieved by a call to the stat() family
     * of functions;
     */
    mode_t stmode;

    /**
     * The number of bytes written to the file.
     */
    uint64_t bytes_written;

} guac_spice_folder_file;

/**
 * A shared folder for the Spice protocol.
 */
typedef struct guac_spice_folder {

    /**
     * The guac_client object this folder is associated with.
     */
    guac_client* client;

    /**
     * The path to the shared folder.
     */
    char* path;

    /**
     * The number of currently open files in the folder.
     */
    int open_files;

    /**
     * A pool of file IDs.
     */
    guac_pool* file_id_pool;

    /**
     * All available file structures.
     */
    guac_spice_folder_file files[GUAC_SPICE_FOLDER_MAX_FILES];

    /**
     * Whether uploads from the client to the shared folder should be disabled.
     */
    int disable_download;

    /**
     * Whether downloads from the shared folder to the client should be disabled.
     */
    int disable_upload;

    /**
     * Thread which watches the Download folder and triggers the automatic
     * download of files within this subfolder.
     */
    pthread_t download_thread;

} guac_spice_folder;

/**
 * Allocates a new filesystem given a root path which will be shared with the
 * user and the remote server via WebDAV.
 *
 * @param client
 *     The guac_client associated with the current RDP session.
 *
 * @param folder_path
 *     The local directory to use as the root directory of the shared folder.
 *
 * @param create_folder
 *     Non-zero if the folder at the path specified should be automatically
 *     created if it does not yet exist, zero otherwise.
 * 
 * @param disable_download
 *     Non-zero if downloads from the remote server to the local browser should
 *     be disabled.
 * 
 * @param disable_upload
 *     Non-zero if uploads from the browser to the remote server should be
 *     disabled.
 *
 * @return
 *     The newly-allocated filesystem.
 */
guac_spice_folder* guac_spice_folder_alloc(guac_client* client, const char* folder_path,
        int create_folder, int disable_download, int disable_upload);

/**
 * Frees the given filesystem.
 *
 * @param folder
 *     The folder to free.
 */
void guac_spice_folder_free(guac_spice_folder* folder);

/**
 * Creates and exposes a new filesystem guac_object to the given user,
 * providing access to the files within the given Spice shared folder. The
 * allocated guac_object must eventually be freed via guac_user_free_object().
 *
 * @param folder
 *     The guac_spice_folder object to expose.
 *
 * @param user
 *     The user that the folder should be exposed to.
 *
 * @return
 *     A new Guacamole filesystem object, configured to use Spice for uploading
 *     and downloading files.
 */
guac_object* guac_spice_folder_alloc_object(guac_spice_folder* folder, guac_user* user);

/**
 * Concatenates the given filename with the given path, separating the two
 * with a single forward slash. The full result must be no more than
 * GUAC_SPICE_FOLDER_MAX_PATH bytes long, counting null terminator.
 *
 * @param fullpath
 *     The buffer to store the result within. This buffer must be at least
 *     GUAC_SPICE_FOLDER_MAX_PATH bytes long.
 *
 * @param path
 *     The path to append the filename to.
 *
 * @param filename
 *     The filename to append to the path.
 *
 * @return
 *     Non-zero if the filename is valid and was successfully appended to the
 *     path, zero otherwise.
 */
int guac_spice_folder_append_filename(char* fullpath, const char* path,
        const char* filename);

/**
 * Given an arbitrary path, returns a pointer to the first character following
 * the last path separator in the path (the basename of the path). For example,
 * given "/foo/bar/baz", this function would return a pointer to "baz".
 *
 * @param path
 *     The path to determine the basename of.
 *
 * @return
 *     A pointer to the first character of the basename within the path.
 */
const char* guac_spice_folder_basename(const char* path);

/**
 * Frees the given file ID, allowing future open operations to reuse it.
 *
 * @param folder
 *     The folder containing the file to close.
 *
 * @param file_id
 *     The ID of the file to close, as returned by guac_spice_folder_open().
 */
void guac_spice_folder_close(guac_spice_folder* folder, int file_id);

/**
 * Deletes the file with the given ID.
 *
 * @param folder
 *     The folder containing the file to delete.
 *
 * @param file_id
 *     The ID of the file to delete, as returned by guac_spice_folder_open().
 *
 * @return
 *     Zero if deletion succeeded, or an error code if an error occurs. All
 *     error codes are negative values and correspond to GUAC_SPICE_FOLDER
 *     constants, such as GUAC_SPICE_FOLDER_ENOENT.
 */
int guac_spice_folder_delete(guac_spice_folder* folder, int file_id);

/**
 * Allocates a new filesystem guac_object for the given user, returning the
 * resulting guac_object. This function is provided for convenience, as it is
 * can be used as the callback for guac_client_foreach_user() or
 * guac_client_for_owner(). Note that this guac_object will be tracked
 * internally by libguac, will be provided to us in the parameters of handlers
 * related to that guac_object, and will automatically be freed when the
 * associated guac_user is freed, so the return value of this function can
 * safely be ignored.
 *
 * If either the given user or the given filesystem are NULL, then this
 * function has no effect.
 *
 * @param user
 *     The use to expose the filesystem to, or NULL if nothing should be
 *     exposed.
 *
 * @param data
 *     A pointer to the guac_spice_folder instance to expose to the given user,
 *     or NULL if nothing should be exposed.
 *
 * @return
 *     The guac_object allocated for the newly-exposed filesystem, or NULL if
 *     no filesystem object could be allocated.
 */
void* guac_spice_folder_expose(guac_user* user, void* data);

/**
 * Translates the given errno error code to a GUAC_SPICE_FOLDER error code.
 *
 * @param err
 *     The error code, as returned within errno by a system call.
 *
 * @return
 *     A GUAC_SPICE_FOLDER error code, such as GUAC_SPICE_FOLDER_ENFILE,
 *     GUAC_SPICE_FOLDER_ENOENT, etc.
 */
int guac_spice_folder_get_errorcode(int err);

/**
 * Returns the file having the given ID, or NULL if no such file exists.
 *
 * @param folder
 *     The folder containing the desired file.
 *
 * @param file_id
 *     The ID of the desired, as returned by guac_spice_folder_open().
 *
 * @return
 *     The file having the given ID, or NULL is no such file exists.
 */
guac_spice_folder_file* guac_spice_folder_get_file(guac_spice_folder* folder,
        int file_id);

/**
 * Given an arbitrary path, which may contain ".." and ".", creates an
 * absolute path which does NOT contain ".." or ".". The given path MUST
 * be absolute.
 *
 * @param path
 *     The path to normalize.
 *
 * @param abs_path
 *     The buffer to populate with the normalized path. The normalized path
 *     will not contain relative path components like ".." or ".".
 *
 * @return
 *     Zero if normalization succeeded, non-zero otherwise.
 */
int guac_spice_folder_normalize_path(const char* path, char* abs_path);

/**
 * Opens the given file, returning the a new file ID, or an error code less
 * than zero if an error occurs. The given path MUST be absolute, and will be
 * translated to be relative to the drive path of the simulated filesystem.
 *
 * @param folder
 *     The shared folder to use when opening the file.
 *
 * @param path
 *     The absolute path to the file within the simulated filesystem.
 *
 * @param flags
 *     A bitwise-OR of various standard POSIX flags to use when opening the
 *     file or directory.
 *
 * @param overwrite
 *     True if the file should be overwritten when opening it, otherwise false.
 *
 * @param directory
 *     True if the path specified is a directory, otherwise false.
 *
 * @return
 *     A new file ID, which will always be a positive value, or an error code
 *     if an error occurs. All error codes are negative values and correspond
 *     to GUAC_SPICE_FOLDER constants, such as GUAC_SPICE_FOLDER_ENOENT.
 */
int guac_spice_folder_open(guac_spice_folder* folder, const char* path,
        int flags, bool overwrite, bool directory);

/**
 * Reads up to the given length of bytes from the given offset within the
 * file having the given ID. Returns the number of bytes read, zero on EOF,
 * and an error code if an error occurs.
 *
 * @param folder
 *     The folder containing the file from which data is to be read.
 *
 * @param file_id
 *     The ID of the file to read data from, as returned by guac_spice_folder_open().
 *
 * @param offset
 *     The byte offset within the file to start reading from.
 *
 * @param buffer
 *     The buffer to fill with data from the file.
 *
 * @param length
 *     The maximum number of bytes to read from the file.
 *
 * @return
 *     The number of bytes actually read, zero on EOF, or an error code if an
 *     error occurs. All error codes are negative values and correspond to
 *     GUAC_SPICE_FOLDER constants, such as GUAC_SPICE_FOLDER_ENOENT.
 */
int guac_spice_folder_read(guac_spice_folder* folder, int file_id, uint64_t offset,
        void* buffer, int length);

/**
 * Returns the next filename within the directory having the given file ID,
 * or NULL if no more files.
 *
 * @param folder
 *     The foleer containing the file to read directory entries from.
 *
 * @param file_id
 *     The ID of the file to read directory entries from, as returned by
 *     guac_spice_folder_open().
 *
 * @return
 *     The name of the next filename within the directory, or NULL if the last
 *     file in the directory has already been returned by a previous call.
 */
const char* guac_spice_folder_read_dir(guac_spice_folder* folder, int file_id);

/**
 * Writes up to the given length of bytes from the given offset within the
 * file having the given ID. Returns the number of bytes written, and an
 * error code if an error occurs.
 *
 * @param folder
 *     The folder containing the file to which data is to be written.
 *
 * @param file_id
 *     The ID of the file to write data to, as returned by guac_spice_folder_open().
 *
 * @param offset
 *     The byte offset within the file to start writinging at.
 *
 * @param buffer
 *     The buffer containing the data to write.
 *
 * @param length
 *     The maximum number of bytes to write to the file.
 *
 * @return
 *     The number of bytes actually written, or an error code if an error
 *     occurs. All error codes are negative values and correspond to
 *     GUAC_SPICE_FOLDER constants, such as GUAC_SPICE_FOLDER_ENOENT.
 */
int guac_spice_folder_write(guac_spice_folder* folder, int file_id, uint64_t offset,
        void* buffer, int length);

/**
 * A handler that is called when the SPICE client receives notification of 
 * a new file transfer task.
 * 
 * @param main_channel
 *     The main channel associated with the SPICE session.
 * 
 * @param task
 *     The file transfer task that triggered this function call.
 * 
 * @param client
 *     The guac_client object associated with this session.
 */
void guac_spice_client_file_transfer_handler(SpiceMainChannel* main_channel,
        SpiceFileTransferTask* task, guac_client* client);

#endif /* GUAC_SPICE_FILE_H */

