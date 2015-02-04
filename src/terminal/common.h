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


#ifndef _GUAC_TERMINAL_COMMON_H
#define _GUAC_TERMINAL_COMMON_H

#include "config.h"

#include <stdbool.h>

/**
 * Returns the closest value to the value given that is also
 * within the given range.
 */
int guac_terminal_fit_to_range(int value, int min, int max);

/**
 * Encodes the given codepoint as UTF-8, storing the result within the
 * provided buffer, and returning the number of bytes stored.
 */
int guac_terminal_encode_utf8(int codepoint, char* utf8);

/**
 * Returns whether a codepoint has a corresponding glyph, or is rendered
 * as a blank space.
 */
bool guac_terminal_has_glyph(int codepoint);

/**
 * Similar to write, but automatically retries the write operation until
 * an error occurs.
 */
int guac_terminal_write_all(int fd, const char* buffer, int size);

/**
 * Similar to read, but automatically retries the read until an error occurs,
 * filling all available space within the buffer. Unless it is known that the
 * given amount of space is available on the file descriptor, there is a good
 * chance this function will block.
 *
 * @param fd
 *     The file descriptor to read data from.
 *
 * @param buffer
 *     The buffer to store data within.
 *
 * @param size
 *     The number of bytes available within the buffer.
 *
 * @return
 *     The number of bytes read if successful, or a negative value if an error
 *     occurs.
 */
int guac_terminal_fill_buffer(int fd, char* buffer, int size);

#endif

