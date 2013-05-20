
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
 * Similar to write, but automatically retries the write operation until
 * an error occurs.
 */
static int __write_all(int fd, const char* buffer, int size) {

    int remaining = size;
    while (remaining > 0) {

        /* Attempt to write data */
        int ret_val = write(fd, buffer, remaining);
        if (ret_val <= 0)
            return -1;

        /* If successful, contine with what data remains (if any) */
        remaining -= ret_val;
        buffer += ret_val;

    }

    return size;

}

/**
 * Reads a single line from STDIN.
 */
static char* prompt(guac_client* client, const char* title, char* str, int size, bool echo) {

    ssh_guac_client_data* client_data = (ssh_guac_client_data*) client->data;

    int pos;
    char in_byte;

    /* Get STDIN and STDOUT */
    int stdin_fd  = client_data->stdin_pipe_fd[0];
    int stdout_fd = client_data->stdout_pipe_fd[1];

    /* Print title */
    __write_all(stdout_fd, title, strlen(title));

    /* Make room for null terminator */
    size--;

    /* Read bytes until newline */
    pos = 0;
    while (pos < size && read(stdin_fd, &in_byte, 1) == 1) {

        /* Backspace */
        if (in_byte == 0x08) {

            if (pos > 0) {
                __write_all(stdout_fd, "\b \b", 3);
                pos--;
            }
        }

        /* Newline (end of input */
        else if (in_byte == 0x0A) {
            __write_all(stdout_fd, "\r\n", 2);
            break;
        }

        else {

            /* Store character, update buffers */
            str[pos++] = in_byte;

            /* Print character if echoing */
            if (echo)
                __write_all(stdout_fd, &in_byte, 1);
            else
                __write_all(stdout_fd, "*", 1);

        }

    }

    str[pos] = 0;
    return str;

}

void* ssh_client_thread(void* data) {

    guac_client* client = (guac_client*) data;

    char username[1024];
    char password[1024];

    /* Get username */
    if (prompt(client, "Login as: ", username, sizeof(username), true) == NULL)
        return NULL;

    /* Get password */
    if (prompt(client, "Password: ", password, sizeof(password), false) == NULL)
        return NULL;

    guac_client_log_info((guac_client*) data, "got: %s ... %s", username, password);

    return NULL;

}

