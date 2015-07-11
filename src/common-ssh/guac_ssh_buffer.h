/*
 * Copyright (C) 2015 Glyptodon LLC
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

#ifndef GUAC_COMMON_SSH_BUFFER_H
#define GUAC_COMMON_SSH_BUFFER_H

#include "config.h"

#include <openssl/bn.h>
#include <stdint.h>

/**
 * Writes the given byte to the given buffer, advancing the buffer pointer by
 * one byte.
 *
 * @param buffer
 *     The buffer to write to.
 *
 * @param value
 *     The value to write.
 */
void guac_common_ssh_buffer_write_byte(char** buffer, uint8_t value);

/**
 * Writes the given integer to the given buffer, advancing the buffer pointer
 * four bytes.
 *
 * @param buffer
 *     The buffer to write to.
 *
 * @param value
 *     The value to write.
 */
void guac_common_ssh_buffer_write_uint32(char** buffer, uint32_t value);

/**
 * Writes the given string and its length to the given buffer, advancing the
 * buffer pointer by the size of the length (four bytes) and the size of the
 * string.
 *
 * @param buffer
 *     The buffer to write to.
 *
 * @param string
 *     The string value to write.
 *
 * @param length
 *     The length of the string to write, in bytes.
 */
void guac_common_ssh_buffer_write_string(char** buffer, const char* string,
        int length);

/**
 * Writes the given BIGNUM the given buffer, advancing the buffer pointer by
 * the size of the length (four bytes) and the size of the BIGNUM.
 *
 * @param buffer
 *     The buffer to write to.
 *
 * @param value
 *     The value to write.
 */
void guac_common_ssh_buffer_write_bignum(char** buffer, BIGNUM* value);

/**
 * Writes the given data the given buffer, advancing the buffer pointer by the
 * given length.
 *
 * @param data
 *     The arbitrary data to write.
 *
 * @param length
 *     The length of data to write, in bytes.
 */
void guac_common_ssh_buffer_write_data(char** buffer, const char* data, int length);

/**
 * Reads a single byte from the given buffer, advancing the buffer by one byte.
 *
 * @param buffer
 *     The buffer to read from.
 *
 * @return
 *     The value read from the buffer.
 */
uint8_t guac_common_ssh_buffer_read_byte(char** buffer);

/**
 * Reads an integer from the given buffer, advancing the buffer by four bytes.
 *
 * @param buffer
 *     The buffer to read from.
 *
 * @return
 *     The value read from the buffer.
 */
uint32_t guac_common_ssh_buffer_read_uint32(char** buffer);

/**
 * Reads a string and its length from the given buffer, advancing the buffer
 * by the size of the length (four bytes) and the size of the string, and
 * returning a pointer to the buffer. The length of the string is stored in
 * the given int.
 *
 * @param buffer
 *     The buffer to read from.
 *
 * @param length
 *     A pointer to an integer into which the length of the read string will
 *     be stored.
 *
 * @return
 *     A pointer to the value within the buffer.
 */
char* guac_common_ssh_buffer_read_string(char** buffer, int* length);

#endif

