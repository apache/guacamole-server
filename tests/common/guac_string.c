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

#include "config.h"

#include "common_suite.h"
#include "guac_string.h"

#include <stdlib.h>
#include <CUnit/Basic.h>

void test_guac_string() {

    char** tokens;

    /* Test occurrence counting */
    CU_ASSERT_EQUAL(4, guac_count_occurrences("this is a test string", 's'));
    CU_ASSERT_EQUAL(3, guac_count_occurrences("this is a test string", 'i'));
    CU_ASSERT_EQUAL(0, guac_count_occurrences("", 's'));

    /* Split test string */
    tokens = guac_split("this is a test string", ' ');

    CU_ASSERT_PTR_NOT_NULL(tokens);

    /* Check resulting tokens */
    CU_ASSERT_PTR_NOT_NULL_FATAL(tokens[0]);
    CU_ASSERT_STRING_EQUAL("this", tokens[0]);

    CU_ASSERT_PTR_NOT_NULL_FATAL(tokens[1]);
    CU_ASSERT_STRING_EQUAL("is", tokens[1]);

    CU_ASSERT_PTR_NOT_NULL_FATAL(tokens[2]);
    CU_ASSERT_STRING_EQUAL("a", tokens[2]);

    CU_ASSERT_PTR_NOT_NULL_FATAL(tokens[3]);
    CU_ASSERT_STRING_EQUAL("test", tokens[3]);

    CU_ASSERT_PTR_NOT_NULL_FATAL(tokens[4]);
    CU_ASSERT_STRING_EQUAL("string", tokens[4]);

    CU_ASSERT_PTR_NULL(tokens[5]);


    /* Clean up */
    free(tokens[0]);
    free(tokens[1]);
    free(tokens[2]);
    free(tokens[3]);
    free(tokens[4]);
    free(tokens);

}

