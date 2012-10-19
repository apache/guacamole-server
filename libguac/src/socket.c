
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
#include "error.h"

char __guac_socket_BASE64_CHARACTERS[64] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/', 
};

guac_socket* guac_socket_open(int fd) {

    guac_socket* socket = malloc(sizeof(guac_socket));

    /* If no memory available, return with error */
    if (socket == NULL) {
        guac_error = GUAC_STATUS_NO_MEMORY;
        guac_error_message = "Could not allocate memory for socket";
        return NULL;
    }

    socket->__ready = 0;
    socket->__written = 0;
    socket->fd = fd;

    /* Allocate instruction buffer */
    socket->__instructionbuf_size = 1024;
    socket->__instructionbuf = malloc(socket->__instructionbuf_size);

    /* If no memory available, return with error */
    if (socket->__instructionbuf == NULL) {
        guac_error = GUAC_STATUS_NO_MEMORY;
        guac_error_message = "Could not allocate memory for instruction buffer";
        free(socket);
        return NULL;
    }

    /* Init members */
    socket->__instructionbuf_used_length = 0;
    socket->__instructionbuf_parse_start = 0;
    socket->__instructionbuf_elementc = 0;

    return socket;

}

void guac_socket_close(guac_socket* socket) {
    guac_socket_flush(socket);
    free(socket->__instructionbuf);
    free(socket);
}

/* Write bytes, limit rate */
ssize_t __guac_socket_write(guac_socket* socket, const char* buf, int count) {

    int retval;

#ifdef __MINGW32__
    /* MINGW32 WINSOCK only works with send() */
    retval = send(socket->fd, buf, count, 0);
#else
    /* Use write() for all other platforms */
    retval = write(socket->fd, buf, count);
#endif

    /* Record errors in guac_error */
    if (retval < 0) {
        guac_error = GUAC_STATUS_SEE_ERRNO;
        guac_error_message = "Error writing data to socket";
    }

    return retval;
}

ssize_t guac_socket_write_int(guac_socket* socket, int64_t i) {

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "%"PRIi64, i);
    return guac_socket_write_string(socket, buffer);

}

ssize_t guac_socket_write_string(guac_socket* socket, const char* str) {

    char* __out_buf = socket->__out_buf;

    int retval;

    for (; *str != '\0'; str++) {

        __out_buf[socket->__written++] = *str; 

        /* Flush when necessary, return on error */
        if (socket->__written > 8188 /* sizeof(__out_buf) - 4 */) {

            retval = socket->write_handler(socket,
                    __out_buf, socket->__written);

            if (retval < 0)
                return retval;

            socket->__written = 0;

        }

    }

    return 0;

}

ssize_t __guac_socket_write_base64_triplet(guac_socket* socket, int a, int b, int c) {

    char* __out_buf = socket->__out_buf;

    int retval;

    /* Byte 1 */
    __out_buf[socket->__written++] = __guac_socket_BASE64_CHARACTERS[(a & 0xFC) >> 2]; /* [AAAAAA]AABBBB BBBBCC CCCCCC */

    if (b >= 0) {
        __out_buf[socket->__written++] = __guac_socket_BASE64_CHARACTERS[((a & 0x03) << 4) | ((b & 0xF0) >> 4)]; /* AAAAAA[AABBBB]BBBBCC CCCCCC */

        if (c >= 0) {
            __out_buf[socket->__written++] = __guac_socket_BASE64_CHARACTERS[((b & 0x0F) << 2) | ((c & 0xC0) >> 6)]; /* AAAAAA AABBBB[BBBBCC]CCCCCC */
            __out_buf[socket->__written++] = __guac_socket_BASE64_CHARACTERS[c & 0x3F]; /* AAAAAA AABBBB BBBBCC[CCCCCC] */
        }
        else { 
            __out_buf[socket->__written++] = __guac_socket_BASE64_CHARACTERS[((b & 0x0F) << 2)]; /* AAAAAA AABBBB[BBBB--]------ */
            __out_buf[socket->__written++] = '='; /* AAAAAA AABBBB BBBB--[------] */
        }
    }
    else {
        __out_buf[socket->__written++] = __guac_socket_BASE64_CHARACTERS[((a & 0x03) << 4)]; /* AAAAAA[AA----]------ ------ */
        __out_buf[socket->__written++] = '='; /* AAAAAA AA----[------]------ */
        __out_buf[socket->__written++] = '='; /* AAAAAA AA---- ------[------] */
    }

    /* At this point, 4 bytes have been socket->__written */

    /* Flush when necessary, return on error */
    if (socket->__written > 8188 /* sizeof(__out_buf) - 4 */) {
        retval = socket->write_handler(socket, __out_buf, socket->__written);
        if (retval < 0)
            return retval;

        socket->__written = 0;
    }

    if (b < 0)
        return 1;

    if (c < 0)
        return 2;

    return 3;

}

ssize_t __guac_socket_write_base64_byte(guac_socket* socket, int buf) {

    int* __ready_buf = socket->__ready_buf;

    int retval;

    __ready_buf[socket->__ready++] = buf;

    /* Flush triplet */
    if (socket->__ready == 3) {
        retval = __guac_socket_write_base64_triplet(socket, __ready_buf[0], __ready_buf[1], __ready_buf[2]);
        if (retval < 0)
            return retval;

        socket->__ready = 0;
    }

    return 1;
}

ssize_t guac_socket_write_base64(guac_socket* socket, const void* buf, size_t count) {

    int retval;

    const unsigned char* char_buf = (const unsigned char*) buf;
    const unsigned char* end = char_buf + count;

    while (char_buf < end) {

        retval = __guac_socket_write_base64_byte(socket, *(char_buf++));
        if (retval < 0)
            return retval;

    }

    return 0;

}

ssize_t guac_socket_flush(guac_socket* socket) {

    int retval;

    /* Flush remaining bytes in buffer */
    if (socket->__written > 0) {
        retval = __guac_socket_write(socket, socket->__out_buf, socket->__written);
        if (retval < 0)
            return retval;

        socket->__written = 0;
    }

    return 0;

}

ssize_t guac_socket_flush_base64(guac_socket* socket) {

    int retval;

    /* Flush triplet to output buffer */
    while (socket->__ready > 0) {
        retval = __guac_socket_write_base64_byte(socket, -1);
        if (retval < 0)
            return retval;
    }

    return 0;

}


int guac_socket_select(guac_socket* socket, int usec_timeout) {

    fd_set fds;
    struct timeval timeout;
    int retval;

    /* No timeout if usec_timeout is negative */
    if (usec_timeout < 0)
        retval = select(socket->fd + 1, &fds, NULL, NULL, NULL); 

    /* Handle timeout if specified */
    else {
        timeout.tv_sec = usec_timeout/1000000;
        timeout.tv_usec = usec_timeout%1000000;

        FD_ZERO(&fds);
        FD_SET(socket->fd, &fds);

        retval = select(socket->fd + 1, &fds, NULL, NULL, &timeout);
    }

    /* Properly set guac_error */
    if (retval <  0) {
        guac_error = GUAC_STATUS_SEE_ERRNO;
        guac_error_message = "Error while waiting for data on socket";
    }

    if (retval == 0) {
        guac_error = GUAC_STATUS_INPUT_TIMEOUT;
        guac_error_message = "Timeout while waiting for data on socket";
    }

    return retval;

}

