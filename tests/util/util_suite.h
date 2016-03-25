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

