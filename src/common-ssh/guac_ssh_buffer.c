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

#include "config.h"

#include <openssl/bn.h>
#include <openssl/ossl_typ.h>

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

void guac_common_ssh_buffer_write_byte(char** buffer, uint8_t value) {

    uint8_t* data = (uint8_t*) *buffer;
    *data = value;

    (*buffer)++;

}

void guac_common_ssh_buffer_write_uint32(char** buffer, uint32_t value) {

    uint8_t* data = (uint8_t*) *buffer;

    data[0] = (value & 0xFF000000) >> 24;
    data[1] = (value & 0x00FF0000) >> 16;
    data[2] = (value & 0x0000FF00) >> 8;
    data[3] =  value & 0x000000FF;

    *buffer += 4;

}

void guac_common_ssh_buffer_write_data(char** buffer, const char* data,
        int length) {
    memcpy(*buffer, data, length);
    *buffer += length;
}

void guac_common_ssh_buffer_write_bignum(char** buffer, BIGNUM* value) {

    unsigned char* bn_buffer;
    int length;

    /* If zero, just write zero length */
    if (BN_is_zero(value)) {
        guac_common_ssh_buffer_write_uint32(buffer, 0);
        return;
    }

    /* Allocate output buffer, add padding byte */
    length = BN_num_bytes(value);
    bn_buffer = malloc(length);

    /* Convert BIGNUM */
    BN_bn2bin(value, bn_buffer);

    /* If first byte has high bit set, write padding byte */
    if (bn_buffer[0] & 0x80) {
        guac_common_ssh_buffer_write_uint32(buffer, length+1);
        guac_common_ssh_buffer_write_byte(buffer, 0);
    }
    else
        guac_common_ssh_buffer_write_uint32(buffer, length);

    /* Write data */
    memcpy(*buffer, bn_buffer, length);
    *buffer += length;

    free(bn_buffer);

}

void guac_common_ssh_buffer_write_string(char** buffer, const char* string,
        int length) {
    guac_common_ssh_buffer_write_uint32(buffer, length);
    guac_common_ssh_buffer_write_data(buffer, string, length);
}

uint8_t guac_common_ssh_buffer_read_byte(char** buffer) {

    uint8_t* data = (uint8_t*) *buffer;
    uint8_t value = *data;

    (*buffer)++;

    return value;

}

uint32_t guac_common_ssh_buffer_read_uint32(char** buffer) {

    uint8_t* data = (uint8_t*) *buffer;
    uint32_t value =
          (data[0] << 24)
        | (data[1] << 16)
        | (data[2] <<  8)
        |  data[3];

    *buffer += 4;

    return value;

}

char* guac_common_ssh_buffer_read_string(char** buffer, int* length) {

    char* value;

    *length = guac_common_ssh_buffer_read_uint32(buffer);
    value = *buffer;

    *buffer += *length;

    return value;

}

