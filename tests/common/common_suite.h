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


#ifndef _GUAC_TEST_COMMON_SUITE_H
#define _GUAC_TEST_COMMON_SUITE_H

/**
 * Test suite containing unit tests for the "common" utility library included
 * for the sake of simplifying guacamole-server development, but not included
 * as part of libguac.
 *
 * @file common_suite.h
 */

#include "config.h"

/**
 * Registers the common test suite with CUnit.
 */
int register_common_suite();

/**
 * Unit test for string utility functions.
 */
void test_guac_string();

/**
 * Unit test for character conversion functions.
 */
void test_guac_iconv();

/**
 * Unit test for rectangle calculation functions.
 */
void test_guac_rect();

#endif

