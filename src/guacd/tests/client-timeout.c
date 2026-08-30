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

#include "conf.h"
#include "conf-parse.h"

#include <CUnit/CUnit.h>

#include <stdio.h>

void test_client_timeout__accepts_supported_positive_milliseconds() {

    char maximum[32];
    snprintf(maximum, sizeof(maximum), "%i", GUACD_MAX_CLIENT_TIMEOUT);

    CU_ASSERT_EQUAL(guacd_parse_client_timeout("1"), 1);
    CU_ASSERT_EQUAL(guacd_parse_client_timeout("15000"), 15000);
    CU_ASSERT_EQUAL(guacd_parse_client_timeout("45000"), 45000);
    CU_ASSERT_EQUAL(guacd_parse_client_timeout(maximum),
            GUACD_MAX_CLIENT_TIMEOUT);

}

void test_client_timeout__rejects_unsafe_or_malformed_values() {

    char above_maximum[32];
    snprintf(above_maximum, sizeof(above_maximum), "%li",
            (long) GUACD_MAX_CLIENT_TIMEOUT + 1);

    CU_ASSERT_EQUAL(guacd_parse_client_timeout(""), -1);
    CU_ASSERT_EQUAL(guacd_parse_client_timeout("0"), -1);
    CU_ASSERT_EQUAL(guacd_parse_client_timeout("-1"), -1);
    CU_ASSERT_EQUAL(guacd_parse_client_timeout("+1"), -1);
    CU_ASSERT_EQUAL(guacd_parse_client_timeout(" 1"), -1);
    CU_ASSERT_EQUAL(guacd_parse_client_timeout("1 "), -1);
    CU_ASSERT_EQUAL(guacd_parse_client_timeout("1.5"), -1);
    CU_ASSERT_EQUAL(guacd_parse_client_timeout("45000ms"), -1);
    CU_ASSERT_EQUAL(guacd_parse_client_timeout(above_maximum), -1);
    CU_ASSERT_EQUAL(guacd_parse_client_timeout("999999999999999999999999"),
            -1);

}
