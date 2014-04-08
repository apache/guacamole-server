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

#ifndef __GUAC_COMMON_ICONV_H
#define __GUAC_COMMON_ICONV_H

#include "config.h"

/**
 * Function which reads a character from the given string data, returning
 * the Unicode codepoint read, updating the string pointer to point to the
 * byte immediately after the character read.
 */
typedef int guac_iconv_read(const char** input, int remaining);

/**
 * Function writes the character having the given Unicode codepoint value to
 * the given string data, updating the string pointer to point to the byte
 * immediately after the character written.
 */
typedef void guac_iconv_write(char** output, int remaining, int value);

/**
 * Converts characters within a given string from one encoding to another,
 * as defined by the reader/writer functions specified. The input and output
 * string pointers will be updated based on the number of bytes read or
 * written.
 *
 * @param reader The reader function to use when reading the input string.
 * @param input Pointer to the beginning of the input string.
 * @param in_remaining The number of bytes remaining after the pointer to the
 *                     input string.
 * @param writer The writer function to use when writing the output string.
 * @param output Pointer to the beginning of the output string.
 * @param out_remaining The number of bytes remaining after the pointer to the
 *                      output string.
 * @return Non-zero if the NULL terminator of the input string was read and
 *         copied into the destination string, zero otherwise.
 */
int guac_iconv(guac_iconv_read* reader, const char** input, int in_remaining,
               guac_iconv_write* writer, char** output, int out_remaining);

/**
 * Read function for UTF8.
 */
guac_iconv_read GUAC_READ_UTF8;

/**
 * Read function for UTF16.
 */
guac_iconv_read GUAC_READ_UTF16;

/**
 * Read function for CP-1252.
 */
guac_iconv_read GUAC_READ_CP1252;

/**
 * Read function for ISO-8859-1
 */
guac_iconv_read GUAC_READ_ISO8859_1;

/**
 * Write function for UTF8.
 */
guac_iconv_write GUAC_WRITE_UTF8;

/**
 * Write function for UTF16.
 */
guac_iconv_write GUAC_WRITE_UTF16;

/**
 * Write function for CP-1252.
 */
guac_iconv_write GUAC_WRITE_CP1252;

/**
 * Write function for ISO-8859-1
 */
guac_iconv_write GUAC_WRITE_ISO8859_1;

#endif

