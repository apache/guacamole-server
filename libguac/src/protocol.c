
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

#if defined(HAVE_CLOCK_GETTIME) || defined(HAVE_NANOSLEEP)
#include <time.h>
#endif

#ifndef HAVE_CLOCK_GETTIME
#include <sys/time.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>

#include <cairo/cairo.h>

#include <sys/types.h>

#ifdef __MINGW32__
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif

#include "socket.h"
#include "protocol.h"
#include "error.h"

ssize_t __guac_socket_write_length_string(guac_socket* io, const char* str) {

    return
           guac_socket_write_int(io, strlen(str))
        || guac_socket_write_string(io, ".")
        || guac_socket_write_string(io, str);

}

ssize_t __guac_socket_write_length_int(guac_socket* io, int64_t i) {

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "%"PRIi64, i);
    return __guac_socket_write_length_string(io, buffer);

}

int guac_protocol_send_args(guac_socket* io, const char** args) {

    int i;

    if (guac_socket_write_string(io, "4.args")) return -1;

    for (i=0; args[i] != NULL; i++) {

        if (guac_socket_write_string(io, ","))
            return -1;

        if (__guac_socket_write_length_string(io, args[i]))
            return -1;

    }

    return guac_socket_write_string(io, ";");

}

int guac_protocol_send_name(guac_socket* io, const char* name) {

    return
           guac_socket_write_string(io, "4.name,")
        || __guac_socket_write_length_string(io, name)
        || guac_socket_write_string(io, ";");

}

int guac_protocol_send_size(guac_socket* io, int w, int h) {

    return
           guac_socket_write_string(io, "4.size,")
        || __guac_socket_write_length_int(io, w)
        || guac_socket_write_string(io, ",")
        || __guac_socket_write_length_int(io, h)
        || guac_socket_write_string(io, ";");

}

int guac_protocol_send_clipboard(guac_socket* io, const char* data) {

    return
           guac_socket_write_string(io, "9.clipboard,")
        || __guac_socket_write_length_string(io, data)
        || guac_socket_write_string(io, ";");

}

int guac_protocol_send_error(guac_socket* io, const char* error) {

    return
           guac_socket_write_string(io, "5.error,")
        || __guac_socket_write_length_string(io, error)
        || guac_socket_write_string(io, ";");
}

int guac_protocol_send_sync(guac_socket* io, guac_timestamp timestamp) {

    return 
           guac_socket_write_string(io, "4.sync,")
        || __guac_socket_write_length_int(io, timestamp)
        || guac_socket_write_string(io, ";");

}

int guac_protocol_send_copy(guac_socket* io,
        const guac_layer* srcl, int srcx, int srcy, int w, int h,
        guac_composite_mode mode, const guac_layer* dstl, int dstx, int dsty) {

    return
           guac_socket_write_string(io, "4.copy,")
        || __guac_socket_write_length_int(io, srcl->index)
        || guac_socket_write_string(io, ",")
        || __guac_socket_write_length_int(io, srcx)
        || guac_socket_write_string(io, ",")
        || __guac_socket_write_length_int(io, srcy)
        || guac_socket_write_string(io, ",")
        || __guac_socket_write_length_int(io, w)
        || guac_socket_write_string(io, ",")
        || __guac_socket_write_length_int(io, h)
        || guac_socket_write_string(io, ",")
        || __guac_socket_write_length_int(io, mode)
        || guac_socket_write_string(io, ",")
        || __guac_socket_write_length_int(io, dstl->index)
        || guac_socket_write_string(io, ",")
        || __guac_socket_write_length_int(io, dstx)
        || guac_socket_write_string(io, ",")
        || __guac_socket_write_length_int(io, dsty)
        || guac_socket_write_string(io, ";");

}

int guac_protocol_send_rect(guac_socket* io,
        guac_composite_mode mode, const guac_layer* layer,
        int x, int y, int width, int height,
        int r, int g, int b, int a) {

    return
           guac_socket_write_string(io, "4.rect,")
        || __guac_socket_write_length_int(io, mode)
        || guac_socket_write_string(io, ",")
        || __guac_socket_write_length_int(io, layer->index)
        || guac_socket_write_string(io, ",")
        || __guac_socket_write_length_int(io, x)
        || guac_socket_write_string(io, ",")
        || __guac_socket_write_length_int(io, y)
        || guac_socket_write_string(io, ",")
        || __guac_socket_write_length_int(io, width)
        || guac_socket_write_string(io, ",")
        || __guac_socket_write_length_int(io, height)
        || guac_socket_write_string(io, ",")
        || __guac_socket_write_length_int(io, r)
        || guac_socket_write_string(io, ",")
        || __guac_socket_write_length_int(io, g)
        || guac_socket_write_string(io, ",")
        || __guac_socket_write_length_int(io, b)
        || guac_socket_write_string(io, ",")
        || __guac_socket_write_length_int(io, a)
        || guac_socket_write_string(io, ";");

}

int guac_protocol_send_clip(guac_socket* io, const guac_layer* layer,
        int x, int y, int width, int height) {

    return
           guac_socket_write_string(io, "4.clip,")
        || __guac_socket_write_length_int(io, layer->index)
        || guac_socket_write_string(io, ",")
        || __guac_socket_write_length_int(io, x)
        || guac_socket_write_string(io, ",")
        || __guac_socket_write_length_int(io, y)
        || guac_socket_write_string(io, ",")
        || __guac_socket_write_length_int(io, width)
        || guac_socket_write_string(io, ",")
        || __guac_socket_write_length_int(io, height)
        || guac_socket_write_string(io, ";");

}

typedef struct __guac_socket_write_png_data {

    guac_socket* io;

    char* buffer;
    int buffer_size;
    int data_size;

} __guac_socket_write_png_data;

cairo_status_t __guac_socket_write_png(void* closure, const unsigned char* data, unsigned int length) {

    __guac_socket_write_png_data* png_data = (__guac_socket_write_png_data*) closure;

    /* Calculate next buffer size */
    int next_size = png_data->data_size + length;

    /* If need resizing, double buffer size until big enough */
    if (next_size > png_data->buffer_size) {

        char* new_buffer;

        do {
            png_data->buffer_size <<= 1;
        } while (next_size > png_data->buffer_size);

        /* Resize buffer */
        new_buffer = realloc(png_data->buffer, png_data->buffer_size);
        png_data->buffer = new_buffer;

    }

    /* Append data to buffer */
    memcpy(png_data->buffer + png_data->data_size, data, length);
    png_data->data_size += length;

    return CAIRO_STATUS_SUCCESS;

}

int __guac_socket_write_length_png(guac_socket* io, cairo_surface_t* surface) {

    __guac_socket_write_png_data png_data;
    int base64_length;

    /* Write surface */

    png_data.io = io;
    png_data.buffer_size = 8192;
    png_data.buffer = malloc(png_data.buffer_size);
    png_data.data_size = 0;

    if (cairo_surface_write_to_png_stream(surface, __guac_socket_write_png, &png_data) != CAIRO_STATUS_SUCCESS) {
        return -1;
    }

    base64_length = (png_data.data_size + 2) / 3 * 4;

    /* Write length and data */
    if (
           guac_socket_write_int(io, base64_length)
        || guac_socket_write_string(io, ".")
        || guac_socket_write_base64(io, png_data.buffer, png_data.data_size)
        || guac_socket_flush_base64(io)) {
        free(png_data.buffer);
        return -1;
    }

    free(png_data.buffer);
    return 0;

}


int guac_protocol_send_png(guac_socket* io, guac_composite_mode mode,
        const guac_layer* layer, int x, int y, cairo_surface_t* surface) {

    return
           guac_socket_write_string(io, "3.png,")
        || __guac_socket_write_length_int(io, mode)
        || guac_socket_write_string(io, ",")
        || __guac_socket_write_length_int(io, layer->index)
        || guac_socket_write_string(io, ",")
        || __guac_socket_write_length_int(io, x)
        || guac_socket_write_string(io, ",")
        || __guac_socket_write_length_int(io, y)
        || guac_socket_write_string(io, ",")
        || __guac_socket_write_length_png(io, surface)
        || guac_socket_write_string(io, ";");

}


int guac_protocol_send_cursor(guac_socket* io, int x, int y, cairo_surface_t* surface) {

    return
           guac_socket_write_string(io, "6.cursor,")
        || __guac_socket_write_length_int(io, x)
        || guac_socket_write_string(io, ",")
        || __guac_socket_write_length_int(io, y)
        || guac_socket_write_string(io, ",")
        || __guac_socket_write_length_png(io, surface)
        || guac_socket_write_string(io, ";");

}


int __guac_fill_instructionbuf(guac_socket* io) {

    int retval;
    
    /* Attempt to fill buffer */
    retval = recv(
            io->fd,
            io->__instructionbuf + io->__instructionbuf_used_length,
            io->__instructionbuf_size - io->__instructionbuf_used_length,
            0
    );

    if (retval < 0)
        return retval;

    io->__instructionbuf_used_length += retval;

    /* Expand buffer if necessary */
    if (io->__instructionbuf_used_length > io->__instructionbuf_size / 2) {
        io->__instructionbuf_size *= 2;
        io->__instructionbuf = realloc(io->__instructionbuf, io->__instructionbuf_size);
    }

    return retval;

}

/* Returns new instruction if one exists, or NULL if no more instructions. */
guac_instruction* guac_protocol_read_instruction(guac_socket* io, int usec_timeout) {

    int retval;
    int i = io->__instructionbuf_parse_start;
    
    /* Loop until a instruction is read */
    for (;;) {

        /* Length of element */
        int element_length = 0;

        /* Parse instruction in buffe */
        while (i < io->__instructionbuf_used_length) {

            /* Read character from buffer */
            char c = io->__instructionbuf[i++];

            /* If digit, calculate element length */
            if (c >= '0' && c <= '9')
                element_length = element_length * 10 + c - '0';

            /* Otherwise, if end of length */
            else if (c == '.') {

                /* Verify element is fully read */
                if (i + element_length < io->__instructionbuf_used_length) {

                    /* Get element value */
                    char* elementv = &(io->__instructionbuf[i]);
                   
                    /* Get terminator, set null terminator of elementv */ 
                    char terminator = elementv[element_length];
                    elementv[element_length] = '\0';

                    /* Move to terminator of element */
                    i += element_length;

                    /* Reset element length */
                    element_length = 0;

                    /* As element has been read successfully, update
                     * parse start */
                    io->__instructionbuf_parse_start = i;

                    /* Save element */
                    io->__instructionbuf_elementv[io->__instructionbuf_elementc++] = elementv;

                    /* Finish parse if terminator is a semicolon */
                    if (terminator == ';') {

                        guac_instruction* parsed_instruction;
                        int j;

                        /* Allocate instruction */
                        parsed_instruction = malloc(sizeof(guac_instruction));
                        if (parsed_instruction == NULL) {
                            guac_error = GUAC_STATUS_NO_MEMORY;
                            return NULL;
                        }

                        /* Init parsed instruction */
                        parsed_instruction->argc = io->__instructionbuf_elementc - 1;
                        parsed_instruction->argv = malloc(sizeof(char*) * parsed_instruction->argc);

                        /* Fail if memory could not be alloc'd for argv */
                        if (parsed_instruction->argv == NULL) {
                            guac_error = GUAC_STATUS_NO_MEMORY;
                            free(parsed_instruction);
                            return NULL;
                        }

                        /* Set opcode */
                        parsed_instruction->opcode = strdup(io->__instructionbuf_elementv[0]);

                        /* Fail if memory could not be alloc'd for opcode */
                        if (parsed_instruction->opcode == NULL) {
                            guac_error = GUAC_STATUS_NO_MEMORY;
                            free(parsed_instruction->argv);
                            free(parsed_instruction);
                            return NULL;
                        }


                        /* Copy element values to parsed instruction */
                        for (j=0; j<parsed_instruction->argc; j++) {
                            parsed_instruction->argv[j] = strdup(io->__instructionbuf_elementv[j+1]);

                            /* Free memory and fail if out of mem */
                            if (parsed_instruction->argv[j] == NULL) {
                                guac_error = GUAC_STATUS_NO_MEMORY;

                                /* Free all alloc'd argv values */
                                while (--j >= 0)
                                    free(parsed_instruction->argv[j]);

                                free(parsed_instruction->opcode);
                                free(parsed_instruction->argv);
                                free(parsed_instruction);
                                return NULL;
                            }

                        }

                        /* Reset buffer */
                        memmove(io->__instructionbuf, io->__instructionbuf + i + 1, io->__instructionbuf_used_length - i - 1);
                        io->__instructionbuf_used_length -= i + 1;
                        io->__instructionbuf_parse_start = 0;
                        io->__instructionbuf_elementc = 0;

                        /* Done */
                        return parsed_instruction;

                    } /* end if terminator */

                } /* end if element fully read */

                /* Otherwise, read more data */
                else
                    break;

            }

        }

        /* No instruction yet? Get more data ... */
        retval = guac_socket_select(io, usec_timeout);
        if (retval <= 0)
            return NULL;

        /* If more data is available, fill into buffer */
        retval = __guac_fill_instructionbuf(io);

        /* Error, guac_error already set */
        if (retval < 0)
            return NULL;

        /* EOF */
        if (retval == 0) {
            guac_error = GUAC_STATUS_NO_INPUT;
            return NULL;
        }

    }

}

guac_instruction* guac_protocol_expect_instruction(guac_socket* io, int usec_timeout,
        const char* opcode) {

    guac_instruction* instruction;

    /* Wait for data until timeout */
    if (guac_protocol_instructions_waiting(io, usec_timeout) <= 0)
        return NULL;

    /* Read available instruction */
    instruction = guac_protocol_read_instruction(io, usec_timeout);
    if (instruction == NULL)
        return NULL;            

    /* Validate instruction */
    if (strcmp(instruction->opcode, opcode) != 0) {
        guac_error = GUAC_STATUS_BAD_STATE;
        guac_instruction_free(instruction);
        return NULL;
    }

    /* Return instruction if valid */
    return instruction;

}

void guac_instruction_free_data(guac_instruction* instruction) {
    free(instruction->opcode);

    if (instruction->argv)
        free(instruction->argv);
}

void guac_instruction_free(guac_instruction* instruction) {
    guac_instruction_free_data(instruction);
    free(instruction);
}


int guac_protocol_instructions_waiting(guac_socket* io, int usec_timeout) {

    if (io->__instructionbuf_used_length > 0)
        return 1;

    return guac_socket_select(io, usec_timeout);
}

guac_timestamp guac_protocol_get_timestamp() {

#ifdef HAVE_CLOCK_GETTIME

    struct timespec current;

    /* Get current time */
    clock_gettime(CLOCK_REALTIME, &current);
    
    /* Calculate milliseconds */
    return (guac_timestamp) current.tv_sec * 1000 + current.tv_nsec / 1000000;

#else

    struct timeval current;

    /* Get current time */
    gettimeofday(&current, NULL);
    
    /* Calculate milliseconds */
    return (guac_timestamp) current.tv_sec * 1000 + current.tv_usec / 1000;

#endif

}

void guac_sleep(int millis) {

#ifdef HAVE_NANOSLEEP 
        struct timespec sleep_period;

        sleep_period.tv_sec =   millis / 1000;
        sleep_period.tv_nsec = (millis % 1000) * 1000000L;

        nanosleep(&sleep_period, NULL);
#elif defined(__MINGW32__)
        Sleep(millis);
#else
#warning No sleep/nanosleep function available. Clients may not perform as expected. Consider patching libguac to add support for your platform.
#endif

}

