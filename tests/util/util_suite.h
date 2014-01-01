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


#ifndef _GUAC_TEST_UTIL_SUITE_H
#define _GUAC_TEST_UTIL_SUITE_H

/**
 * Test suite containing unit tests for utility functions built into libguac.
 * These utility functions are included for convenience rather as integral
 * requirements of the core.
 *
 * @file util_suite.h
 */

#include "config.h"

/**
 * A single Unicode character encoded as one byte with UTF-8.
 */
#define UTF8_1b "g"

/**
 * A single Unicode character encoded as two bytes with UTF-8.
 */
#define UTF8_2b "\xc4\xa3"

/**
 * A single Unicode character encoded as three bytes with UTF-8.
 */
#define UTF8_3b "\xe7\x8a\xac"

/**
 * A single Unicode character encoded as four bytes with UTF-8.
 */
#define UTF8_4b "\xf0\x90\x84\xa3"

/**
 * Registers the utility test suite with CUnit.
 */
int register_util_suite();

/**
 * Unit test for the guac_pool structure and related functions. The guac_pool
 * structure provides a consistent source of pooled integers. This unit test
 * checks that the associated functions behave as documented (returning
 * integers in the proper order, allocating new integers as necessary, etc.).
 */
void test_guac_pool();

/**
 * Unit test for libguac's Unicode convenience functions. This test checks that
 * the functions provided for determining string length, character length, and
 * for reading and writing UTF-8 behave as specified in the documentation.
 */
void test_guac_unicode();

#endif

