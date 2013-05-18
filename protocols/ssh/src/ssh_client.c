
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
 * The Original Code is libguac-client-ssh.
 *
 * The Initial Developer of the Original Code is
 * Michael Jumper.
 * Portions created by the Initial Developer are Copyright (C) 2011
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * James Muehlner <dagger10k@users.sourceforge.net>
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

#include <stdio.h>
#include <string.h>

#include <guacamole/client.h>

#include "client.h"

/**
 * Similar to fgets(), reads a single line from STDIN. Unlike fgets(), this
 * function does not include the trailing newline character, although the
 * character is removed from the input stream.
 *
 * @param title The title of the prompt to display.
 * @param str The buffer to read the result into.
 * @param size The number of bytes available in the buffer.
 * @return str, or NULL if the prompt failed.
 */
static char* prompt(const char* title, char* str, int size) {

    /* Print title */
    printf("%s", title);
    fflush(stdout);

    /* Read input */
    str = fgets(str, size, stdin);

    /* Remove trailing newline, if any */
    if (str != NULL) {
        int length = strlen(str);
        if (str[length-1] == '\n')
            str[length-1] = 0;
    }

    return str;

}

void* ssh_client_thread(void* data) {

    char username[1024];
    char password[1024];

    /* Get username */
    if (prompt("Login as: ", username, sizeof(username)) == NULL)
        return NULL;

    /* Get password */
    if (prompt("Password: ", password, sizeof(password)) == NULL)
        return NULL;

    guac_client_log_info((guac_client*) data, "got: %s ... %s", username, password);

    return NULL;

}

