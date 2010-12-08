
/*
 *  Guacamole - Clientless Remote Desktop
 *  Copyright (C) 2010  Michael Jumper
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#include <time.h>
#include <sys/select.h>
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
    io->fd = fd;

    /* Allocate instruction buffer */
    io->instructionbuf_size = 1024;
    io->instructionbuf = malloc(io->instructionbuf_size);
    io->instructionbuf_used_length = 0;

    /* Set limit */
    io->transfer_limit = 0;

    return io;

}

void guac_close(GUACIO* io) {
    guac_flush(io);
    free(io);
}

/* Write bytes, limit rate */
ssize_t __guac_write(GUACIO* io, const char* buf, int count) {

    struct timeval start, end;
    int retval;

    /* Write and time how long the write takes (microseconds) */
    gettimeofday(&start, NULL);
    retval = write(io->fd, buf, count);
    gettimeofday(&end, NULL);

    if (retval < 0)
        return retval;

    if (io->transfer_limit > 0) {

        suseconds_t elapsed;
        suseconds_t required_usecs;

        /* Get elapsed time */
        elapsed = (end.tv_sec - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec;

        /* Calculate how much time we must sleep */
        required_usecs = retval * 1000 / io->transfer_limit - elapsed; /* useconds at transfer_limit KB/s*/

        /* Sleep as necessary */
        if (required_usecs > 0) {

            struct timespec required_sleep;

            required_sleep.tv_sec = required_usecs / 1000000;
            required_sleep.tv_nsec = (required_usecs % 1000000) * 1000;

            nanosleep(&required_sleep, NULL);

        }

    }

    return retval;
}

ssize_t guac_write_int(GUACIO* io, unsigned int i) {

    char buffer[128];
    char* ptr = &(buffer[127]);

    *ptr = 0;

    do {

        ptr--;
        *ptr = '0' + (i % 10);

        i /= 10;

    } while (i > 0 && ptr >= buffer);

    return guac_write_string(io, ptr);

}

ssize_t guac_write_string(GUACIO* io, const char* str) {

    char* out_buf = io->out_buf;

    int retval;

    for (; *str != '\0'; str++) {

        out_buf[io->written++] = *str; 

        /* Flush when necessary, return on error */
        if (io->written > 8188 /* sizeof(out_buf) - 4 */) {

            struct timeval start, end;
            suseconds_t elapsed;

            gettimeofday(&start, NULL);
            retval = __guac_write(io, out_buf, io->written);
            gettimeofday(&end, NULL);

            if (retval < 0)
                return retval;

            /* Get elapsed time */
            elapsed = (end.tv_sec - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec;

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

    if (b >= 0) {
        out_buf[io->written++] = __GUACIO_BASE64_CHARACTERS[((a & 0x03) << 4) | ((b & 0xF0) >> 4)]; /* AAAAAA[AABBBB]BBBBCC CCCCCC */

        if (c >= 0) {
            out_buf[io->written++] = __GUACIO_BASE64_CHARACTERS[((b & 0x0F) << 2) | ((c & 0xC0) >> 6)]; /* AAAAAA AABBBB[BBBBCC]CCCCCC */
            out_buf[io->written++] = __GUACIO_BASE64_CHARACTERS[c & 0x3F]; /* AAAAAA AABBBB BBBBCC[CCCCCC] */
        }
        else { 
            out_buf[io->written++] = __GUACIO_BASE64_CHARACTERS[((b & 0x0F) << 2)]; /* AAAAAA AABBBB[BBBB--]------ */
            out_buf[io->written++] = '='; /* AAAAAA AABBBB BBBB--[------] */
        }
    }
    else {
        out_buf[io->written++] = __GUACIO_BASE64_CHARACTERS[((a & 0x03) << 4)]; /* AAAAAA[AA----]------ ------ */
        out_buf[io->written++] = '='; /* AAAAAA AA----[------]------ */
        out_buf[io->written++] = '='; /* AAAAAA AA---- ------[------] */
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

