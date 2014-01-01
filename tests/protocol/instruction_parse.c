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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <CUnit/Basic.h>
#include <guacamole/instruction.h>

void test_instruction_parse() {

    /* Allocate instruction space */
    guac_instruction* instruction = guac_instruction_alloc();
    CU_ASSERT_PTR_NOT_NULL_FATAL(instruction);

    /* Instruction input */
    char buffer[] = "4.test,8.testdata,5.zxcvb,13.guacamoletest;XXXXXXXXXXXXXXXXXX";
    char* current = buffer;

    /* While data remains */
    int remaining = sizeof(buffer)-1;
    while (remaining > 18) {

        /* Parse more data */
        int parsed = guac_instruction_append(instruction, current, remaining);
        if (parsed == 0)
            break;

        current += parsed;
        remaining -= parsed;

    }

    CU_ASSERT_EQUAL(remaining, 18);
    CU_ASSERT_EQUAL(instruction->state, GUAC_INSTRUCTION_PARSE_COMPLETE);

    /* Parse is complete - no more data should be read */
    CU_ASSERT_EQUAL(guac_instruction_append(instruction, current, 18), 0);
    CU_ASSERT_EQUAL(instruction->state, GUAC_INSTRUCTION_PARSE_COMPLETE);

    /* Validate resulting structure */
    CU_ASSERT_EQUAL(instruction->argc, 3);
    CU_ASSERT_PTR_NOT_NULL_FATAL(instruction->opcode);
    CU_ASSERT_PTR_NOT_NULL_FATAL(instruction->argv[0]);
    CU_ASSERT_PTR_NOT_NULL_FATAL(instruction->argv[1]);
    CU_ASSERT_PTR_NOT_NULL_FATAL(instruction->argv[2]);

    /* Validate resulting content */
    CU_ASSERT_STRING_EQUAL(instruction->opcode,  "test");
    CU_ASSERT_STRING_EQUAL(instruction->argv[0], "testdata");
    CU_ASSERT_STRING_EQUAL(instruction->argv[1], "zxcvb");
    CU_ASSERT_STRING_EQUAL(instruction->argv[2], "guacamoletest");

}

