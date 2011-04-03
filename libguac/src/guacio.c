
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
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#ifdef __MINGW32__
#include <winsock2.h>
#else
#include <sys/select.h>
#endif

#include <time.h>
#include <sys/time.h>

#include "guacio.h"

char __GUACIO_BASE64_CHARACTERS[64] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/', 
};

GUACIO* guac_open(int fd) {

    GUACIO* io = malloc(sizeof(GUACIO));
    io->ready = 0;
    io->written = 0;
    io->total_written = 0;
    io->fd = fd;

    /* Allocate instruction buffer */
    io->instructionbuf_size = 1024;
    io->instructionbuf = malloc(io->instructionbuf_size);
    io->instructionbuf_used_length = 0;

    return io;

}

void guac_close(GUACIO* io) {
    guac_flush(io);
    free(io->instructionbuf);
    free(io);
}

/* Write bytes, limit rate */
ssize_t __guac_write(GUACIO* io, const char* buf, int count) {

    int retval;

#ifdef __MINGW32__
    /* MINGW32 WINSOCK only works with send() */
    retval = send(io->fd, buf, count, 0);
#else
    /* Use write() for all other platforms */
    retval = write(io->fd, buf, count);
#endif

    return retval;
}

ssize_t guac_write_int(GUACIO* io, long i) {

    char buffer[128];
    char* ptr = &(buffer[127]);
    long nonneg;

    /* Obtain non-negative value */
    if (i < 0) nonneg = -i;
    else       nonneg = i;

    /* Generate numeric string */

    *ptr = 0;

    do {

        ptr--;
        *ptr = '0' + (nonneg % 10);

        nonneg /= 10;

    } while (nonneg > 0 && ptr >= buffer);

    /* Prepend with dash if negative */
    if (i < 0 && ptr >= buffer) *(--ptr) = '-';

    return guac_write_string(io, ptr);

}

ssize_t guac_write_string(GUACIO* io, const char* str) {

    char* out_buf = io->out_buf;

    int retval;

    for (; *str != '\0'; str++) {

        out_buf[io->written++] = *str; 
        io->total_written++;

        /* Flush when necessary, return on error */
        if (io->written > 8188 /* sizeof(out_buf) - 4 */) {

            retval = __guac_write(io, out_buf, io->written);

            if (retval < 0)
                return retval;

            io->written = 0;
        }

    }

    return 0;

}

ssize_t __guac_write_base64_triplet(GUACIO* io, int a, int b, int c) {

    char* out_buf = io->out_buf;

    int retval;

    /* Byte 1 */
    out_buf[io->written++] = __GUACIO_BASE64_CHARACTERS[(a & 0xFC) >> 2]; /* [AAAAAA]AABBBB BBBBCC CCCCCC */
    io->total_written++;

    if (b >= 0) {
        out_buf[io->written++] = __GUACIO_BASE64_CHARACTERS[((a & 0x03) << 4) | ((b & 0xF0) >> 4)]; /* AAAAAA[AABBBB]BBBBCC CCCCCC */
        io->total_written++;

        if (c >= 0) {
            out_buf[io->written++] = __GUACIO_BASE64_CHARACTERS[((b & 0x0F) << 2) | ((c & 0xC0) >> 6)]; /* AAAAAA AABBBB[BBBBCC]CCCCCC */
            out_buf[io->written++] = __GUACIO_BASE64_CHARACTERS[c & 0x3F]; /* AAAAAA AABBBB BBBBCC[CCCCCC] */
            io->total_written += 2;
        }
        else { 
            out_buf[io->written++] = __GUACIO_BASE64_CHARACTERS[((b & 0x0F) << 2)]; /* AAAAAA AABBBB[BBBB--]------ */
            out_buf[io->written++] = '='; /* AAAAAA AABBBB BBBB--[------] */
            io->total_written += 2;
        }
    }
    else {
        out_buf[io->written++] = __GUACIO_BASE64_CHARACTERS[((a & 0x03) << 4)]; /* AAAAAA[AA----]------ ------ */
        out_buf[io->written++] = '='; /* AAAAAA AA----[------]------ */
        out_buf[io->written++] = '='; /* AAAAAA AA---- ------[------] */
        io->total_written += 3;
    }

    /* At this point, 4 bytes have been io->written */

    /* Flush when necessary, return on error */
    if (io->written > 8188 /* sizeof(out_buf) - 4 */) {
        retval = __guac_write(io, out_buf, io->written);
        if (retval < 0)
            return retval;

        io->written = 0;
    }

    if (b < 0)
        return 1;

    if (c < 0)
        return 2;

    return 3;

}

ssize_t __guac_write_base64_byte(GUACIO* io, char buf) {

    int* ready_buf = io->ready_buf;

    int retval;

    ready_buf[io->ready++] = buf & 0xFF;

    /* Flush triplet */
    if (io->ready == 3) {
        retval = __guac_write_base64_triplet(io, ready_buf[0], ready_buf[1], ready_buf[2]);
        if (retval < 0)
            return retval;

        io->ready = 0;
    }

    return 1;
}

ssize_t guac_write_base64(GUACIO* io, const void* buf, size_t count) {

    int retval;

    const char* char_buf = (const char*) buf;
    const char* end = char_buf + count;

    while (char_buf < end) {

        retval = __guac_write_base64_byte(io, *(char_buf++));
        if (retval < 0)
            return retval;

    }

    return count;

}

ssize_t guac_flush(GUACIO* io) {

    int retval;

    /* Flush remaining bytes in buffer */
    if (io->written > 0) {
        retval = __guac_write(io, io->out_buf, io->written);
        if (retval < 0)
            return retval;

        io->written = 0;
    }

    return 0;

}

ssize_t guac_flush_base64(GUACIO* io) {

    int retval;

    /* Flush triplet to output buffer */
    while (io->ready > 0) {
        retval = __guac_write_base64_byte(io, -1);
        if (retval < 0)
            return retval;
    }

    return 0;

}


int guac_select(GUACIO* io, int usec_timeout) {

    fd_set fds;
    struct timeval timeout;

    if (usec_timeout < 0)
        return select(io->fd + 1, &fds, NULL, NULL, NULL); 

    timeout.tv_sec = usec_timeout/1000000;
    timeout.tv_usec = usec_timeout%1000000;

    FD_ZERO(&fds);
    FD_SET(io->fd, &fds);

    return select(io->fd + 1, &fds, NULL, NULL, &timeout); 

}

