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

#include "keymap.h"

#include <string.h>

const int GUAC_KEYSYMS_SHIFT[] = {0xFFE1, 0};
const int GUAC_KEYSYMS_ALL_SHIFT[] = {0xFFE1, 0xFFE2, 0};

const int GUAC_KEYSYMS_ALTGR[] = {0xFFEA, 0};
const int GUAC_KEYSYMS_SHIFT_ALTGR[] = {0xFFE1, 0xFFEA, 0};
const int GUAC_KEYSYMS_ALL_SHIFT_ALTGR[] = {0xFFE1, 0xFFE2, 0xFFEA, 0};

const int GUAC_KEYSYMS_CTRL[] = {0xFFE3, 0};
const int GUAC_KEYSYMS_ALL_CTRL[] = {0xFFE3, 0xFFE4, 0};

const int GUAC_KEYSYMS_ALT[] = {0xFFE9, 0};
const int GUAC_KEYSYMS_ALL_ALT[] = {0xFFE9, 0xFFEA, 0};

const int GUAC_KEYSYMS_CTRL_ALT[] = {0xFFE3, 0xFFE9, 0};

const int GUAC_KEYSYMS_ALL_MODIFIERS[] = {
    0xFFE1, 0xFFE2, /* Left and right shift */
    0xFFE3, 0xFFE4, /* Left and right control */
    0xFFE9, 0xFFEA, /* Left and right alt (AltGr) */
    0
};

const guac_rdp_keymap* guac_rdp_keymap_find(const char* name) {

    /* For each keymap */
    const guac_rdp_keymap** current = GUAC_KEYMAPS;
    while (*current != NULL) {

        /* If name matches, done */
        if (strcmp((*current)->name, name) == 0)
            return *current;

        current++;
    }

    /* Failure */
    return NULL;

}

