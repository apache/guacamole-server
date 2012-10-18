
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

#include <stdlib.h>
#include <stdio.h>

#include "client.h"
#include "protocol.h"
#include "client-handlers.h"

/* Guacamole instruction handler map */

__guac_instruction_handler_mapping __guac_instruction_handler_map[] = {
   {"sync",       __guac_handle_sync},
   {"mouse",      __guac_handle_mouse},
   {"key",        __guac_handle_key},
   {"clipboard",  __guac_handle_clipboard},
   {"disconnect", __guac_handle_disconnect},
   {NULL,         NULL}
};

int64_t __guac_parse_int(const char* str) {

    int sign = 1;
    int64_t num = 0;

    for (; *str != '\0'; str++) {

        if (*str == '-')
            sign = -sign;
        else
            num = num * 10 + (*str - '0');

    }

    return num * sign;

}


/* Guacamole instruction handlers */

int __guac_handle_sync(guac_client* client, guac_instruction* instruction) {
    guac_timestamp timestamp = __guac_parse_int(instruction->argv[0]);

    /* Error if timestamp is in future */
    if (timestamp > client->last_sent_timestamp)
        return -1;

    client->last_received_timestamp = timestamp;
    return 0;
}

int __guac_handle_mouse(guac_client* client, guac_instruction* instruction) {
    if (client->mouse_handler)
        return client->mouse_handler(
            client,
            atoi(instruction->argv[0]), /* x */
            atoi(instruction->argv[1]), /* y */
            atoi(instruction->argv[2])  /* mask */
        );
    return 0;
}

int __guac_handle_key(guac_client* client, guac_instruction* instruction) {
    if (client->key_handler)
        return client->key_handler(
            client,
            atoi(instruction->argv[0]), /* keysym */
            atoi(instruction->argv[1])  /* pressed */
        );
    return 0;
}

int __guac_handle_clipboard(guac_client* client, guac_instruction* instruction) {
    if (client->clipboard_handler)
        return client->clipboard_handler(
            client,
            instruction->argv[0] /* data */
        );
    return 0;
}

int __guac_handle_disconnect(guac_client* client, guac_instruction* instruction) {
    /* Return error code to force disconnect */
    return -1;
}

