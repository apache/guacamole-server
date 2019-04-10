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

#include "guacamole/error.h"
#include "id.h"

#ifdef HAVE_OSSP_UUID_H
#include <ossp/uuid.h>
#else
#include <uuid.h>
#endif

#include <stdlib.h>

char* guac_generate_id(char prefix) {

    char* buffer;
    char* identifier;
    size_t identifier_length;

    uuid_t* uuid;

    /* Attempt to create UUID object */
    if (uuid_create(&uuid) != UUID_RC_OK) {
        guac_error = GUAC_STATUS_NO_MEMORY;
        guac_error_message = "Could not allocate memory for UUID";
        return NULL;
    }

    /* Generate random UUID */
    if (uuid_make(uuid, UUID_MAKE_V4) != UUID_RC_OK) {
        uuid_destroy(uuid);
        guac_error = GUAC_STATUS_NO_MEMORY;
        guac_error_message = "UUID generation failed";
        return NULL;
    }

    /* Allocate buffer for future formatted ID */
    buffer = malloc(UUID_LEN_STR + 2);
    if (buffer == NULL) {
        uuid_destroy(uuid);
        guac_error = GUAC_STATUS_NO_MEMORY;
        guac_error_message = "Could not allocate memory for connection ID";
        return NULL;
    }

    identifier = &(buffer[1]);
    identifier_length = UUID_LEN_STR + 1;

    /* Build connection ID from UUID */
    if (uuid_export(uuid, UUID_FMT_STR, &identifier, &identifier_length) != UUID_RC_OK) {
        free(buffer);
        uuid_destroy(uuid);
        guac_error = GUAC_STATUS_INTERNAL_ERROR;
        guac_error_message = "Conversion of UUID to connection ID failed";
        return NULL;
    }

    uuid_destroy(uuid);

    buffer[0] = prefix;
    buffer[UUID_LEN_STR + 1] = '\0';
    return buffer;

}


