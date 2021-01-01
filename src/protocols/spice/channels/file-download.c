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

#include "common/json.h"
#include "file-download.h"
#include "file-ls.h"
#include "file.h"
#include "spice.h"

#include <guacamole/client.h>
#include <guacamole/mem.h>
#include <guacamole/object.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/stream.h>
#include <guacamole/string.h>
#include <guacamole/user.h>

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/inotify.h>

void* guac_spice_file_download_monitor(void* data) {

    guac_spice_folder* folder = (guac_spice_folder*) data;
    char download_path[GUAC_SPICE_FOLDER_MAX_PATH];
    char download_events[GUAC_SPICE_FOLDER_MAX_EVENTS];
    char file_path[GUAC_SPICE_FOLDER_MAX_PATH];
    const struct inotify_event *event;

    guac_client_log(folder->client, GUAC_LOG_DEBUG, "%s: Starting up file monitor thread.", __func__);

    /* If folder has already been freed, or isn't open, yet, don't do anything. */
    if (folder == NULL)
        return NULL;

    download_path[0] = '\0';
    guac_strlcat(download_path, folder->path, GUAC_SPICE_FOLDER_MAX_PATH);
    guac_strlcat(download_path, "/Download", GUAC_SPICE_FOLDER_MAX_PATH);

    guac_client_log(folder->client, GUAC_LOG_DEBUG, "%s: Watching folder at path \"%s\".", __func__, download_path);

    int notify = inotify_init();

    if (notify == -1) {
        guac_client_log(folder->client, GUAC_LOG_ERROR,
                "%s: Failed to start inotify, automatic downloads will not work: %s",
                __func__, strerror(errno));
        return NULL;
    }

    if(inotify_add_watch(notify, download_path, IN_CREATE | IN_ATTRIB | IN_CLOSE_WRITE | IN_MOVED_TO | IN_ONLYDIR | IN_EXCL_UNLINK) == -1) {
        guac_client_log(folder->client, GUAC_LOG_ERROR,
                "%s: Failed to set inotify flags for \"%s\".",
                __func__, download_path);
        return NULL;
    }

    while (true) {
        int events = read(notify, download_events, sizeof(download_events));
        if (events == -1 && errno != EAGAIN) {
            guac_client_log(folder->client, GUAC_LOG_ERROR,
                    "%s: Failed to read inotify events: %s",
                    __func__, strerror(errno));
            return NULL;
        }

        if (events <= 0)
            continue;

        
        for (char* ptr = download_events; ptr < download_events + events; ptr += sizeof(struct inotify_event) + event->len) {
            
            event = (const struct inotify_event *) ptr;

            if (event->mask & IN_ISDIR) {
                guac_client_log(folder->client, GUAC_LOG_DEBUG, "%s: Ignoring event 0x%x for directory %s.", __func__, event->mask, event->name);
                continue;
            }

            guac_client_log(folder->client, GUAC_LOG_ERROR,
                    "%s: 0x%x - Downloading the file: %s", __func__, event->mask, event->name, event->cookie);

            file_path[0] = '\0';
            guac_strlcat(file_path, "/Download/", GUAC_SPICE_FOLDER_MAX_PATH);
            guac_strlcat(file_path, event->name, GUAC_SPICE_FOLDER_MAX_PATH);
            // guac_client_for_owner(folder->client, guac_spice_file_download_to_user, file_path);
            //int fileid = guac_spice_folder_open(folder, file_path, O_WRONLY, 0, 0);
            // guac_spice_folder_delete(folder, fileid);
            

        }

    }

    return NULL;

}

int guac_spice_file_download_ack_handler(guac_user* user, guac_stream* stream,
        char* message, guac_protocol_status status) {

    guac_client* client = user->client;
    guac_spice_client* spice_client = (guac_spice_client*) client->data;
    guac_spice_file_download_status* download_status = (guac_spice_file_download_status*) stream->data;

    /* Get folder, return error if no folder */
    guac_spice_folder* folder = spice_client->shared_folder;
    if (folder == NULL) {
        guac_protocol_send_ack(user->socket, stream, "FAIL (NO FOLDER)",
                GUAC_PROTOCOL_STATUS_SERVER_ERROR);
        guac_socket_flush(user->socket);
        return 0;
    }

    /* If successful, read data */
    if (status == GUAC_PROTOCOL_STATUS_SUCCESS) {

        /* Attempt read into buffer */
        char buffer[4096];
        int bytes_read = guac_spice_folder_read(folder,
                download_status->file_id,
                download_status->offset, buffer, sizeof(buffer));

        /* If bytes read, send as blob */
        if (bytes_read > 0) {
            download_status->offset += bytes_read;
            guac_protocol_send_blob(user->socket, stream,
                    buffer, bytes_read);
        }

        /* If EOF, send end */
        else if (bytes_read == 0) {
            guac_protocol_send_end(user->socket, stream);
            guac_user_free_stream(user, stream);
            free(download_status);
        }

        /* Otherwise, fail stream */
        else {
            guac_user_log(user, GUAC_LOG_ERROR,
                    "Error reading file for download");
            guac_protocol_send_end(user->socket, stream);
            guac_user_free_stream(user, stream);
            free(download_status);
        }

        guac_socket_flush(user->socket);

    }

    /* Otherwise, return stream to user */
    else
        guac_user_free_stream(user, stream);

    return 0;

}

int guac_spice_file_download_get_handler(guac_user* user, guac_object* object,
        char* name) {

    guac_client* client = user->client;
    guac_spice_client* spice_client = (guac_spice_client*) client->data;
    int flags = 0;

    /* Get folder, ignore request if no folder */
    guac_spice_folder* folder = spice_client->shared_folder;
    if (folder == NULL)
        return 0;

    flags |= O_RDONLY;

    guac_user_log(user, GUAC_LOG_DEBUG, "%s: folder->path=%s, name=%s", __func__, folder->path, name);

    /* Attempt to open file for reading */
    int file_id = guac_spice_folder_open(folder, name, flags, 0, 0);
    if (file_id < 0) {
        guac_user_log(user, GUAC_LOG_INFO, "Unable to read file \"%s\"",
                name);
        return 0;
    }

    /* Get opened file */
    guac_spice_folder_file* file = guac_spice_folder_get_file(folder, file_id);
    if (file == NULL) {
        guac_client_log(folder->client, GUAC_LOG_DEBUG,
                "%s: Successful open produced bad file_id: %i",
                __func__, file_id);
        return 0;
    }

    /* If directory, send contents of directory */
    if (S_ISDIR(file->stmode)) {

        /* Create stream data */
        guac_spice_file_ls_status* ls_status = guac_mem_alloc(sizeof(guac_spice_file_ls_status));
        ls_status->folder = folder;
        ls_status->file_id = file_id;
        guac_strlcpy(ls_status->directory_name, name,
                sizeof(ls_status->directory_name));

        /* Allocate stream for body */
        guac_stream* stream = guac_user_alloc_stream(user);
        stream->ack_handler = guac_spice_file_ls_ack_handler;
        stream->data = ls_status;

        /* Init JSON object state */
        guac_common_json_begin_object(user, stream,
                &ls_status->json_state);

        /* Associate new stream with get request */
        guac_protocol_send_body(user->socket, object, stream,
                GUAC_USER_STREAM_INDEX_MIMETYPE, name);

    }

    /* Otherwise, send file contents if downloads are allowed */
    else if (!folder->disable_download) {

        /* Create stream data */
        guac_spice_file_download_status* download_status = guac_mem_alloc(sizeof(guac_spice_file_download_status));
        download_status->file_id = file_id;
        download_status->offset = 0;

        /* Allocate stream for body */
        guac_stream* stream = guac_user_alloc_stream(user);
        stream->data = download_status;
        stream->ack_handler = guac_spice_file_download_ack_handler;

        /* Associate new stream with get request */
        guac_protocol_send_body(user->socket, object, stream,
                "application/octet-stream", name);

    }

    else
        guac_client_log(client, GUAC_LOG_INFO, "Unable to download file "
                "\"%s\", file downloads have been disabled.", name);

    guac_socket_flush(user->socket);
    return 0;
}

void* guac_spice_file_download_to_user(guac_user* user, void* data) {

    /* Do not bother attempting the download if the user has left */
    if (user == NULL)
        return NULL;

    guac_client* client = user->client;
    guac_spice_client* spice_client = (guac_spice_client*) client->data;
    guac_spice_folder* folder = spice_client->shared_folder;
    int flags = 0;

    /* Ignore download if folder has been unloaded */
    if (folder == NULL)
        return NULL;

    /* Ignore download if downloads have been disabled */
    if (folder->disable_download) {
        guac_client_log(client, GUAC_LOG_WARNING, "A download attempt has "
                "been blocked due to downloads being disabled, however it "
                "should have been blocked at a higher level. This is likely "
                "a bug.");
        return NULL;
    }

    /* Attempt to open requested file */
    char* path = (char*) data;
    flags |= O_RDONLY;
    int file_id = guac_spice_folder_open(folder, path,
            flags, 0, 0);

    /* If file opened successfully, start stream */
    if (file_id >= 0) {

        /* Associate stream with transfer status */
        guac_stream* stream = guac_user_alloc_stream(user);
        guac_spice_file_download_status* download_status = guac_mem_alloc(sizeof(guac_spice_file_download_status));
        stream->data = download_status;
        stream->ack_handler = guac_spice_file_download_ack_handler;
        download_status->file_id = file_id;
        download_status->offset = 0;

        guac_user_log(user, GUAC_LOG_DEBUG, "%s: Initiating download "
                "of \"%s\"", __func__, path);

        /* Begin stream */
        guac_protocol_send_file(user->socket, stream,
                "application/octet-stream", guac_spice_folder_basename(path));
        guac_socket_flush(user->socket);

        /* Download started successfully */
        return stream;

    }

    /* Download failed */
    guac_user_log(user, GUAC_LOG_ERROR, "Unable to download \"%s\"", path);
    return NULL;

}

