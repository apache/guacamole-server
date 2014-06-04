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


#ifndef _GUAC_TERMINAL_TYPES_H
#define _GUAC_TERMINAL_TYPES_H

#include "config.h"

#include <stdbool.h>

/**
 * A character which is not truly a character, but rather part of an
 * existing character which spans multiple columns. The original
 * character will be somewhere earlier in the row, separated from
 * this character by a contiguous string of zero of more
 * GUAC_CHAR_CONTINUATION characters.
 */
#define GUAC_CHAR_CONTINUATION -1

/**
 * An RGB color, where each component ranges from 0 to 255.
 */
typedef struct guac_terminal_color {

    /**
     * The red component of this color.
     */
    int red;

    /**
     * The green component of this color.
     */
    int green;

    /**
     * The blue component of this color.
     */
    int blue;

} guac_terminal_color;

/**
 * Terminal attributes, as can be applied to a single character.
 */
typedef struct guac_terminal_attributes {

    /**
     * Whether the character should be rendered bold.
     */
    bool bold;

    /**
     * Whether the character should be rendered with reversed colors
     * (background becomes foreground and vice-versa).
     */
    bool reverse;

    /**
     * Whether the associated character is highlighted by the cursor.
     */
    bool cursor;

    /**
     * Whether to render the character with underscore.
     */
    bool underscore;

    /**
     * The foreground color of this character, as a palette index.
     */
    int foreground;

    /**
     * The background color of this character, as a palette index.
     */
    int background;

} guac_terminal_attributes;

/**
 * Represents a single character for display in a terminal, including actual
 * character value, foreground color, and background color.
 */
typedef struct guac_terminal_char {

    /**
     * The Unicode codepoint of the character to display, or
     * GUAC_CHAR_CONTINUATION if this character is part of
     * another character which spans multiple columns.
     */
    int value;

    /**
     * The attributes of the character to display.
     */
    guac_terminal_attributes attributes;

    /**
     * The number of columns this character occupies. If the character is
     * GUAC_CHAR_CONTINUATION, this value is undefined and not applicable.
     */
    int width;

} guac_terminal_char;

#endif

