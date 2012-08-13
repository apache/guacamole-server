
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

#include "suite.h"

void test_instruction_write() {

    int rfd, wfd;
    int fd[2], childpid;

    /* Create pipe */
    CU_ASSERT_EQUAL_FATAL(pipe(fd), 0);

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

        guac_socket* socket;

        close(rfd);

        /* Open guac socket */
        socket = guac_socket_open(wfd);

        /* Write instruction */
        guac_protocol_send_clipboard(socket, "a" UTF8_4 "b" UTF8_4 "c");
        guac_protocol_send_sync(socket, 12345);
        guac_socket_flush(socket);

        guac_socket_close(socket);
        exit(0);
    }

    /* Parent (unit test) */
    else {

        char expected[] =
            "9.clipboard,11.a" UTF8_4 "b" UTF8_4 "c;"
            "4.sync,5.12345;";

        int numread;
        char buffer[1024];
        int offset = 0;

        close(wfd);

        /* Read everything available into buffer */
        while ((numread =
                    read(rfd,
                        &(buffer[offset]),
                        sizeof(buffer)-offset)) != 0) {
            offset += numread;
        }

        /* Add NULL terminator */
        buffer[offset] = '\0';

        /* Read value should be equal to expected value */
        CU_ASSERT_STRING_EQUAL(buffer, expected);

    }
 
}

