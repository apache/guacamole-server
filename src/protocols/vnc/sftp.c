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

#include "config.h"

#include "common-ssh/sftp.h"
#include "sftp.h"
#include "vnc.h"

#include <guacamole/client.h>
#include <guacamole/stream.h>
#include <guacamole/user.h>

int guac_vnc_sftp_file_handler(guac_user* user, guac_stream* stream,
        char* mimetype, char* filename) {

    guac_client* client = user->client;
    guac_vnc_client* vnc_client = (guac_vnc_client*) client->data;
    guac_common_ssh_sftp_filesystem* filesystem = vnc_client->sftp_filesystem;

    /* Handle file upload */
    return guac_common_ssh_sftp_handle_file_stream(filesystem, user, stream,
            mimetype, filename);

}

