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

#include "aad.h"

#include <CUnit/CUnit.h>
#include <guacamole/mem.h>

/**
 * Test which verifies percent-encoded strings are correctly decoded, that
 * invalid or truncated escape sequences are passed through literally, and that
 * NULL input yields NULL. Only exercised when Azure AD support is compiled in.
 */
void test_aad__percent_decode() {

#ifdef HAVE_FREERDP_AAD_SUPPORT

    char* result;

    /* Valid escapes are decoded */
    result = guac_rdp_percent_decode("a%20b%2Fc");
    CU_ASSERT_PTR_NOT_NULL_FATAL(result);
    CU_ASSERT_STRING_EQUAL(result, "a b/c");
    guac_mem_free(result);

    /* Strings without escapes are returned unchanged */
    result = guac_rdp_percent_decode("plain-value");
    CU_ASSERT_PTR_NOT_NULL_FATAL(result);
    CU_ASSERT_STRING_EQUAL(result, "plain-value");
    guac_mem_free(result);

    /* A trailing, truncated escape is left literal */
    result = guac_rdp_percent_decode("100%");
    CU_ASSERT_PTR_NOT_NULL_FATAL(result);
    CU_ASSERT_STRING_EQUAL(result, "100%");
    guac_mem_free(result);

    /* A non-hex escape is left literal */
    result = guac_rdp_percent_decode("%zz");
    CU_ASSERT_PTR_NOT_NULL_FATAL(result);
    CU_ASSERT_STRING_EQUAL(result, "%zz");
    guac_mem_free(result);

    /* NULL input yields NULL */
    CU_ASSERT_PTR_NULL(guac_rdp_percent_decode(NULL));

#else
    CU_PASS("Azure AD support not enabled; nothing to test.");
#endif

}
