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
#include "convert-test-data.h"

#include <CUnit/CUnit.h>
#include <guacamole/mem.h>
#include <stdio.h>

/**
 * Tests that conversion between character sets using the given guac_iconv_read
 * and guac_iconv_write implementations matches expectations.
 *
 * @param reader
 *     The guac_iconv_read implementation to use to read the input string.
 *
 * @param in_string
 *     A pointer to the test_string structure describing the input string being
 *     tested.
 *
 * @param writer
 *     The guac_iconv_write implementation to use to write the output string
 *     (the converted input string).
 *
 * @param out_string
 *     A pointer to the test_string structure describing the expected result of
 *     the conversion.
 */
static void verify_conversion(
        guac_iconv_read* reader,  test_string* in_string,
        guac_iconv_write* writer, test_string* out_string) {

    char* input = guac_mem_alloc(in_string->size);
    char* output = guac_mem_alloc(out_string->size);

    const char* current_input = input;
    char* current_output = output;

    CU_ASSERT_PTR_NOT_NULL_FATAL(input);
    CU_ASSERT_PTR_NOT_NULL_FATAL(output);

    memcpy(input, in_string->buffer, in_string->size);
    guac_iconv(reader, &current_input, in_string->size,
               writer, &current_output, out_string->size);

    /* Verify output length */
    CU_ASSERT_EQUAL(out_string->size, current_output - output);

    /* Verify entire input read */
    CU_ASSERT_EQUAL(in_string->size, current_input - input);

    /* Verify output content */
    CU_ASSERT_EQUAL(0, memcmp(output, out_string->buffer, out_string->size));

    guac_mem_free(output);
    guac_mem_free(input);

}

/**
 * Test which verifies that every supported encoding can be correctly converted
 * to every other supported encoding, with all line endings preserved verbatim
 * (not normalized).
 */
void test_iconv__preserve(void) {
    for (int i = 0; i < NUM_SUPPORTED_ENCODINGS; i++) {
        for (int j = 0; j < NUM_SUPPORTED_ENCODINGS; j++) {

            encoding_test_parameters* from = &test_params[i];
            encoding_test_parameters* to = &test_params[j];

            printf("# \"%s\" -> \"%s\" ...\n", from->name, to->name);
            verify_conversion(from->reader, &from->test_mixed,
                    to->writer, &to->test_mixed);

        }
    }
}

/**
 * Test which verifies that every supported encoding can be correctly converted
 * to every other supported encoding, normalizing all line endings to
 * Unix-style line endings.
 */
void test_iconv__normalize_unix(void) {
    for (int i = 0; i < NUM_SUPPORTED_ENCODINGS; i++) {
        for (int j = 0; j < NUM_SUPPORTED_ENCODINGS; j++) {

            encoding_test_parameters* from = &test_params[i];
            encoding_test_parameters* to = &test_params[j];

            printf("# \"%s\" -> \"%s\" ...\n", from->name, to->name);
            verify_conversion(from->reader_normalized, &from->test_mixed,
                    to->writer, &to->test_unix);

        }
    }
}

/**
 * Test which verifies that every supported encoding can be correctly converted
 * to every other supported encoding, normalizing all line endings to
 * Windows-style line endings.
 */
void test_iconv__normalize_crlf(void) {
    for (int i = 0; i < NUM_SUPPORTED_ENCODINGS; i++) {
        for (int j = 0; j < NUM_SUPPORTED_ENCODINGS; j++) {

            encoding_test_parameters* from = &test_params[i];
            encoding_test_parameters* to = &test_params[j];

            printf("# \"%s\" -> \"%s\" ...\n", from->name, to->name);
            verify_conversion(from->reader_normalized, &from->test_mixed,
                    to->writer_crlf, &to->test_windows);

        }
    }
}

/**
 * Verifies that MacRoman encoding and decoding are symmetrical for every byte
 * value.
 */
void test_iconv__macroman_encode_decode_symmetrical() {
    /* Verify symmetry for each value in the MacRoman encoding range, i.e. the
     * lookup value matches the reverse lookup value. */
    for (int i = 0x00; i <= 0xFF; i++) {

        /* Build a one-byte encoded input buffer, then view it through the
         * reader's const char* interface. */
        unsigned char input[] = { i };
        const char* current_input = (const char*) input;

        /* Decode one MacRoman byte to its Unicode codepoint. */
        int codepoint = GUAC_READ_MACROMAN(&current_input, sizeof(input));

        char output[4];
        char* current_output = output;

        /* Re-encode that codepoint and verify the original byte is restored. */
        GUAC_WRITE_MACROMAN(&current_output, sizeof(output), codepoint);

        CU_ASSERT_EQUAL(1, current_input - (const char*) input);
        CU_ASSERT_EQUAL(1, current_output - output);
        CU_ASSERT_EQUAL((unsigned char) i, (unsigned char) output[0]);

    }
}

/**
 * Verifies that codepoints outside the valid Unicode range written to
 * CP1252 degrade to '?' instead of being truncated.
 */
void test_iconv__cp1252_invalid_codepoint() {
    /* Exercise several clearly invalid Unicode codepoints. */
    const int invalid_codepoints[] = { -1, 0x110000, 0x123456 };

    for (int i = 0; i < (int)ARRAY_SIZE(invalid_codepoints); i++) {

        char output[4];
        char* current_output = output;
        int actual_value;

        /* Invalid codepoints must fall back to '?' rather than truncating. */
        GUAC_WRITE_CP1252(&current_output, sizeof(output), invalid_codepoints[i]);
        actual_value = (unsigned char) output[0];

        CU_ASSERT_EQUAL(1, current_output - output);
        CU_ASSERT_EQUAL((unsigned char) '?', actual_value);

    }
}

/**
 * Verifies that GUAC_READ_MACROMAN_NORMALIZED normalizes both bare CR (\r)
 * and CRLF (\r\n) sequences to Unix newlines (\n).
 */
void test_iconv__normalize_cr() {

    /* Input contains a bare CR, a CRLF pair, and a plain LF */
    const unsigned char input_buf[] = "line1\rline2\r\nline3\n";
    const unsigned char expected[]  = "line1\nline2\nline3\n";

    char* input = guac_mem_alloc(sizeof(input_buf));
    char* output = guac_mem_alloc(sizeof(expected));

    CU_ASSERT_PTR_NOT_NULL_FATAL(input);
    CU_ASSERT_PTR_NOT_NULL_FATAL(output);

    memcpy(input, input_buf, sizeof(input_buf));

    const char* current_input = input;
    char* current_output = output;

    guac_iconv(GUAC_READ_MACROMAN_NORMALIZED, &current_input, sizeof(input_buf),
               GUAC_WRITE_UTF8,              &current_output, sizeof(expected));

    CU_ASSERT_EQUAL((int) sizeof(expected), current_output - output);
    CU_ASSERT_EQUAL(0, memcmp(output, expected, sizeof(expected)));

    guac_mem_free(output);
    guac_mem_free(input);

}

/**
 * Verifies that codepoints outside the valid Unicode range written to
 * MacRoman degrade to '?' instead of being truncated.
 */
void test_iconv__macroman_invalid_codepoint() {
    /* Exercise several clearly invalid Unicode codepoints. */
    const int invalid_codepoints[] = { -1, 0x110000, 0x123456 };

    for (int i = 0; i < (int) ARRAY_SIZE(invalid_codepoints); i++) {

        char output[4];
        char* current_output = output;
        int actual_value;

        /* Invalid codepoints must fall back to '?' rather than truncating. */
        GUAC_WRITE_MACROMAN(&current_output, sizeof(output), invalid_codepoints[i]);
        actual_value = (unsigned char) output[0];

        CU_ASSERT_EQUAL(1, current_output - output);
        CU_ASSERT_EQUAL((unsigned char) '?', actual_value);

    }
}
