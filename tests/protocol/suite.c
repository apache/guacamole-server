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

