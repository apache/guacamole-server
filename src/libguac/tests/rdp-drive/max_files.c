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

#include <CUnit/CUnit.h>
#include <guacamole/rdp-drive.h>

#include <stdio.h>

/**
 * Test which verifies supported open-file limits are accepted.
 */
void test_rdp_drive__returns_parsed_limit_when_value_is_supported(void) {

    /* given */
    char maximum[32];
    int parsed_one;
    int parsed_default;
    int parsed_raised;
    int parsed_maximum;

    snprintf(maximum, sizeof(maximum), "%i", GUAC_RDP_DRIVE_MAX_FILES_LIMIT);

    /* when */
    parsed_one = guac_rdp_drive_parse_max_files("1");
    parsed_default = guac_rdp_drive_parse_max_files("128");
    parsed_raised = guac_rdp_drive_parse_max_files("1024");
    parsed_maximum = guac_rdp_drive_parse_max_files(maximum);

    /* then */
    CU_ASSERT_EQUAL(parsed_one, 1);
    CU_ASSERT_EQUAL(parsed_default, 128);
    CU_ASSERT_EQUAL(parsed_raised, 1024);
    CU_ASSERT_EQUAL(parsed_maximum, GUAC_RDP_DRIVE_MAX_FILES_LIMIT);

}

/**
 * Test which verifies malformed open-file limits are rejected.
 */
void test_rdp_drive__returns_error_when_value_is_malformed(void) {

    /* given */
    char above_maximum[32];
    int parsed_null;
    int parsed_empty;
    int parsed_zero;
    int parsed_negative;
    int parsed_signed;
    int parsed_leading_space;
    int parsed_trailing_space;
    int parsed_fraction;
    int parsed_suffix;
    int parsed_above_maximum;
    int parsed_overflow;

    snprintf(above_maximum, sizeof(above_maximum), "%i",
            GUAC_RDP_DRIVE_MAX_FILES_LIMIT + 1);

    /* when */
    parsed_null = guac_rdp_drive_parse_max_files(NULL);
    parsed_empty = guac_rdp_drive_parse_max_files("");
    parsed_zero = guac_rdp_drive_parse_max_files("0");
    parsed_negative = guac_rdp_drive_parse_max_files("-1");
    parsed_signed = guac_rdp_drive_parse_max_files("+1");
    parsed_leading_space = guac_rdp_drive_parse_max_files(" 1");
    parsed_trailing_space = guac_rdp_drive_parse_max_files("1 ");
    parsed_fraction = guac_rdp_drive_parse_max_files("1.5");
    parsed_suffix = guac_rdp_drive_parse_max_files("1024files");
    parsed_above_maximum = guac_rdp_drive_parse_max_files(above_maximum);
    parsed_overflow = guac_rdp_drive_parse_max_files("999999999999999999999999");

    /* then */
    CU_ASSERT_EQUAL(parsed_null, -1);
    CU_ASSERT_EQUAL(parsed_empty, -1);
    CU_ASSERT_EQUAL(parsed_zero, -1);
    CU_ASSERT_EQUAL(parsed_negative, -1);
    CU_ASSERT_EQUAL(parsed_signed, -1);
    CU_ASSERT_EQUAL(parsed_leading_space, -1);
    CU_ASSERT_EQUAL(parsed_trailing_space, -1);
    CU_ASSERT_EQUAL(parsed_fraction, -1);
    CU_ASSERT_EQUAL(parsed_suffix, -1);
    CU_ASSERT_EQUAL(parsed_above_maximum, -1);
    CU_ASSERT_EQUAL(parsed_overflow, -1);

}

/**
 * Test which verifies a supported limit is stored and an unsupported limit
 * leaves the stored value unchanged.
 */
void test_rdp_drive__stores_limit_when_value_is_supported(void) {

    /* given */
    int original = guac_rdp_drive_get_max_files();
    int stored;
    int unchanged;

    /* when */
    guac_rdp_drive_set_max_files(1024);
    stored = guac_rdp_drive_get_max_files();
    guac_rdp_drive_set_max_files(0);
    unchanged = guac_rdp_drive_get_max_files();
    guac_rdp_drive_set_max_files(original);

    /* then */
    CU_ASSERT_EQUAL(stored, 1024);
    CU_ASSERT_EQUAL(unchanged, 1024);
    CU_ASSERT_EQUAL(guac_rdp_drive_get_max_files(), original);

}
