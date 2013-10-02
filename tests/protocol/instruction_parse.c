
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is libguac.
 *
 * The Initial Developer of the Original Code is
 * Michael Jumper.
 * Portions created by the Initial Developer are Copyright (C) 2010
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <CUnit/Basic.h>

#include <guacamole/instruction.h>

#include "suite.h"


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

