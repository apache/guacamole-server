
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

#include "socket.h"
#include "protocol.h"
#include "error.h"

#define UTF8_DOG "\xe7\x8a\xac"

int init_suite() {
    return 0;
}

int cleanup_suite() {
    return 0;
}

void test_instruction_parse() {

    int rfd, wfd;
    int fd[2], childpid;

    char test_string[] = "4.test,3.a" UTF8_DOG "b,"
                         "5.12345,4.a" UTF8_DOG UTF8_DOG "c;"
                         "5.test2,10.hellohello,15.worldworldworld;";

    /* Create pipe */
    pipe(fd);

    /* File descriptors */
    rfd = fd[0];
    wfd = fd[1];

    /* Fork */
    if ((childpid = fork()) == -1) {
        /* ERROR */
        perror("fork");
        return;
    }

    /* Child (pipe writer) */
    if (childpid != 0) {
        close(rfd);
        write(wfd, test_string, sizeof(test_string));
        exit(0);
    }

    /* Parent (unit test) */
    else {

        guac_socket* socket;
        guac_instruction* instruction;

        close(wfd);

        /* Open guac socket */
        socket = guac_socket_open(rfd);
        CU_ASSERT_PTR_NOT_NULL_FATAL(socket);

        /* Read instruction */
        instruction = guac_protocol_read_instruction(socket, 1000000);
        CU_ASSERT_PTR_NOT_NULL_FATAL(instruction);
        
        /* Validate contents */
        CU_ASSERT_STRING_EQUAL(instruction->opcode, "test");
        CU_ASSERT_EQUAL_FATAL(instruction->argc, 3);
        CU_ASSERT_STRING_EQUAL(instruction->argv[0], "a" UTF8_DOG "b");
        CU_ASSERT_STRING_EQUAL(instruction->argv[1], "12345");
        CU_ASSERT_STRING_EQUAL(instruction->argv[2], "a" UTF8_DOG UTF8_DOG "c");
        
        /* Read another instruction */
        guac_instruction_free(instruction);
        instruction = guac_protocol_read_instruction(socket, 1000000);

        /* Validate contents */
        CU_ASSERT_STRING_EQUAL(instruction->opcode, "test2");
        CU_ASSERT_EQUAL_FATAL(instruction->argc, 2);
        CU_ASSERT_STRING_EQUAL(instruction->argv[0], "hellohello");
        CU_ASSERT_STRING_EQUAL(instruction->argv[1], "worldworldworld");

        guac_instruction_free(instruction);
        guac_socket_close(socket);

    }
 
}

int main() {

    CU_pSuite suite;

    /* Init registry */
    if (CU_initialize_registry() != CUE_SUCCESS)
        return CU_get_error();

    /* Add protocol test suite */
    suite = CU_add_suite("protocol", init_suite, cleanup_suite);
    if (suite == NULL) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* Add tests */
    if (
        CU_add_test(suite, "instruction-parse", test_instruction_parse) == NULL
       ) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* Run tests */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();

}

