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

