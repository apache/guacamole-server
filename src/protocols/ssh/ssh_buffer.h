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


#ifndef _GUAC_SSH_BUFFER_H
#define _GUAC_SSH_BUFFER_H

#include "config.h"

#include <openssl/bn.h>
#include <stdint.h>

/**
 * Writes the given byte to the given buffer, advancing the buffer pointer by
 * one byte.
 */
void buffer_write_byte(char** buffer, uint8_t value);

/**
 * Writes the given integer to the given buffer, advancing the buffer pointer
 * four bytes.
 */
void buffer_write_uint32(char** buffer, uint32_t value);

/**
 * Writes the given string and its length to the given buffer, advancing the
 * buffer pointer by the size of the length (four bytes) and the size of the
 * string.
 */
void buffer_write_string(char** buffer, const char* string, int length);

/**
 * Writes the given BIGNUM the given buffer, advancing the buffer pointer by
 * the size of the length (four bytes) and the size of the BIGNUM.
 */
void buffer_write_bignum(char** buffer, BIGNUM* value);

/**
 * Writes the given data the given buffer, advancing the buffer pointer by the
 * given length.
 */
void buffer_write_data(char** buffer, const char* data, int length);

/**
 * Reads a single byte from the given buffer, advancing the buffer by one byte.
 */
uint8_t buffer_read_byte(char** buffer);

/**
 * Reads an integer from the given buffer, advancing the buffer by four bytes.
 */
uint32_t buffer_read_uint32(char** buffer);

/**
 * Reads a string and its length from the given buffer, advancing the buffer
 * by the size of the length (four bytes) and the size of the string, and
 * returning a pointer to the buffer. The length of the string is stored in
 * the given int.
 */
char* buffer_read_string(char** buffer, int* length);

#endif

