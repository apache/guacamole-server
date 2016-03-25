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

#include "util_suite.h"

#include <CUnit/Basic.h>

int util_suite_init() {
    return 0;
}

int util_suite_cleanup() {
    return 0;
}

int register_util_suite() {

    /* Add util test suite */
    CU_pSuite suite = CU_add_suite("util",
            util_suite_init, util_suite_cleanup);
    if (suite == NULL) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* Add tests */
    if (
           CU_add_test(suite, "guac-pool",    test_guac_pool)    == NULL
        || CU_add_test(suite, "guac-unicode", test_guac_unicode) == NULL
       ) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    return 0;

}

