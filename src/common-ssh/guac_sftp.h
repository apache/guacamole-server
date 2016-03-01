/*
 * Copyright (C) 2015 Glyptodon LLC
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

#ifndef GUAC_COMMON_SSH_SFTP_H
#define GUAC_COMMON_SSH_SFTP_H

#include "guac_json.h"
#include "guac_ssh.h"

#include <guacamole/object.h>
#include <guacamole/user.h>
#include <libssh2.h>
#include <libssh2_sftp.h>

/**
 * Maximum number of bytes per path.
 */
#define GUAC_COMMON_SSH_SFTP_MAX_PATH 2048

/**
 * Representation of an SFTP-driven filesystem object. Unlike guac_object, this
 * structure is not tied to any particular user.
 */
typedef struct guac_common_ssh_sftp_filesystem {

    /**
     * The distinct SSH session used for SFTP.
     */
    guac_common_ssh_session* ssh_session;

    /**
     * SFTP session, used for file transfers.
     */
    LIBSSH2_SFTP* sftp_session;

    /**
     * The path files will be sent to, if uploaded directly via a "file"
     * instruction.
     */
    char upload_path[GUAC_COMMON_SSH_SFTP_MAX_PATH];

} guac_common_ssh_sftp_filesystem;

/**
 * The current state of a directory listing operation.
 */
typedef struct guac_common_ssh_sftp_ls_state {

    /**
     * The SFTP filesystem being listed.
     */
    guac_common_ssh_sftp_filesystem* filesystem;

    /**
     * Reference to the directory currently being listed over SFTP. This
     * directory must already be open from a call to libssh2_sftp_opendir().
     */
    LIBSSH2_SFTP_HANDLE* directory;

    /**
     * The absolute path of the directory being listed.
     */
    char directory_name[GUAC_COMMON_SSH_SFTP_MAX_PATH];

    /**
     * The current state of the JSON directory object being written.
     */
    guac_common_json_state json_state;

} guac_common_ssh_sftp_ls_state;

/**
 * Creates a new Guacamole filesystem object which provides access to files
 * and directories via SFTP using the given SSH session. When the filesystem
 * will no longer be used, it must be explicitly destroyed with
 * guac_common_ssh_destroy_sftp_filesystem(). The resulting object is not
 * automatically exposed to users of the connection - filesystem operations
 * must be mediated either through various handlers or through exposing a
 * filesystem guac_object via guac_common_ssh_alloc_sftp_filesystem_object().
 *
 * @param session
 *     The session to use to provide SFTP. This session will automatically be
 *     destroyed when this filesystem is destroyed.
 *
 * @return
 *     A new SFTP filesystem object, not yet exposed to users.
 */
guac_common_ssh_sftp_filesystem* guac_common_ssh_create_sftp_filesystem(
        guac_common_ssh_session* session);

/**
 * Destroys the given filesystem object, disconnecting from SFTP and freeing
 * and associated resources. Any associated session or user objects must be
 * explicitly destroyed.
 *
 * @param filesystem
 *     The filesystem object to destroy.
 */
void guac_common_ssh_destroy_sftp_filesystem(
        guac_common_ssh_sftp_filesystem* filesystem);

/**
 * Creates and exposes a new filesystem guac_object to the given user,
 * providing access to the files within the given SFTP filesystem. The
 * allocated guac_object must eventually be freed via guac_user_free_object().
 *
 * @param filesystem
 *     The filesystem object to expose.
 *
 * @param user
 *     The user that the SFTP filesystem should be exposed to.
 *
 * @param name
 *     The name to send as the name of the filesystem.
 *
 * @return
 *     A new Guacamole filesystem object, configured to use SFTP for uploading
 *     and downloading files.
 */
guac_object* guac_common_ssh_alloc_sftp_filesystem_object(
        guac_common_ssh_sftp_filesystem* filesystem, guac_user* user,
        const char* name);

/**
 * Initiates an SFTP file download to the user via the Guacamole "file"
 * instruction. The download will be automatically monitored and continued
 * after this function terminates in response to "ack" instructions received by
 * the user.
 *
 * @param filesystem
 *     The filesystem containing the file to be downloaded.
 *
 * @param user
 *     The user that should receive the file (via a "file" instruction).
 *
 * @param filename
 *     The filename of the file to download, relative to the given filesystem.
 *
 * @return
 *     The file stream created for the file download, already configured to
 *     properly handle "ack" responses, etc. from the user.
 */
guac_stream* guac_common_ssh_sftp_download_file(
        guac_common_ssh_sftp_filesystem* filesystem, guac_user* user,
        char* filename);

/**
 * Handles an incoming stream from a Guacamole "file" instruction, saving the
 * contents of that stream to the file having the given name within the
 * upload directory set by guac_common_ssh_sftp_set_upload_path().
 *
 * @param filesystem
 *     The filesystem that should receive the uploaded file.
 *
 * @param user
 *     The user who is attempting to open the file stream (the user that sent
 *     the "file" instruction).
 *
 * @param stream
 *     The stream through which the uploaded file data will be received.
 *
 * @param mimetype
 *     The mimetype of the data being received.
 *
 * @param filename
 *     The filename of the file to write to. This filename will always be taken
 *     relative to the upload path set by
 *     guac_common_ssh_sftp_set_upload_path().
 *
 * @return
 *     Zero if the incoming stream has been handled successfully, non-zero on
 *     failure.
 */
int guac_common_ssh_sftp_handle_file_stream(
        guac_common_ssh_sftp_filesystem* filesystem, guac_user* user,
        guac_stream* stream, char* mimetype, char* filename);

/**
 * Set the destination directory for future uploads submitted via
 * guac_common_ssh_sftp_handle_file_stream(). This function has no bearing
 * on the destination directories of files uploaded with "put" instructions.
 *
 * @param filesystem
 *     The filesystem to set the upload path of.
 *
 * @param path
 *     The path to use for future uploads submitted via the
 *     guac_common_ssh_sftp_handle_file_stream() function.
 */
void guac_common_ssh_sftp_set_upload_path(
        guac_common_ssh_sftp_filesystem* filesystem, const char* path);

#endif

