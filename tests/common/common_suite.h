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

