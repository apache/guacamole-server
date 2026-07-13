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

#include "file.h"
#include "spice.h"
#include "file-upload.h"

#include <guacamole/client.h>
#include <guacamole/mem.h>
#include <guacamole/object.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/stream.h>
#include <guacamole/user.h>

#include <gio/gio.h>

#include <fcntl.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/**
 * The maximum number of bytes a single SPICE-agent upload may stage to the
 * guacd host's temporary directory before it is pushed into the guest. This
 * bounds host disk/tmpfs consumption from an over-large or endless upload
 * stream (CWE-400 / CWE-770). Uploads exceeding this are rejected and the
 * partial staged file is removed.
 */
#define GUAC_SPICE_MAX_AGENT_UPLOAD ((uint64_t) 2 * 1024 * 1024 * 1024)

/**
 * Writes the given filename to the given upload path, sanitizing the filename
 * and translating the filename to the root directory.
 *
 * @param filename
 *     The filename to sanitize and move to the root directory.
 *
 * @param path
 *     A pointer to a buffer which should receive the sanitized path. The
 *     buffer must have at least GUAC_SPICE_FOLDER_MAX_PATH bytes available.
 */
static void __generate_upload_path(const char* filename, char* path) {

    int i;

    /* Add initial slash (the shared folder uses absolute, forward-slash
     * paths, as required by guac_spice_folder_normalize_path()) */
    *(path++) = '/';

    for (i=1; i<GUAC_SPICE_FOLDER_MAX_PATH; i++) {

        /* Get current, stop at end */
        char c = *(filename++);
        if (c == '\0')
            break;

        /* Replace special characters with underscores */
        if (c == '/' || c == '\\')
            c = '_';

        *(path++) = c;

    }

    /* Terminate path */
    *path = '\0';

}

int guac_spice_file_upload_file_handler(guac_user* user, guac_stream* stream,
        char* mimetype, char* filename) {

    guac_client* client = user->client;
    guac_spice_client* spice_client = (guac_spice_client*) client->data;

    int file_id;
    char file_path[GUAC_SPICE_FOLDER_MAX_PATH];

    /* Get filesystem, return error if no filesystem */
    guac_spice_folder* folder = spice_client->shared_folder;
    if (folder == NULL) {
        guac_protocol_send_ack(user->socket, stream, "FAIL (NO FS)",
                GUAC_PROTOCOL_STATUS_SERVER_ERROR);
        guac_socket_flush(user->socket);
        return 0;
    }

    /* Ignore upload if uploads have been disabled */
    if (folder->disable_upload) {
        guac_client_log(client, GUAC_LOG_WARNING, "A upload attempt has "
                "been blocked due to uploads being disabled, however it "
                "should have been blocked at a higher level. This is likely "
                "a bug.");
        guac_protocol_send_ack(user->socket, stream, "FAIL (UPLOAD DISABLED)",
                GUAC_PROTOCOL_STATUS_CLIENT_FORBIDDEN);
        guac_socket_flush(user->socket);
        return 0;
    }

    /* Translate name */
    __generate_upload_path(filename, file_path);

    /* Open file */
    file_id = guac_spice_folder_open(folder, file_path, (O_WRONLY | O_CREAT | O_TRUNC),
            1, 0);
    if (file_id < 0) {
        guac_protocol_send_ack(user->socket, stream, "FAIL (CANNOT OPEN)",
                GUAC_PROTOCOL_STATUS_CLIENT_FORBIDDEN);
        guac_socket_flush(user->socket);
        return 0;
    }

    /* Init upload status */
    guac_spice_file_upload_status* upload_status = guac_mem_alloc(sizeof(guac_spice_file_upload_status));
    upload_status->offset = 0;
    upload_status->file_id = file_id;
    stream->data = upload_status;
    stream->blob_handler = guac_spice_file_upload_blob_handler;
    stream->end_handler = guac_spice_file_upload_end_handler;

    guac_protocol_send_ack(user->socket, stream, "OK (STREAM BEGIN)",
            GUAC_PROTOCOL_STATUS_SUCCESS);
    guac_socket_flush(user->socket);
    return 0;

}

int guac_spice_file_upload_blob_handler(guac_user* user, guac_stream* stream,
        void* data, int length) {

    int bytes_written;
    guac_spice_file_upload_status* upload_status = (guac_spice_file_upload_status*) stream->data;

    /* Get filesystem, return error if no filesystem */
    guac_client* client = user->client;
    guac_spice_client* spice_client = (guac_spice_client*) client->data;
    guac_spice_folder* folder = spice_client->shared_folder;
    if (folder == NULL) {
        guac_protocol_send_ack(user->socket, stream, "FAIL (NO FOLDER)",
                GUAC_PROTOCOL_STATUS_SERVER_ERROR);
        guac_socket_flush(user->socket);
        return 0;
    }

    /* Write entire block */
    while (length > 0) {

        /* Attempt write */
        bytes_written = guac_spice_folder_write(folder, upload_status->file_id,
                upload_status->offset, data, length);

        /* On error, abort */
        if (bytes_written < 0) {
            guac_protocol_send_ack(user->socket, stream,
                    "FAIL (BAD WRITE)",
                    GUAC_PROTOCOL_STATUS_CLIENT_FORBIDDEN);
            guac_socket_flush(user->socket);
            return 0;
        }

        /* Update counters */
        upload_status->offset += bytes_written;
        data = (char *)data + bytes_written;
        length -= bytes_written;

    }

    guac_protocol_send_ack(user->socket, stream, "OK (DATA RECEIVED)",
            GUAC_PROTOCOL_STATUS_SUCCESS);
    guac_socket_flush(user->socket);
    return 0;

}

int guac_spice_file_upload_end_handler(guac_user* user, guac_stream* stream) {

    guac_client* client = user->client;
    guac_spice_client* spice_client = (guac_spice_client*) client->data;
    guac_spice_file_upload_status* upload_status = (guac_spice_file_upload_status*) stream->data;

    /* Get folder, return error if no filesystem */
    guac_spice_folder* folder = spice_client->shared_folder;
    if (folder == NULL) {
        guac_protocol_send_ack(user->socket, stream, "FAIL (NO FOLDER)",
                GUAC_PROTOCOL_STATUS_SERVER_ERROR);
        guac_socket_flush(user->socket);
        return 0;
    }

    /* Close file */
    guac_spice_folder_close(folder, upload_status->file_id);

    /* Acknowledge stream end */
    guac_protocol_send_ack(user->socket, stream, "OK (STREAM END)",
            GUAC_PROTOCOL_STATUS_SUCCESS);
    guac_socket_flush(user->socket);

    free(upload_status);
    return 0;

}

int guac_spice_file_upload_put_handler(guac_user* user, guac_object* object,
        guac_stream* stream, char* mimetype, char* name) {

    guac_client* client = user->client;
    guac_spice_client* spice_client = (guac_spice_client*) client->data;

    /* Get folder, return error if no filesystem */
    guac_spice_folder* folder = spice_client->shared_folder;
    if (folder == NULL) {
        guac_protocol_send_ack(user->socket, stream, "FAIL (NO FOLDER)",
                GUAC_PROTOCOL_STATUS_SERVER_ERROR);
        guac_socket_flush(user->socket);
        return 0;
    }

    /* Ignore upload if uploads have been disabled */
    if (folder->disable_upload) {
        guac_client_log(client, GUAC_LOG_WARNING, "A upload attempt has "
                "been blocked due to uploads being disabled, however it "
                "should have been blocked at a higher level. This is likely "
                "a bug.");
        guac_protocol_send_ack(user->socket, stream, "FAIL (UPLOAD DISABLED)",
                GUAC_PROTOCOL_STATUS_CLIENT_FORBIDDEN);
        guac_socket_flush(user->socket);
        return 0;
    }

    /* Open file */
    int file_id = guac_spice_folder_open(folder, name, (O_WRONLY | O_CREAT | O_TRUNC),
            1, 0);

    /* Abort on failure */
    if (file_id < 0) {
        guac_protocol_send_ack(user->socket, stream, "FAIL (CANNOT OPEN)",
                GUAC_PROTOCOL_STATUS_CLIENT_FORBIDDEN);
        guac_socket_flush(user->socket);
        return 0;
    }

    /* Init upload stream data */
    guac_spice_file_upload_status* upload_status = guac_mem_alloc(sizeof(guac_spice_file_upload_status));
    upload_status->offset = 0;
    upload_status->file_id = file_id;

    /* Allocate stream, init for file upload */
    stream->data = upload_status;
    stream->blob_handler = guac_spice_file_upload_blob_handler;
    stream->end_handler = guac_spice_file_upload_end_handler;

    /* Acknowledge stream creation */
    guac_protocol_send_ack(user->socket, stream, "OK (STREAM BEGIN)",
            GUAC_PROTOCOL_STATUS_SUCCESS);
    guac_socket_flush(user->socket);
    return 0;
}

/**
 * The in-progress state of a single file being uploaded directly to the guest
 * via the SPICE agent. The file is staged on the guacd host under a private
 * temporary directory (named with the original filename so it reaches the guest
 * unchanged) and then handed to spice_main_channel_file_copy_async().
 */
typedef struct guac_spice_agent_upload {

    /**
     * The guac_client associated with the upload, used for logging.
     */
    guac_client* client;

    /**
     * The private temporary directory created to stage the file, or NULL.
     */
    char* tmpdir;

    /**
     * The full path of the staged file within tmpdir, or NULL.
     */
    char* tmppath;

    /**
     * The sanitized original filename, used for logging.
     */
    char* filename;

    /**
     * The open file descriptor of the staged file while blobs are being
     * written, or -1 once closed.
     */
    int fd;

    /**
     * Non-zero if the upload was aborted (e.g. a write to the staged file
     * failed). An aborted upload must not be pushed to the guest, as the staged
     * file would be truncated/incomplete.
     */
    int aborted;

    /**
     * The total number of bytes staged so far.
     */
    uint64_t bytes;

} guac_spice_agent_upload;

/**
 * Frees a SPICE-agent upload context, removing the staged file and its
 * temporary directory. Safe to call with NULL.
 *
 * @param upload
 *     The upload context to free.
 */
static void guac_spice_agent_upload_free(guac_spice_agent_upload* upload) {

    if (upload == NULL)
        return;

    if (upload->fd >= 0)
        close(upload->fd);

    if (upload->tmppath != NULL) {
        unlink(upload->tmppath);
        g_free(upload->tmppath);
    }

    if (upload->tmpdir != NULL) {
        rmdir(upload->tmpdir);
        g_free(upload->tmpdir);
    }

    g_free(upload->filename);
    g_free(upload);

}

/**
 * Sanitizes a client-supplied filename into a bare basename (no path
 * separators), suitable for use as the name of the staged file.
 *
 * @param filename
 *     The client-supplied filename.
 *
 * @param name
 *     A buffer of at least GUAC_SPICE_FOLDER_MAX_PATH bytes to receive the
 *     sanitized basename.
 */
static void __sanitize_filename(const char* filename, char* name) {

    int i;

    for (i = 0; i < GUAC_SPICE_FOLDER_MAX_PATH - 1; i++) {

        char c = filename[i];
        if (c == '\0')
            break;

        /* Replace path separators to keep the name a bare basename */
        if (c == '/' || c == '\\')
            c = '_';

        name[i] = c;

    }

    name[i] = '\0';

    /* Never allow an empty name or the special directory entries "." / ".." —
     * substitute a safe default so the staged basename is always a real file */
    if (i == 0 || strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
        strcpy(name, "upload.bin");

}

/**
 * GAsyncReadyCallback invoked on the SPICE event-loop thread once a direct
 * guest upload has completed (or failed). Logs the outcome with explicit
 * direction and mechanism for auditing, then frees the upload context.
 */
static void guac_spice_agent_copy_ready(GObject* source_object,
        GAsyncResult* result, gpointer user_data) {

    guac_spice_agent_upload* upload = (guac_spice_agent_upload*) user_data;
    SpiceMainChannel* main_channel = SPICE_MAIN_CHANNEL(source_object);
    GError* error = NULL;

    if (spice_main_channel_file_copy_finish(main_channel, result, &error))
        guac_client_log(upload->client, GUAC_LOG_INFO,
                "File transfer client->guest complete: \"%s\" (%" PRIu64
                " bytes) pushed into the guest via the SPICE agent (the guest "
                "agent saves it to the user's download/desktop location).",
                upload->filename, upload->bytes);
    else
        guac_client_log(upload->client, GUAC_LOG_WARNING,
                "File transfer client->guest failed for \"%s\" via the SPICE "
                "agent: %s", upload->filename,
                error != NULL ? error->message : "unknown error");

    g_clear_error(&error);
    guac_spice_agent_upload_free(upload);

}

/**
 * Deferred-call handler which starts the asynchronous SPICE agent file copy on
 * the event-loop thread. Ownership of the upload context is taken from the
 * deferred call (call->data is cleared so it is not double-freed) and handed to
 * the async callback.
 */
static void guac_spice_agent_copy_dispatch(guac_spice_deferred_call* call) {

    guac_spice_agent_upload* upload = (guac_spice_agent_upload*) call->data;

    /* Take ownership; prevent guac_spice_deferred_free() from freeing it */
    call->data = NULL;

    if (upload == NULL)
        return;

    /* Read the live main channel on the event-loop thread, which is the
     * authoritative owner of this pointer. Capturing it on the user thread
     * would risk a dangling reference if the channel were torn down before
     * dispatch. */
    guac_spice_client* spice_client =
            (guac_spice_client*) upload->client->data;
    SpiceMainChannel* main_channel = spice_client->main_channel;

    /* Verify the SPICE agent is connected before attempting the push. This is
     * the authoritative check (the user-thread handler only did a cheap
     * NULL-channel pre-screen). */
    gboolean agent_connected = FALSE;
    if (main_channel != NULL)
        g_object_get(main_channel, "agent-connected", &agent_connected, NULL);

    if (!agent_connected) {
        guac_client_log(upload->client, GUAC_LOG_WARNING,
                "File transfer client->guest aborted for \"%s\": the SPICE "
                "guest agent is not connected.", upload->filename);
        guac_spice_agent_upload_free(upload);
        return;
    }

    /* Push the staged file into the guest. spice-gtk refs the source GFiles
     * internally, so the local array/reference can be released immediately. */
    GFile* source = g_file_new_for_path(upload->tmppath);
    GFile* sources[] = { source, NULL };

    spice_main_channel_file_copy_async(main_channel, sources,
            G_FILE_COPY_NONE, NULL, NULL, NULL,
            guac_spice_agent_copy_ready, upload);

    g_object_unref(source);

}

int guac_spice_file_upload_agent_handler(guac_user* user, guac_stream* stream,
        char* mimetype, char* filename) {

    guac_client* client = user->client;
    guac_spice_client* spice_client = (guac_spice_client*) client->data;

    /* Cheap pre-screen on the user thread: reject early if there is no SPICE
     * main channel at all. The authoritative "agent-connected" check is done on
     * the event-loop thread just before the push (guac_spice_agent_copy_dispatch),
     * since spice-gtk objects are owned by that thread. */
    if (spice_client->main_channel == NULL) {
        guac_client_log(client, GUAC_LOG_WARNING, "Direct upload to guest "
                "rejected: the SPICE guest agent is not connected.");
        guac_protocol_send_ack(user->socket, stream,
                "FAIL (AGENT UNAVAILABLE)",
                GUAC_PROTOCOL_STATUS_UPSTREAM_UNAVAILABLE);
        guac_socket_flush(user->socket);
        return 0;
    }

    /* Stage the incoming file under a private temporary directory, keeping the
     * original filename so it reaches the guest unchanged */
    GError* error = NULL;
    char* tmpdir = g_dir_make_tmp("guac-spice-upload-XXXXXX", &error);
    if (tmpdir == NULL) {
        guac_client_log(client, GUAC_LOG_ERROR, "Unable to stage direct "
                "upload: %s", error != NULL ? error->message
                : "could not create temporary directory");
        g_clear_error(&error);
        guac_protocol_send_ack(user->socket, stream, "FAIL (NO STAGING)",
                GUAC_PROTOCOL_STATUS_SERVER_ERROR);
        guac_socket_flush(user->socket);
        return 0;
    }

    char safe_name[GUAC_SPICE_FOLDER_MAX_PATH];
    __sanitize_filename(filename, safe_name);

    /* Create the staged file fresh within the private temp dir. O_EXCL |
     * O_NOFOLLOW guarantee we create a brand-new regular file and never follow
     * a pre-planted symlink (defense-in-depth on top of the 0700 temp dir). */
    char* tmppath = g_build_filename(tmpdir, safe_name, NULL);
    int fd = open(tmppath, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0600);
    if (fd < 0) {
        guac_client_log(client, GUAC_LOG_ERROR, "Unable to stage direct "
                "upload file \"%s\".", tmppath);
        rmdir(tmpdir);
        g_free(tmppath);
        g_free(tmpdir);
        guac_protocol_send_ack(user->socket, stream, "FAIL (CANNOT OPEN)",
                GUAC_PROTOCOL_STATUS_SERVER_ERROR);
        guac_socket_flush(user->socket);
        return 0;
    }

    /* Track upload state for the duration of the stream */
    guac_spice_agent_upload* upload = g_new0(guac_spice_agent_upload, 1);
    upload->client = client;
    upload->tmpdir = tmpdir;
    upload->tmppath = tmppath;
    upload->filename = g_strdup(safe_name);
    upload->fd = fd;
    upload->bytes = 0;

    stream->data = upload;
    stream->blob_handler = guac_spice_file_upload_agent_blob_handler;
    stream->end_handler = guac_spice_file_upload_agent_end_handler;

    guac_protocol_send_ack(user->socket, stream, "OK (STREAM BEGIN)",
            GUAC_PROTOCOL_STATUS_SUCCESS);
    guac_socket_flush(user->socket);
    return 0;

}

int guac_spice_file_upload_agent_blob_handler(guac_user* user,
        guac_stream* stream, void* data, int length) {

    guac_spice_agent_upload* upload = (guac_spice_agent_upload*) stream->data;

    if (upload == NULL || upload->fd < 0) {
        guac_protocol_send_ack(user->socket, stream, "FAIL (NO UPLOAD)",
                GUAC_PROTOCOL_STATUS_SERVER_ERROR);
        guac_socket_flush(user->socket);
        return 0;
    }

    /* Enforce a maximum staged size so an over-large or endless upload cannot
     * exhaust the guacd host's temporary storage. Abort the transfer and let
     * the end/free path remove the partial file. */
    if (length < 0 || upload->bytes + (uint64_t) length
            > GUAC_SPICE_MAX_AGENT_UPLOAD) {
        guac_client_log(user->client, GUAC_LOG_WARNING, "Direct upload to "
                "guest aborted: exceeds the maximum staged size of %" PRIu64
                " bytes.", (uint64_t) GUAC_SPICE_MAX_AGENT_UPLOAD);
        guac_protocol_send_ack(user->socket, stream, "FAIL (TOO LARGE)",
                GUAC_PROTOCOL_STATUS_CLIENT_OVERRUN);
        guac_socket_flush(user->socket);
        close(upload->fd);
        upload->fd = -1;
        upload->aborted = 1;
        return 0;
    }

    /* Write the entire block to the staged file */
    int total = 0;
    while (total < length) {

        ssize_t written = write(upload->fd, (char*) data + total,
                length - total);

        if (written < 0) {
            guac_protocol_send_ack(user->socket, stream, "FAIL (BAD WRITE)",
                    GUAC_PROTOCOL_STATUS_SERVER_ERROR);
            guac_socket_flush(user->socket);
            close(upload->fd);
            upload->fd = -1;
            upload->aborted = 1;
            return 0;
        }

        total += written;

    }

    upload->bytes += length;

    guac_protocol_send_ack(user->socket, stream, "OK (DATA RECEIVED)",
            GUAC_PROTOCOL_STATUS_SUCCESS);
    guac_socket_flush(user->socket);
    return 0;

}

int guac_spice_file_upload_agent_end_handler(guac_user* user,
        guac_stream* stream) {

    guac_spice_agent_upload* upload = (guac_spice_agent_upload*) stream->data;

    /* Detach upload state from the stream */
    stream->data = NULL;

    if (upload == NULL) {
        guac_protocol_send_ack(user->socket, stream, "FAIL (NO UPLOAD)",
                GUAC_PROTOCOL_STATUS_SERVER_ERROR);
        guac_socket_flush(user->socket);
        return 0;
    }

    /* Finish writing the staged file */
    if (upload->fd >= 0) {
        close(upload->fd);
        upload->fd = -1;
    }

    /* If the upload was aborted mid-stream (e.g. a staging write failed), do
     * not push the truncated file to the guest — just clean up. The failure was
     * already reported to the client on the failing blob. */
    if (upload->aborted) {
        guac_spice_agent_upload_free(upload);
        guac_protocol_send_ack(user->socket, stream, "FAIL (UPLOAD ABORTED)",
                GUAC_PROTOCOL_STATUS_SERVER_ERROR);
        guac_socket_flush(user->socket);
        return 0;
    }

    /* Hand the staged file to the SPICE agent on the event-loop thread. The
     * deferred call takes ownership of the upload context via call->data; if the
     * loop terminates before dispatch, data_destroy ensures the staged file and
     * its temp dir are still cleaned up. The live main channel is resolved on
     * the loop thread within the dispatch handler, so it is not captured here. */
    guac_spice_deferred_call* call = g_new0(guac_spice_deferred_call, 1);
    call->handler = guac_spice_agent_copy_dispatch;
    call->data = upload;
    call->data_destroy = (GDestroyNotify) guac_spice_agent_upload_free;
    guac_spice_defer_call(call);

    guac_protocol_send_ack(user->socket, stream, "OK (STREAM END)",
            GUAC_PROTOCOL_STATUS_SUCCESS);
    guac_socket_flush(user->socket);
    return 0;

}

