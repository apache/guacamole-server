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

#include <freerdp/input.h>

#ifdef HAVE_FREERDP_LOCALE_KEYBOARD_H
#include <freerdp/locale/keyboard.h>
#else
#include <freerdp/kbd/layouts.h>
#endif

static guac_rdp_keysym_desc __guac_rdp_keymap_mapping[] = {

    {0}

};

const guac_rdp_keymap guac_rdp_keymap_failsafe = {

    .name = "failsafe",

    .parent = &guac_rdp_keymap_base,
    .mapping = __guac_rdp_keymap_mapping,
    .freerdp_keyboard_layout = KBD_US

};

