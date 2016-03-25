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

#include "config.h"

#include "suite.h"

#include <CUnit/Basic.h>

int protocol_suite_init() {
    return 0;
}

int protocol_suite_cleanup() {
    return 0;
}

int register_protocol_suite() {

    /* Add protocol test suite */
    CU_pSuite suite = CU_add_suite("protocol",
            protocol_suite_init, protocol_suite_cleanup);
    if (suite == NULL) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* Add tests */
    if (
        CU_add_test(suite, "base64-decode", test_base64_decode) == NULL
     || CU_add_test(suite, "instruction-parse", test_instruction_parse) == NULL
     || CU_add_test(suite, "instruction-read", test_instruction_read) == NULL
     || CU_add_test(suite, "instruction-write", test_instruction_write) == NULL
     || CU_add_test(suite, "nest-write", test_nest_write) == NULL
       ) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    return 0;

}

