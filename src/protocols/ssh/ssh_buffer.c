
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is libguac-client-ssh.
 *
 * The Initial Developer of the Original Code is
 * Michael Jumper.
 * Portions created by the Initial Developer are Copyright (C) 2011
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include <stdint.h>
#include <string.h>

#include <openssl/bn.h>

void buffer_write_byte(char** buffer, uint8_t value) {

    uint8_t* data = (uint8_t*) *buffer;
    *data = value;

    (*buffer)++;

}

void buffer_write_uint32(char** buffer, uint32_t value) {

    uint8_t* data = (uint8_t*) *buffer;

    data[0] = (value & 0xFF000000) >> 24;
    data[1] = (value & 0x00FF0000) >> 16;
    data[2] = (value & 0x0000FF00) >> 8;
    data[3] =  value & 0x000000FF;

    *buffer += 4;

}

void buffer_write_data(char** buffer, const char* data, int length) {
    memcpy(*buffer, data, length);
    *buffer += length;
}

void buffer_write_bignum(char** buffer, BIGNUM* value) {

    unsigned char* bn_buffer;
    int length;

    /* If zero, just write zero length */
    if (BN_is_zero(value)) {
        buffer_write_uint32(buffer, 0);
        return;
    }

    /* Allocate output buffer, add padding byte */
    length = BN_num_bytes(value);
    bn_buffer = malloc(length);

    /* Convert BIGNUM */
    BN_bn2bin(value, bn_buffer);

    /* If first byte has high bit set, write padding byte */
    if (bn_buffer[0] & 0x80) {
        buffer_write_uint32(buffer, length+1);
        buffer_write_byte(buffer, 0);
    }
    else
        buffer_write_uint32(buffer, length);

    /* Write data */
    memcpy(*buffer, bn_buffer, length);
    *buffer += length;

    free(bn_buffer);

}

void buffer_write_string(char** buffer, const char* string, int length) {
    buffer_write_uint32(buffer, length);
    buffer_write_data(buffer, string, length);
}

uint8_t buffer_read_byte(char** buffer) {

    uint8_t* data = (uint8_t*) *buffer;
    uint8_t value = *data;

    (*buffer)++;

    return value;

}

uint32_t buffer_read_uint32(char** buffer) {

    uint8_t* data = (uint8_t*) *buffer;
    uint32_t value =
          (data[0] << 24)
        | (data[1] << 16)
        | (data[2] <<  8)
        |  data[3];

    *buffer += 4;

    return value;

}

char* buffer_read_string(char** buffer, int* length) {

    char* value;

    *length = buffer_read_uint32(buffer);
    value = *buffer;

    *buffer += *length;

    return value;

}

