/*
 * Copyright (C) 2014 Glyptodon LLC
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

#include "config.h"

#include "id.h"
#include "error.h"

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


