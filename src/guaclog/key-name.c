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
#include "key-name.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>

/**
 * A mapping of X11 keysym to its corresponding human-readable name.
 */
typedef struct guaclog_known_key {

    /**
     * The X11 keysym of the key.
     */
    const int keysym;

    /**
     * A human-readable name for the key.
     */
    const char* name;

} guaclog_known_key;

/**
 * All known keys.
 */
const guaclog_known_key known_keys[] = {
    { 0xFFE1, "Shift" }
};

/**
 * Comparator for the standard bsearch() function which compares an integer
 * keysym against the keysym associated with a guaclog_known_key.
 *
 * @param key
 *     The key value being compared against the member. This MUST be the
 *     keysym value, passed through typecasting to an intptr_t (NOT a pointer
 *     to the int itself).
 *
 * @param member
 *     The member within the known_keys array being compared against the given
 *     key.
 *
 * @return
 *     Zero if the given keysym is equal to that of the given member, a
 *     positive value if the given keysym is greater than that of the given
 *     member, or a negative value if the given keysym is less than that of the
 *     given member.
 */
static int guaclog_known_key_bsearch_compare(const void* key,
        const void* member) {

    int keysym = (int) ((intptr_t) key);
    guaclog_known_key* current = (guaclog_known_key*) member;

    /* Compare given keysym to keysym of current member */
    return keysym  - current->keysym;

}

/**
 * Searches through the known_keys array of known keys for the name of the key
 * having the given keysym. If found, the name of the keysym is copied into the
 * given buffer, which must be at least GUACLOG_MAX_KEY_NAME_LENGTH bytes long.
 *
 * @param key_name
 *     The buffer to copy the key name into, which must be at least
 *     GUACLOG_MAX_KEY_NAME_LENGTH.
 *
 * @param keysym
 *     The X11 keysym of the key whose name should be stored in
 *     key_name.
 *
 * @return
 *     The length of the name, in bytes, excluding null terminator, or zero if
 *     the key could not be found.
 */
static int guaclog_locate_key_name(char* key_name, int keysym) {

    /* Search through known keys for given keysym */
    guaclog_known_key* found = bsearch((void*) ((intptr_t) keysym),
            known_keys, sizeof(known_keys) / sizeof(known_keys[0]),
            sizeof(known_keys[0]), guaclog_known_key_bsearch_compare);

    /* If found, format name and return length of result */
    if (found != NULL)
        return snprintf(key_name, GUACLOG_MAX_KEY_NAME_LENGTH,
                "[ %s ]", found->name);

    /* Key not found */
    return 0;

}

int guaclog_key_name(char* key_name, int keysym) {

    int name_length;

    /* Search for name within list of known keys */
    name_length = guaclog_locate_key_name(key_name, keysym);

    /* Fallback to using hex keysym as name */
    if (name_length == 0)
        name_length = snprintf(key_name, GUACLOG_MAX_KEY_NAME_LENGTH,
                "0x%X", keysym);

    /* Truncate name if necessary */
    if (name_length >= GUACLOG_MAX_KEY_NAME_LENGTH) {
        name_length = GUACLOG_MAX_KEY_NAME_LENGTH - 1;
        key_name[name_length] = '\0';
        guaclog_log(GUAC_LOG_DEBUG, "Name for key 0x%X was "
                "truncated.", keysym);
    }

    return name_length;

}

