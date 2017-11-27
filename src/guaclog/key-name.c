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
    { 0x0020, "Space" },
    { 0xFE03, "AltGr" },
    { 0xFF08, "Backspace" },
    { 0xFF09, "Tab" },
    { 0xFF0B, "Clear" },
    { 0xFF0D, "Return" },
    { 0xFF13, "Pause" },
    { 0xFF1B, "Escape" },
    { 0xFF51, "Left" },
    { 0xFF52, "Up" },
    { 0xFF53, "Right" },
    { 0xFF54, "Down" },
    { 0xFF55, "Page Up" },
    { 0xFF56, "Page Down" },
    { 0xFF63, "Insert" },
    { 0xFF65, "Undo" },
    { 0xFF6A, "Help" },
    { 0xFF80, "Space" },
    { 0xFF8D, "Enter" },
    { 0xFFBD, "Equals" },
    { 0xFFBE, "F1" },
    { 0xFFBF, "F2" },
    { 0xFFC0, "F3" },
    { 0xFFC1, "F4" },
    { 0xFFC2, "F5" },
    { 0xFFC3, "F6" },
    { 0xFFC4, "F7" },
    { 0xFFC5, "F8" },
    { 0xFFC6, "F9" },
    { 0xFFC7, "F10" },
    { 0xFFC8, "F11" },
    { 0xFFC9, "F12" },
    { 0xFFCA, "F13" },
    { 0xFFCB, "F14" },
    { 0xFFCC, "F15" },
    { 0xFFCD, "F16" },
    { 0xFFCE, "F17" },
    { 0xFFCF, "F18" },
    { 0xFFD0, "F19" },
    { 0xFFD1, "F20" },
    { 0xFFD2, "F21" },
    { 0xFFD3, "F22" },
    { 0xFFD4, "F23" },
    { 0xFFD5, "F24" },
    { 0xFFE1, "Shift" },
    { 0xFFE2, "Shift" },
    { 0xFFE3, "Ctrl" },
    { 0xFFE4, "Ctrl" },
    { 0xFFE5, "Caps" },
    { 0xFFE7, "Meta" },
    { 0xFFE8, "Meta" },
    { 0xFFE9, "Alt" },
    { 0xFFEA, "Alt" },
    { 0xFFEB, "Super" },
    { 0xFFEB, "Win" },
    { 0xFFEC, "Super" },
    { 0xFFED, "Hyper" },
    { 0xFFEE, "Hyper" },
    { 0xFFFF, "Delete" }
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

/**
 * Produces a name for the key having the given keysym using its corresponding
 * Unicode character. If possible, the name of the keysym is copied into the
 * given buffer, which must be at least GUAC_MAX_KEY_NAME_LENGTH bytes long.
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
 *     a readable name cannot be directly produced via Unicode alone.
 */
static int guaclog_unicode_key_name(char* key_name, int keysym) {

    int i;
    int mask, bytes;

    /* Translate only if keysym maps to Unicode */
    if (keysym < 0x00 || (keysym > 0xFF && (keysym & 0xFFFF0000) != 0x01000000))
        return 0;

    /* Do not translate whitespace - it will be unreadable */
    if (keysym == 0x20)
        return 0;

    int codepoint = keysym & 0xFFFF;

    /* Determine size and initial byte mask */
    if (codepoint <= 0x007F) {
        mask  = 0x00;
        bytes = 1;
    }
    else if (codepoint <= 0x7FF) {
        mask  = 0xC0;
        bytes = 2;
    }
    else if (codepoint <= 0xFFFF) {
        mask  = 0xE0;
        bytes = 3;
    }
    else if (codepoint <= 0x1FFFFF) {
        mask  = 0xF0;
        bytes = 4;
    }

    /* Otherwise, invalid codepoint */
    else {
        *(key_name++) = '?';
        return 1;
    }

    /* Offset buffer by size */
    key_name += bytes;

    /* Add null terminator */
    *(key_name--) = '\0';

    /* Add trailing bytes, if any */
    for (i=1; i<bytes; i++) {
        *(key_name--) = 0x80 | (codepoint & 0x3F);
        codepoint >>= 6;
    }

    /* Set initial byte */
    *key_name = mask | codepoint;

    /* Done */
    return bytes;

}

int guaclog_key_name(char* key_name, int keysym) {

    int name_length;

    /* Attempt to translate straight into a Unicode character */
    name_length = guaclog_unicode_key_name(key_name, keysym);

    /* If not Unicode, search for name within list of known keys */
    if (name_length == 0)
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

