/*
 * Copyright (C) 2013 Glyptodon LLC
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

#include "rdp_keymap.h"

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

