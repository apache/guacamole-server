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

#include "common/iconv.h"

#include <CUnit/CUnit.h>

/**
 * UTF8 for "papà è bello".
 */
unsigned char test_string_utf8[] = {
    'p',  'a',  'p', 0xC3, 0xA0, ' ',
    0xC3, 0xA8, ' ',
    'b',  'e',  'l', 'l',  'o',
    0x00
};

/**
 * UTF16 for "papà è bello".
 */
unsigned char test_string_utf16[] = {
    'p',  0x00, 'a', 0x00, 'p', 0x00, 0xE0, 0x00, ' ', 0x00,
    0xE8, 0x00, ' ', 0x00,
    'b',  0x00, 'e', 0x00, 'l', 0x00, 'l',  0x00, 'o', 0x00,
    0x00, 0x00
};

/**
 * ISO-8859-1 for "papà è bello".
 */
unsigned char test_string_iso8859_1[] = {
    'p',  'a',  'p', 0xE0, ' ',
    0xE8, ' ',
    'b',  'e',  'l', 'l',  'o',
    0x00
};

/**
 * CP1252 for "papà è bello".
 */
unsigned char test_string_cp1252[] = {
    'p',  'a',  'p', 0xE0, ' ',
    0xE8, ' ',
    'b',  'e',  'l', 'l',  'o',
    0x00
};

/**
 * Tests that conversion between character sets using the given guac_iconv_read
 * and guac_iconv_write implementations matches expectations.
 *
 * @param reader
 *     The guac_iconv_read implementation to use to read the input string.
 *
 * @param in_string
 *     A pointer to the beginning of the input string.
 *
 * @param in_length
 *     The size of the input string in bytes.
 *
 * @param writer
 *     The guac_iconv_write implementation to use to write the output string
 *     (the converted input string).
 *
 * @param out_string
 *     A pointer to the beginning of a string which contains the expected
 *     result of the conversion.
 *
 * @param out_length
 *     The size of the expected result in bytes.
 */
static void verify_conversion(
        guac_iconv_read* reader,  unsigned char* in_string,  int in_length,
        guac_iconv_write* writer, unsigned char* out_string, int out_length) {

    char output[4096];
    char input[4096];

    const char* current_input = input;
    char* current_output = output;

    memcpy(input, in_string, in_length);
    guac_iconv(reader, &current_input,  sizeof(input),
               writer, &current_output, sizeof(output));

    /* Verify output length */
    CU_ASSERT_EQUAL(out_length, current_output - output);

    /* Verify entire input read */
    CU_ASSERT_EQUAL(in_length, current_input - input);

    /* Verify output content */
    CU_ASSERT_EQUAL(0, memcmp(output, out_string, out_length));

}

/**
 * Tests which verifies conversion of UTF-8 to itself.
 */
void test_iconv__utf8_to_utf8() {
    verify_conversion(
            GUAC_READ_UTF8,  test_string_utf8, sizeof(test_string_utf8),
            GUAC_WRITE_UTF8, test_string_utf8, sizeof(test_string_utf8));
}

/**
 * Tests which verifies conversion of UTF-16 to UTF-8.
 */
void test_iconv__utf8_to_utf16() {
    verify_conversion(
            GUAC_READ_UTF8,   test_string_utf8,  sizeof(test_string_utf8),
            GUAC_WRITE_UTF16, test_string_utf16, sizeof(test_string_utf16));
}

/**
 * Tests which verifies conversion of UTF-16 to itself.
 */
void test_iconv__utf16_to_utf16() {
    verify_conversion(
            GUAC_READ_UTF16,  test_string_utf16, sizeof(test_string_utf16),
            GUAC_WRITE_UTF16, test_string_utf16, sizeof(test_string_utf16));
}

/**
 * Tests which verifies conversion of UTF-8 to UTF-16.
 */
void test_iconv__utf16_to_utf8() {
    verify_conversion(
            GUAC_READ_UTF16, test_string_utf16, sizeof(test_string_utf16),
            GUAC_WRITE_UTF8, test_string_utf8,  sizeof(test_string_utf8));
}

/**
 * Tests which verifies conversion of UTF-16 to ISO 8859-1.
 */
void test_iconv__utf16_to_iso8859_1() {
    verify_conversion(
            GUAC_READ_UTF16,      test_string_utf16,      sizeof(test_string_utf16),
            GUAC_WRITE_ISO8859_1, test_string_iso8859_1,  sizeof(test_string_iso8859_1));
}

/**
 * Tests which verifies conversion of UTF-16 to CP1252.
 */
void test_iconv__utf16_to_cp1252() {
    verify_conversion(
            GUAC_READ_UTF16,   test_string_utf16,  sizeof(test_string_utf16),
            GUAC_WRITE_CP1252, test_string_cp1252, sizeof(test_string_cp1252));
}

/**
 * Tests which verifies conversion of CP1252 to UTF-8.
 */
void test_iconv__cp1252_to_utf8() {
    verify_conversion(
            GUAC_READ_CP1252, test_string_cp1252, sizeof(test_string_cp1252),
            GUAC_WRITE_UTF8,  test_string_utf8,   sizeof(test_string_utf8));
}

/**
 * Tests which verifies conversion of ISO 8859-1 to UTF-8.
 */
void test_iconv__iso8859_1_to_utf8() {
    verify_conversion(
            GUAC_READ_ISO8859_1, test_string_iso8859_1, sizeof(test_string_iso8859_1),
            GUAC_WRITE_UTF8,     test_string_utf8,      sizeof(test_string_utf8));

}

