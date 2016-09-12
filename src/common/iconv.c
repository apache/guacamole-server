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

#include "config.h"
#include "common/iconv.h"

#include <guacamole/unicode.h>
#include <stdint.h>

/**
 * Lookup table for Unicode code points, indexed by CP-1252 codepoint.
 */
const static int __GUAC_RDP_CP1252_CODEPOINT[32] = {
    0x20AC, /* 0x80 */
    0xFFFD, /* 0x81 */
    0x201A, /* 0x82 */
    0x0192, /* 0x83 */
    0x201E, /* 0x84 */
    0x2026, /* 0x85 */
    0x2020, /* 0x86 */
    0x2021, /* 0x87 */
    0x02C6, /* 0x88 */
    0x2030, /* 0x89 */
    0x0160, /* 0x8A */
    0x2039, /* 0x8B */
    0x0152, /* 0x8C */
    0xFFFD, /* 0x8D */
    0x017D, /* 0x8E */
    0xFFFD, /* 0x8F */
    0xFFFD, /* 0x90 */
    0x2018, /* 0x91 */
    0x2019, /* 0x92 */
    0x201C, /* 0x93 */
    0x201D, /* 0x94 */
    0x2022, /* 0x95 */
    0x2013, /* 0x96 */
    0x2014, /* 0x97 */
    0x02DC, /* 0x98 */
    0x2122, /* 0x99 */
    0x0161, /* 0x9A */
    0x203A, /* 0x9B */
    0x0153, /* 0x9C */
    0xFFFD, /* 0x9D */
    0x017E, /* 0x9E */
    0x0178, /* 0x9F */
};

int guac_iconv(guac_iconv_read* reader, const char** input, int in_remaining,
               guac_iconv_write* writer, char** output, int out_remaining) {

    while (in_remaining > 0 && out_remaining > 0) {

        int value;
        const char* read_start;
        char* write_start;

        /* Read character */
        read_start = *input;
        value = reader(input, in_remaining);
        in_remaining -= *input - read_start;

        /* Write character */
        write_start = *output;
        writer(output, out_remaining, value);
        out_remaining -= *output - write_start;

        /* Stop if null terminator reached */
        if (value == 0)
            return 1;

    }

    /* Null terminator not reached */
    return 0;

}

int GUAC_READ_UTF8(const char** input, int remaining) {

    int value;

    *input += guac_utf8_read(*input, remaining, &value);
    return value;

}

int GUAC_READ_UTF16(const char** input, int remaining) {

    int value;

    /* Bail if not enough data */
    if (remaining < 2)
        return 0;

    /* Read two bytes as integer */
    value = *((uint16_t*) *input);
    *input += 2;

    return value;

}

int GUAC_READ_CP1252(const char** input, int remaining) {

    int value = *((unsigned char*) *input);

    /* Replace value with exception if not identical to ISO-8859-1 */
    if (value >= 0x80 && value <= 0x9F)
        value = __GUAC_RDP_CP1252_CODEPOINT[value - 0x80];

    (*input)++;
    return value;

}

int GUAC_READ_ISO8859_1(const char** input, int remaining) {

    int value = *((unsigned char*) *input);

    (*input)++;
    return value;

}

void GUAC_WRITE_UTF8(char** output, int remaining, int value) {
    *output += guac_utf8_write(value, *output, remaining);
}

void GUAC_WRITE_UTF16(char** output, int remaining, int value) {

    /* Bail if not enough data */
    if (remaining < 2)
        return;

    /* Write two bytes as integer */
    *((uint16_t*) *output) = value;
    *output += 2;

}

void GUAC_WRITE_CP1252(char** output, int remaining, int value) {

    /* If not in ISO-8859-1 part of CP1252, check lookup table */
    if ((value >= 0x80 && value <= 0x9F) || value > 0xFF) {

        int i;
        int replacement_value = '?';
        const int* codepoint = __GUAC_RDP_CP1252_CODEPOINT;

        /* Search lookup table for value */
        for (i=0x80; i<=0x9F; i++, codepoint++) {
            if (*codepoint == value) {
                replacement_value = i;
                break;
            }
        }

        /* Replace value with discovered value (or question mark) */
        value = replacement_value;

    }

    *((unsigned char*) *output) = (unsigned char) value;
    (*output)++;
}

void GUAC_WRITE_ISO8859_1(char** output, int remaining, int value) {

    /* Translate to question mark if out of range */
    if (value > 0xFF)
        value = '?';

    *((unsigned char*) *output) = (unsigned char) value;
    (*output)++;
}

