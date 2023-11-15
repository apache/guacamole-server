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
#include <guacamole/id.h>


#ifdef CYGWIN_BUILD
#include <rpcdce.h>
#elif defined(HAVE_LIBUUID)
#include <uuid/uuid.h>
#elif defined(HAVE_OSSP_UUID_H)
#include <ossp/uuid.h>
#else
#include <uuid.h>
#endif

#include <stdlib.h>

char* guac_generate_id(char prefix) {

    char* buffer;
    char* identifier;

    /* Allocate buffer for future formatted ID */
    buffer = malloc(GUAC_UUID_LEN + 1);
    if (buffer == NULL) {
        guac_error = GUAC_STATUS_NO_MEMORY;
        guac_error_message = "Could not allocate memory for unique ID";
        return NULL;
    }

    identifier = &(buffer[1]);

#ifdef CYGWIN_BUILD

    /* Generate a UUID using a built in windows function */
    UUID uuid;
    UuidCreate(&uuid);

    /* Convert the UUID to an all-caps, null-terminated tring */
    RPC_CSTR uuid_string;
    if (UuidToString(uuid, &uuid_string) == RPC_S_OUT_OF_MEMORY)  {
        guac_error = GUAC_STATUS_NO_MEMORY;
        guac_error_message = "Could not allocate memory for unique ID";
        return NULL;
    }

    /* Copy over lowercase letters to the final target string */
    for (int i = 0; i < GUAC_UUID_LEN; i++)
        identifier[i] = tolower(uuid_string[i]);

    RpcStringFree(uuid_string);

#else

    /* Prepare object to receive generated UUID */
#ifdef HAVE_LIBUUID
    uuid_t uuid;
#else
    uuid_t* uuid;
    if (uuid_create(&uuid) != UUID_RC_OK) {
        guac_error = GUAC_STATUS_NO_MEMORY;
        guac_error_message = "Could not allocate memory for UUID";
        return NULL;
    }
#endif

    /* Generate unique identifier */
#ifdef HAVE_LIBUUID
    uuid_generate(uuid);
#else
    if (uuid_make(uuid, UUID_MAKE_V4) != UUID_RC_OK) {
        uuid_destroy(uuid);
        guac_error = GUAC_STATUS_NO_MEMORY;
        guac_error_message = "UUID generation failed";
        return NULL;
    }
#endif

    /* Convert UUID to string to produce unique identifier */
#ifdef HAVE_LIBUUID
    uuid_unparse_lower(uuid, identifier);
#else
    size_t identifier_length = GUAC_UUID_LEN;
    if (uuid_export(uuid, UUID_FMT_STR, &identifier, &identifier_length) != UUID_RC_OK) {
        free(buffer);
        uuid_destroy(uuid);
        guac_error = GUAC_STATUS_INTERNAL_ERROR;
        guac_error_message = "Conversion of UUID to unique ID failed";
        return NULL;
    }

    /* Clean up generated UUID */
    uuid_destroy(uuid);
#endif
#endif

    buffer[0] = prefix;
    buffer[GUAC_UUID_LEN] = '\0';
    return buffer;

}


