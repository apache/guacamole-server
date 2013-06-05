
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
 * David PHAM-VAN <d.pham-van@ulteo.com> Ulteo SAS - http://www.ulteo.com
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
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#ifdef __MINGW32__
#include <winsock2.h>
#else
#include <sys/select.h>
#endif

#include <time.h>
#include <sys/time.h>

#include "socket.h"
#include "protocol.h"
#include "error.h"
#include "unicode.h"

#define GUAC_SOCKET_NEST_BUFFER_SIZE 8192

typedef struct __guac_socket_nest_data {

    guac_socket* parent;
    char buffer[GUAC_SOCKET_NEST_BUFFER_SIZE];
    int index;

} __guac_socket_nest_data;

ssize_t __guac_socket_nest_write_handler(guac_socket* socket,
        void* buf, size_t count) {

    __guac_socket_nest_data* data = (__guac_socket_nest_data*) socket->data;
    unsigned char* source = (unsigned char*) buf;

    /* Current location in destination buffer during copy */
    char* current = data->buffer;

    /* Number of bytes remaining in source buffer */
    int remaining = count;

    /* If we can't actually store that many bytes, reduce number of bytes
     * expected to be written */
    if (remaining > GUAC_SOCKET_NEST_BUFFER_SIZE)
        remaining = GUAC_SOCKET_NEST_BUFFER_SIZE;

    /* Current offset within destination buffer */
    int offset;

    /* Number of characters before start of next character */
    int skip = 0;

    /* Copy UTF-8 characters into buffer */
    for (offset = 0; offset < GUAC_SOCKET_NEST_BUFFER_SIZE; offset++) {

        /* Get next byte */
        unsigned char c = *source;
        remaining--;

        /* If skipping, then skip */
        if (skip > 0) skip--;

        /* Otherwise, determine next skip value, and increment length */
        else {

            /* Determine skip value (size in bytes of rest of character) */
            skip = guac_utf8_charsize(c) - 1;

            /* If not enough bytes to complete character, break */
            if (skip > remaining)
                break;

        }

        /* Store byte */
        *current = c;

        /* Advance to next character */
        source++;
        current++;

    }

    /* Append null-terminator */
    *current = 0;

    /* Send nest instruction containing read UTF-8 segment */
    guac_protocol_send_nest(data->parent, data->index, data->buffer);

    /* Return number of bytes actually written */
    return offset;

}

guac_socket* guac_socket_nest(guac_socket* parent, int index) {

    /* Allocate socket and associated data */
    guac_socket* socket = guac_socket_alloc();
    __guac_socket_nest_data* data = malloc(sizeof(__guac_socket_nest_data));

    /* Store file descriptor as socket data */
    data->parent = parent;
    socket->data = data;

    /* Set write handler */
    socket->write_handler  = __guac_socket_nest_write_handler;

    return socket;

}

