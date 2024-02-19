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
#include "file-ls.h"

#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/stream.h>
#include <guacamole/user.h>

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

int guac_spice_file_ls_ack_handler(guac_user* user, guac_stream* stream,
        char* message, guac_protocol_status status) {

    int blob_written = 0;
    const char* filename;

    guac_spice_file_ls_status* ls_status = (guac_spice_file_ls_status*) stream->data;

    guac_user_log(user, GUAC_LOG_DEBUG, "%s: folder=\"%s\"", __func__, ls_status->folder->path);

    /* If unsuccessful, free stream and abort */
    if (status != GUAC_PROTOCOL_STATUS_SUCCESS) {
        guac_spice_folder_close(ls_status->folder, ls_status->file_id);
        guac_user_free_stream(user, stream);
        free(ls_status);
        return 0;
    }

    /* While directory entries remain */
    while ((filename = guac_spice_folder_read_dir(ls_status->folder,
                    ls_status->file_id)) != NULL
            && !blob_written) {

        char absolute_path[GUAC_SPICE_FOLDER_MAX_PATH];

        /* Skip current and parent directory entries */
        if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0)
            continue;

        /* Concatenate into absolute path - skip if invalid */
        if (!guac_spice_folder_append_filename(absolute_path,
                    ls_status->directory_name, filename)) {

            guac_user_log(user, GUAC_LOG_DEBUG,
                    "Skipping filename \"%s\" - filename is invalid or "
                    "resulting path is too long", filename);

            continue;
        }

        guac_user_log(user, GUAC_LOG_DEBUG, "%s: absolute_path=\"%s\"", __func__, absolute_path);

        /* Attempt to open file to determine type */
        int flags = (0 | O_RDONLY);
        int file_id = guac_spice_folder_open(ls_status->folder, absolute_path,
                flags, 0, 0);
        if (file_id < 0)
            continue;

        /* Get opened file */
        guac_spice_folder_file* file = guac_spice_folder_get_file(ls_status->folder, file_id);
        if (file == NULL) {
            guac_user_log(user, GUAC_LOG_DEBUG, "%s: Successful open produced "
                    "bad file_id: %i", __func__, file_id);
            return 0;
        }

        /* Determine mimetype */
        const char* mimetype;
        if (S_ISDIR(file->stmode))
            mimetype = GUAC_USER_STREAM_INDEX_MIMETYPE;
        else
            mimetype = "application/octet-stream";

        /* Write entry */
        blob_written |= guac_common_json_write_property(user, stream,
                &ls_status->json_state, absolute_path, mimetype);

        guac_spice_folder_close(ls_status->folder, file_id);

    }

    /* Complete JSON and cleanup at end of directory */
    if (filename == NULL) {

        /* Complete JSON object */
        guac_common_json_end_object(user, stream, &ls_status->json_state);
        guac_common_json_flush(user, stream, &ls_status->json_state);

        /* Clean up resources */
        guac_spice_folder_close(ls_status->folder, ls_status->file_id);
        free(ls_status);

        /* Signal of stream */
        guac_protocol_send_end(user->socket, stream);
        guac_user_free_stream(user, stream);

    }

    guac_socket_flush(user->socket);
    return 0;

}

