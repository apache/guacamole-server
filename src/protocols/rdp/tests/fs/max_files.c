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

#include "fs.h"

#include <CUnit/CUnit.h>
#include <stdio.h>

/**
 * Test which verifies supported GUAC_RDP_FS_MAX_FILES values are accepted.
 */
void test_fs__returns_parsed_limit_when_value_is_supported(void) {

    /* given */
    char maximum[32];
    snprintf(maximum, sizeof(maximum), "%i", GUAC_RDP_FS_MAX_FILES_LIMIT);

    /* when / then */
    CU_ASSERT_EQUAL(guac_rdp_fs_parse_max_files("1"), 1);
    CU_ASSERT_EQUAL(guac_rdp_fs_parse_max_files("128"), 128);
    CU_ASSERT_EQUAL(guac_rdp_fs_parse_max_files("1024"), 1024);
    CU_ASSERT_EQUAL(guac_rdp_fs_parse_max_files(maximum),
            GUAC_RDP_FS_MAX_FILES_LIMIT);

}

/**
 * Test which verifies malformed GUAC_RDP_FS_MAX_FILES values are rejected.
 */
void test_fs__returns_error_when_value_is_malformed(void) {

    /* given */
    char above_maximum[32];
    snprintf(above_maximum, sizeof(above_maximum), "%i",
            GUAC_RDP_FS_MAX_FILES_LIMIT + 1);

    /* when / then */
    CU_ASSERT_EQUAL(guac_rdp_fs_parse_max_files(NULL), -1);
    CU_ASSERT_EQUAL(guac_rdp_fs_parse_max_files(""), -1);
    CU_ASSERT_EQUAL(guac_rdp_fs_parse_max_files("0"), -1);
    CU_ASSERT_EQUAL(guac_rdp_fs_parse_max_files("-1"), -1);
    CU_ASSERT_EQUAL(guac_rdp_fs_parse_max_files("+1"), -1);
    CU_ASSERT_EQUAL(guac_rdp_fs_parse_max_files(" 1"), -1);
    CU_ASSERT_EQUAL(guac_rdp_fs_parse_max_files("1 "), -1);
    CU_ASSERT_EQUAL(guac_rdp_fs_parse_max_files("1.5"), -1);
    CU_ASSERT_EQUAL(guac_rdp_fs_parse_max_files("1024files"), -1);
    CU_ASSERT_EQUAL(guac_rdp_fs_parse_max_files(above_maximum), -1);
    CU_ASSERT_EQUAL(guac_rdp_fs_parse_max_files("999999999999999999999999"),
            -1);

}
