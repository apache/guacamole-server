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


#ifndef _GUAC_UNICODE_H
#define _GUAC_UNICODE_H

/**
 * Provides functions for manipulating Unicode strings.
 *
 * @file unicode.h
 */

#include <stddef.h>

/**
 * Given the initial byte of a single UTF-8 character, returns the overall
 * byte size of the entire character.
 *
 * @param c The initial byte of the character to check.
 * @return The number of bytes in the given character overall.
 */
size_t guac_utf8_charsize(unsigned char c);

/**
 * Given a UTF-8-encoded string, returns the length of the string in characters
 * (not bytes).
 *
 * @param str The UTF-8 string to calculate the length of.
 * @return The length in characters of the given UTF-8 string.
 */
size_t guac_utf8_strlen(const char* str);

/**
 * Given destination buffer and its length, writes the given codepoint as UTF-8
 * to the buffer, returning the number of bytes written. If there is not enough
 * space in the buffer to write the character, no bytes are written at all.
 *
 * @param codepoint The Unicode codepoint to write to the buffer.
 * @param utf8 The buffer to write to.
 * @param length The length of the buffer, in bytes.
 * @return The number of bytes written, which may be zero if there is not
 *         enough space in the buffer to write the UTF-8 character.
 */
int guac_utf8_write(int codepoint, char* utf8, int length);

/**
 * Given a buffer containing UTF-8 characters, reads the first codepoint in the
 * buffer, returning the length of the codepoint in bytes. If no codepoint
 * could be read, zero is returned.
 *
 * @param utf8 A buffer containing UTF-8 characters.
 * @param length The length of the buffer, in bytes.
 * @param codepoint A pointer to an integer which will contain the codepoint
 *                  read, if any. If no character can be read, the integer
 *                  will be left untouched.
 * @return The number of bytes read, which may be zero if there is not enough
 *         space in the buffer to read a character.
 */
int guac_utf8_read(const char* utf8, int length, int* codepoint);

#endif

