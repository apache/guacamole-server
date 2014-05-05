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


#ifndef _GUAC_TERMINAL_CHAR_MAPPINGS_H
#define _GUAC_TERMINAL_CHAR_MAPPINGS_H

#include "config.h"

/**
 * VT100 graphics mapping. Each entry is the corresponding Unicode codepoint
 * for the character N+32, where N is the index of the element in the array.
 * All characters less than 32 are universally mapped to themselves.
 */
extern const int vt100_map[];

/**
 * Null graphics mapping. Each entry is the corresponding Unicode codepoint
 * for the character N+32, where N is the index of the element in the array.
 * All characters less than 32 are universally mapped to themselves.
 */
extern const int null_map[];

/**
 * User graphics mapping. Each entry is the corresponding Unicode codepoint
 * for the character N+32, where N is the index of the element in the array.
 * All characters less than 32 are universally mapped to themselves.
 */
extern const int user_map[];

#endif

