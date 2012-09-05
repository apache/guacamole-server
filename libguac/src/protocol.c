
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

#ifdef HAVE_PNGSTRUCT_H
#include <pngstruct.h>
#endif

#include <png.h>

#include <cairo/cairo.h>

#include <sys/types.h>

#ifdef __MINGW32__
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif

#include "error.h"
#include "instruction.h"
#include "layer.h"
#include "palette.h"
#include "protocol.h"
#include "socket.h"
#include "unicode.h"

/* Output formatting functions */

ssize_t __guac_socket_write_length_string(guac_socket* socket, const char* str) {

    return
           guac_socket_write_int(socket, guac_utf8_strlen(str))
        || guac_socket_write_string(socket, ".")
        || guac_socket_write_string(socket, str);

}


ssize_t __guac_socket_write_length_int(guac_socket* socket, int64_t i) {

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "%"PRIi64, i);
    return __guac_socket_write_length_string(socket, buffer);

}


ssize_t __guac_socket_write_length_double(guac_socket* socket, double d) {

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "%g", d);
    return __guac_socket_write_length_string(socket, buffer);

}


/* PNG output formatting */

typedef struct __guac_socket_write_png_data {

    guac_socket* socket;

    char* buffer;
    int buffer_size;
    int data_size;

} __guac_socket_write_png_data;

cairo_status_t __guac_socket_write_png_cairo(void* closure, const unsigned char* data, unsigned int length) {

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


int __guac_socket_write_length_png_cairo(guac_socket* socket, cairo_surface_t* surface) {

    __guac_socket_write_png_data png_data;
    int base64_length;

    /* Write surface */

    png_data.socket = socket;
    png_data.buffer_size = 8192;
    png_data.buffer = malloc(png_data.buffer_size);
    png_data.data_size = 0;

    if (cairo_surface_write_to_png_stream(surface, __guac_socket_write_png_cairo, &png_data) != CAIRO_STATUS_SUCCESS) {
        guac_error = GUAC_STATUS_OUTPUT_ERROR;
        guac_error_message = "Cairo PNG backend failed";
        return -1;
    }

    base64_length = (png_data.data_size + 2) / 3 * 4;

    /* Write length and data */
    if (
           guac_socket_write_int(socket, base64_length)
        || guac_socket_write_string(socket, ".")
        || guac_socket_write_base64(socket, png_data.buffer, png_data.data_size)
        || guac_socket_flush_base64(socket)) {
        free(png_data.buffer);
        return -1;
    }

    free(png_data.buffer);
    return 0;

}

void __guac_socket_write_png(png_structp png,
        png_bytep data, png_size_t length) {

    /* Get png buffer structure */
    __guac_socket_write_png_data* png_data;
#ifdef HAVE_PNG_GET_IO_PTR
    png_data = (__guac_socket_write_png_data*) png_get_io_ptr(png);
#else
    png_data = (__guac_socket_write_png_data*) png->io_ptr;
#endif

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

}

void __guac_socket_flush_png(png_structp png) {
    /* Dummy function */
}

int __guac_socket_write_length_png(guac_socket* socket, cairo_surface_t* surface) {

    png_structp png;
    png_infop png_info;
    png_byte** png_rows;
    int bpp;

    int x, y;

    __guac_socket_write_png_data png_data;
    int base64_length;

    /* Get image surface properties and data */
    cairo_format_t format = cairo_image_surface_get_format(surface);
    int width = cairo_image_surface_get_width(surface);
    int height = cairo_image_surface_get_height(surface);
    int stride = cairo_image_surface_get_stride(surface);
    unsigned char* data = cairo_image_surface_get_data(surface);

    /* If not RGB24, use Cairo PNG writer */
    if (format != CAIRO_FORMAT_RGB24 || data == NULL)
        return __guac_socket_write_length_png_cairo(socket, surface);

    /* Flush pending operations to surface */
    cairo_surface_flush(surface);

    /* Attempt to build palette */
    guac_palette* palette = guac_palette_alloc(surface);

    /* If not possible, resort to Cairo PNG writer */
    if (palette == NULL)
        return __guac_socket_write_length_png_cairo(socket, surface);

    /* Calculate BPP from palette size */
    if      (palette->size <= 2)  bpp = 1;
    else if (palette->size <= 4)  bpp = 2;
    else if (palette->size <= 16) bpp = 4;
    else                          bpp = 8;

    /* Set up PNG writer */
    png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) {
        guac_error = GUAC_STATUS_OUTPUT_ERROR;
        guac_error_message = "libpng failed to create write structure";
        return -1;
    }

    png_info = png_create_info_struct(png);
    if (!png_info) {
        png_destroy_write_struct(&png, NULL);
        guac_error = GUAC_STATUS_OUTPUT_ERROR;
        guac_error_message = "libpng failed to create info structure";
        return -1;
    }

    /* Set error handler */
    if (setjmp(png_jmpbuf(png))) {
        png_destroy_write_struct(&png, &png_info);
        guac_error = GUAC_STATUS_OUTPUT_ERROR;
        guac_error_message = "libpng output error";
        return -1;
    }

    /* Set up buffer structure */
    png_data.socket = socket;
    png_data.buffer_size = 8192;
    png_data.buffer = malloc(png_data.buffer_size);
    png_data.data_size = 0;

    /* Set up writer */
    png_set_write_fn(png, &png_data,
            __guac_socket_write_png,
            __guac_socket_flush_png);

    /* Copy data from surface into PNG data */
    png_rows = (png_byte**) malloc(sizeof(png_byte*) * height);
    for (y=0; y<height; y++) {

        /* Allocate new PNG row */
        png_byte* row = (png_byte*) malloc(sizeof(png_byte) * width);
        png_rows[y] = row;

        /* Copy data from surface into current row */
        for (x=0; x<width; x++) {

            /* Get pixel color */
            int color = ((uint32_t*) data)[x] & 0xFFFFFF;

            /* Set index in row */
            row[x] = guac_palette_find(palette, color);

        }

        /* Advance to next data row */
        data += stride;

    }

    /* Write image info */
    png_set_IHDR(
        png,
        png_info,
        width,
        height,
        bpp,
        PNG_COLOR_TYPE_PALETTE,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT
    );

    /* Write palette */
    png_set_PLTE(png, png_info, palette->colors, palette->size);

    /* Write image */
    png_set_rows(png, png_info, png_rows);
    png_write_png(png, png_info, PNG_TRANSFORM_PACKING, NULL);

    /* Finish write */
    png_destroy_write_struct(&png, &png_info);

    /* Free palette */
    guac_palette_free(palette);

    /* Free PNG data */
    for (y=0; y<height; y++)
        free(png_rows[y]);
    free(png_rows);

    base64_length = (png_data.data_size + 2) / 3 * 4;

    /* Write length and data */
    if (
           guac_socket_write_int(socket, base64_length)
        || guac_socket_write_string(socket, ".")
        || guac_socket_write_base64(socket, png_data.buffer, png_data.data_size)
        || guac_socket_flush_base64(socket)) {
        free(png_data.buffer);
        return -1;
    }

    free(png_data.buffer);
    return 0;

}


/* Instruction I/O */

int __guac_fill_instructionbuf(guac_socket* socket) {

    int retval;
    
    /* Attempt to fill buffer */
    retval = read(
        socket->fd,
        socket->__instructionbuf + socket->__instructionbuf_used_length,
        socket->__instructionbuf_size - socket->__instructionbuf_used_length
    );

    /* Set guac_error if recv() unsuccessful */
    if (retval < 0) {
        guac_error = GUAC_STATUS_SEE_ERRNO;
        guac_error_message = "Error filling instruction buffer";
        return retval;
    }

    socket->__instructionbuf_used_length += retval;

    /* Expand buffer if necessary */
    if (socket->__instructionbuf_used_length >
            socket->__instructionbuf_size / 2) {

        socket->__instructionbuf_size *= 2;
        socket->__instructionbuf = realloc(socket->__instructionbuf,
                socket->__instructionbuf_size);
    }

    return retval;

}


/* Returns new instruction if one exists, or NULL if no more instructions. */
guac_instruction* guac_protocol_read_instruction(guac_socket* socket,
        int usec_timeout) {

    int retval;
   
    /* Loop until a instruction is read */
    for (;;) {

        /* Length of element, in Unicode characters */
        int element_length = 0;

        /* Length of element, in bytes */
        int element_byte_length = 0;

        /* Current position within the element, in Unicode characters */
        int current_unicode_length = 0;

        /* Position within buffer */
        int i = socket->__instructionbuf_parse_start;

        /* Parse instruction in buffer */
        while (i < socket->__instructionbuf_used_length) {

            /* Read character from buffer */
            char c = socket->__instructionbuf[i++];

            /* If digit, calculate element length */
            if (c >= '0' && c <= '9')
                element_length = element_length * 10 + c - '0';

            /* Otherwise, if end of length */
            else if (c == '.') {

                /* Calculate element byte length by walking buffer */
                while (i + element_byte_length <
                            socket->__instructionbuf_used_length
                    && current_unicode_length < element_length) {

                    /* Get next byte */
                    c = socket->__instructionbuf[i + element_byte_length];

                    /* Update byte and character lengths */
                    element_byte_length += guac_utf8_charsize((unsigned) c);
                    current_unicode_length++;

                }

                /* Verify element is fully read */
                if (current_unicode_length == element_length) {

                    /* Get element value */
                    char* elementv = &(socket->__instructionbuf[i]);
                   
                    /* Get terminator, set null terminator of elementv */ 
                    char terminator = elementv[element_byte_length];
                    elementv[element_byte_length] = '\0';

                    /* Move to char after terminator of element */
                    i += element_byte_length+1;

                    /* Reset element length */
                    element_length =
                    element_byte_length =
                    current_unicode_length = 0;

                    /* As element has been read successfully, update
                     * parse start */
                    socket->__instructionbuf_parse_start = i;

                    /* Save element */
                    socket->__instructionbuf_elementv[socket->__instructionbuf_elementc++] = elementv;

                    /* Finish parse if terminator is a semicolon */
                    if (terminator == ';') {

                        guac_instruction* parsed_instruction;
                        int j;

                        /* Allocate instruction */
                        parsed_instruction = malloc(sizeof(guac_instruction));
                        if (parsed_instruction == NULL) {
                            guac_error = GUAC_STATUS_NO_MEMORY;
                            guac_error_message = "Could not allocate memory for parsed instruction";
                            return NULL;
                        }

                        /* Init parsed instruction */
                        parsed_instruction->argc = socket->__instructionbuf_elementc - 1;
                        parsed_instruction->argv = malloc(sizeof(char*) * parsed_instruction->argc);

                        /* Fail if memory could not be alloc'd for argv */
                        if (parsed_instruction->argv == NULL) {
                            guac_error = GUAC_STATUS_NO_MEMORY;
                            guac_error_message = "Could not allocate memory for arguments of parsed instruction";
                            free(parsed_instruction);
                            return NULL;
                        }

                        /* Set opcode */
                        parsed_instruction->opcode = strdup(socket->__instructionbuf_elementv[0]);

                        /* Fail if memory could not be alloc'd for opcode */
                        if (parsed_instruction->opcode == NULL) {
                            guac_error = GUAC_STATUS_NO_MEMORY;
                            guac_error_message = "Could not allocate memory for opcode of parsed instruction";
                            free(parsed_instruction->argv);
                            free(parsed_instruction);
                            return NULL;
                        }


                        /* Copy element values to parsed instruction */
                        for (j=0; j<parsed_instruction->argc; j++) {
                            parsed_instruction->argv[j] = strdup(socket->__instructionbuf_elementv[j+1]);

                            /* Free memory and fail if out of mem */
                            if (parsed_instruction->argv[j] == NULL) {
                                guac_error = GUAC_STATUS_NO_MEMORY;
                                guac_error_message = "Could not allocate memory for single argument of parsed instruction";

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
                        memmove(socket->__instructionbuf, socket->__instructionbuf + i, socket->__instructionbuf_used_length - i);
                        socket->__instructionbuf_used_length -= i;
                        socket->__instructionbuf_parse_start = 0;
                        socket->__instructionbuf_elementc = 0;

                        /* Done */
                        return parsed_instruction;

                    } /* end if terminator */

                    /* Error if expected comma is not present */
                    else if (terminator != ',') {
                        guac_error = GUAC_STATUS_BAD_ARGUMENT;
                        guac_error_message = "Element terminator of instructioni was not ';' nor ','";
                        return NULL;
                    }

                } /* end if element fully read */

                /* Otherwise, read more data */
                else
                    break;

            }

            /* Error if length is non-numeric or does not end in a period */
            else {
                guac_error = GUAC_STATUS_BAD_ARGUMENT;
                guac_error_message = "Non-numeric character in element length";
                return NULL;
            }

        }

        /* No instruction yet? Get more data ... */
        retval = guac_socket_select(socket, usec_timeout);
        if (retval <= 0)
            return NULL;

        /* If more data is available, fill into buffer */
        retval = __guac_fill_instructionbuf(socket);

        /* Error, guac_error already set */
        if (retval < 0)
            return NULL;

        /* EOF */
        if (retval == 0) {
            guac_error = GUAC_STATUS_NO_INPUT;
            guac_error_message = "End of stream reached while reading instruction";
            return NULL;
        }

    }

}


guac_instruction* guac_protocol_expect_instruction(guac_socket* socket, int usec_timeout,
        const char* opcode) {

    guac_instruction* instruction;

    /* Wait for data until timeout */
    if (guac_instruction_waiting(socket, usec_timeout) <= 0)
        return NULL;

    /* Read available instruction */
    instruction = guac_protocol_read_instruction(socket, usec_timeout);
    if (instruction == NULL)
        return NULL;            

    /* Validate instruction */
    if (strcmp(instruction->opcode, opcode) != 0) {
        guac_error = GUAC_STATUS_BAD_STATE;
        guac_error_message = "Instruction read did not have expected opcode";
        guac_instruction_free(instruction);
        return NULL;
    }

    /* Return instruction if valid */
    return instruction;

}


void guac_instruction_free(guac_instruction* instruction) {

    int argc = instruction->argc;

    /* Free opcode */
    free(instruction->opcode);

    /* Free argv if set (may be NULL of argc is 0) */
    if (instruction->argv) {

        /* All argument values */
        while (argc > 0)
            free(instruction->argv[--argc]);

        /* Free actual array */
        free(instruction->argv);

    }

    /* Free instruction */
    free(instruction);

}


int guac_protocol_instructions_waiting(guac_socket* socket, int usec_timeout) {

    if (socket->__instructionbuf_used_length > 0)
        return 1;

    return guac_socket_select(socket, usec_timeout);
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


/* Protocol functions */

int guac_protocol_send_args(guac_socket* socket, const char** args) {

    int i;

    if (guac_socket_write_string(socket, "4.args")) return -1;

    for (i=0; args[i] != NULL; i++) {

        if (guac_socket_write_string(socket, ","))
            return -1;

        if (__guac_socket_write_length_string(socket, args[i]))
            return -1;

    }

    return guac_socket_write_string(socket, ";");

}


int guac_protocol_send_arc(guac_socket* socket, const guac_layer* layer,
        int x, int y, int radius, double startAngle, double endAngle,
        int negative) {

    return
           guac_socket_write_string(socket, "3.arc,")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, x)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, y)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, radius)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_double(socket, startAngle)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_double(socket, endAngle)
        || guac_socket_write_string(socket, ",")
        || guac_socket_write_string(socket, negative ? "1.1" : "1.0")
        || guac_socket_write_string(socket, ";");

}


int guac_protocol_send_cfill(guac_socket* socket,
        guac_composite_mode mode, const guac_layer* layer,
        int r, int g, int b, int a) {

    return
           guac_socket_write_string(socket, "5.cfill,")
        || __guac_socket_write_length_int(socket, mode)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, r)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, g)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, b)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, a)
        || guac_socket_write_string(socket, ";");

}


int guac_protocol_send_close(guac_socket* socket, const guac_layer* layer) {

    return
           guac_socket_write_string(socket, "5.close,")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ";");

}


int guac_protocol_send_connect(guac_socket* socket, const char** args) {

    int i;

    if (guac_socket_write_string(socket, "7.connect")) return -1;

    for (i=0; args[i] != NULL; i++) {

        if (guac_socket_write_string(socket, ","))
            return -1;

        if (__guac_socket_write_length_string(socket, args[i]))
            return -1;

    }

    return guac_socket_write_string(socket, ";");

}


int guac_protocol_send_clip(guac_socket* socket, const guac_layer* layer) {

    return
           guac_socket_write_string(socket, "4.clip,")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ";");

}


int guac_protocol_send_clipboard(guac_socket* socket, const char* data) {

    return
           guac_socket_write_string(socket, "9.clipboard,")
        || __guac_socket_write_length_string(socket, data)
        || guac_socket_write_string(socket, ";");

}


int guac_protocol_send_copy(guac_socket* socket,
        const guac_layer* srcl, int srcx, int srcy, int w, int h,
        guac_composite_mode mode, const guac_layer* dstl, int dstx, int dsty) {

    return
           guac_socket_write_string(socket, "4.copy,")
        || __guac_socket_write_length_int(socket, srcl->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, srcx)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, srcy)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, w)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, h)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, mode)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, dstl->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, dstx)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, dsty)
        || guac_socket_write_string(socket, ";");

}


int guac_protocol_send_cstroke(guac_socket* socket,
        guac_composite_mode mode, const guac_layer* layer,
        guac_line_cap_style cap, guac_line_join_style join, int thickness,
        int r, int g, int b, int a) {

    return
           guac_socket_write_string(socket, "7.cstroke,")
        || __guac_socket_write_length_int(socket, mode)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, cap)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, join)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, thickness)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, r)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, g)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, b)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, a)
        || guac_socket_write_string(socket, ";");

}


int guac_protocol_send_cursor(guac_socket* socket, int x, int y,
        const guac_layer* srcl, int srcx, int srcy, int w, int h) {
    return
           guac_socket_write_string(socket, "6.cursor,")
        || __guac_socket_write_length_int(socket, x)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, y)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, srcl->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, srcx)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, srcy)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, w)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, h)
        || guac_socket_write_string(socket, ";");

}


int guac_protocol_send_curve(guac_socket* socket, const guac_layer* layer,
        int cp1x, int cp1y, int cp2x, int cp2y, int x, int y) {

    return
           guac_socket_write_string(socket, "5.curve,")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, cp1x)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, cp1y)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, cp2x)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, cp2y)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, x)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, y)
        || guac_socket_write_string(socket, ";");

}


int guac_protocol_send_data(guac_socket* socket, guac_resource* resource,
        const unsigned char* data, size_t size) {

    /* Calculate base64 length */
    int base64_length = (size + 2) / 3 * 4;

    /* Send base64-encoded data */
    return
           guac_socket_write_string(socket, "4.data,")
        || __guac_socket_write_length_int(socket, resource->index)
        || guac_socket_write_string(socket, ",")
        || guac_socket_write_int(socket, base64_length)
        || guac_socket_write_string(socket, ".")
        || guac_socket_write_base64(socket, data, size)
        || guac_socket_flush_base64(socket)
        || guac_socket_write_string(socket, ";");

}


int guac_protocol_send_disconnect(guac_socket* socket) {
    return guac_socket_write_string(socket, "10.disconnect;");
}


int guac_protocol_send_dispose(guac_socket* socket, const guac_layer* layer) {

    return
           guac_socket_write_string(socket, "7.dispose,")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ";");

}


int guac_protocol_send_distort(guac_socket* socket, const guac_layer* layer,
        double a, double b, double c,
        double d, double e, double f) {

    return 
           guac_socket_write_string(socket, "7.distort,")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_double(socket, a)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_double(socket, b)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_double(socket, c)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_double(socket, d)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_double(socket, e)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_double(socket, f)
        || guac_socket_write_string(socket, ";");

}


int guac_protocol_send_end(guac_socket* socket, guac_resource* resource) {

    return
           guac_socket_write_string(socket, "3.end,")
        || __guac_socket_write_length_int(socket, resource->index)
        || guac_socket_write_string(socket, ";");
}


int guac_protocol_send_error(guac_socket* socket, const char* error) {

    return
           guac_socket_write_string(socket, "5.error,")
        || __guac_socket_write_length_string(socket, error)
        || guac_socket_write_string(socket, ";");
}


int guac_protocol_send_identity(guac_socket* socket, const guac_layer* layer) {

    return
           guac_socket_write_string(socket, "8.identity,")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ";");

}


int guac_protocol_send_lfill(guac_socket* socket,
        guac_composite_mode mode, const guac_layer* layer,
        const guac_layer* srcl) {

    return
           guac_socket_write_string(socket, "5.lfill,")
        || __guac_socket_write_length_int(socket, mode)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, srcl->index)
        || guac_socket_write_string(socket, ";");

}


int guac_protocol_send_line(guac_socket* socket, const guac_layer* layer,
        int x, int y) {

    return
           guac_socket_write_string(socket, "4.line,")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, x)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, y)
        || guac_socket_write_string(socket, ";");

}


int guac_protocol_send_lstroke(guac_socket* socket,
        guac_composite_mode mode, const guac_layer* layer,
        guac_line_cap_style cap, guac_line_join_style join, int thickness,
        const guac_layer* srcl) {

    return
           guac_socket_write_string(socket, "7.lstroke,")
        || __guac_socket_write_length_int(socket, mode)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, cap)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, join)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, thickness)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, srcl->index)
        || guac_socket_write_string(socket, ";");

}


int guac_protocol_send_move(guac_socket* socket, const guac_layer* layer,
        const guac_layer* parent, int x, int y, int z) {

    return
           guac_socket_write_string(socket, "4.move,")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, parent->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, x)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, y)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, z)
        || guac_socket_write_string(socket, ";");

}


int guac_protocol_send_name(guac_socket* socket, const char* name) {

    return
           guac_socket_write_string(socket, "4.name,")
        || __guac_socket_write_length_string(socket, name)
        || guac_socket_write_string(socket, ";");

}


int guac_protocol_send_png(guac_socket* socket, guac_composite_mode mode,
        const guac_layer* layer, int x, int y, cairo_surface_t* surface) {

    return
           guac_socket_write_string(socket, "3.png,")
        || __guac_socket_write_length_int(socket, mode)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, x)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, y)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_png(socket, surface)
        || guac_socket_write_string(socket, ";");

}


int guac_protocol_send_pop(guac_socket* socket, const guac_layer* layer) {

    return
           guac_socket_write_string(socket, "3.pop,")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ";");

}


int guac_protocol_send_push(guac_socket* socket, const guac_layer* layer) {

    return
           guac_socket_write_string(socket, "4.push,")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ";");

}


int guac_protocol_send_rect(guac_socket* socket,
        const guac_layer* layer, int x, int y, int width, int height) {

    return
           guac_socket_write_string(socket, "4.rect,")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, x)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, y)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, width)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, height)
        || guac_socket_write_string(socket, ";");

}


int guac_protocol_send_reset(guac_socket* socket, const guac_layer* layer) {

    return
           guac_socket_write_string(socket, "5.reset,")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ";");

}


int guac_protocol_send_resource(guac_socket* socket, guac_resource* resource,
        const char* uri, const char** mimetypes) {

    /* Write resource header */
    if (
           guac_socket_write_string(socket, "8.resource,")
        || __guac_socket_write_length_int(socket, resource->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_string(socket, uri)
       )
        return -1;

    /* Write each mimetype */
    while (*mimetypes != NULL) {

        /* Write mimetype */
        if (  
               guac_socket_write_string(socket, ",")
            || __guac_socket_write_length_string(socket, *mimetypes)
           )
            return -1;

        /* Next mimetype */
        mimetypes++;

    }

    /* Finish instruction */
    return  guac_socket_write_string(socket, ";");

}


int guac_protocol_send_set(guac_socket* socket, const guac_layer* layer,
        const char* name, const char* value) {

    return
           guac_socket_write_string(socket, "3.set,")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_string(socket, name)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_string(socket, value)
        || guac_socket_write_string(socket, ";");

}


int guac_protocol_send_select(guac_socket* socket, const char* protocol) {

    return
           guac_socket_write_string(socket, "6.select,")
        || __guac_socket_write_length_string(socket, protocol)
        || guac_socket_write_string(socket, ";");

}


int guac_protocol_send_shade(guac_socket* socket, const guac_layer* layer,
        int a) {

    return
           guac_socket_write_string(socket, "5.shade,")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, a)
        || guac_socket_write_string(socket, ";");

}


int guac_protocol_send_size(guac_socket* socket, const guac_layer* layer,
        int w, int h) {

    return
           guac_socket_write_string(socket, "4.size,")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, w)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, h)
        || guac_socket_write_string(socket, ";");

}


int guac_protocol_send_start(guac_socket* socket, const guac_layer* layer,
        int x, int y) {

    return
           guac_socket_write_string(socket, "5.start,")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, x)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, y)
        || guac_socket_write_string(socket, ";");

}


int guac_protocol_send_sync(guac_socket* socket, guac_timestamp timestamp) {

    return 
           guac_socket_write_string(socket, "4.sync,")
        || __guac_socket_write_length_int(socket, timestamp)
        || guac_socket_write_string(socket, ";");

}


int guac_protocol_send_transfer(guac_socket* socket,
        const guac_layer* srcl, int srcx, int srcy, int w, int h,
        guac_transfer_function fn, const guac_layer* dstl, int dstx, int dsty) {

    return
           guac_socket_write_string(socket, "8.transfer,")
        || __guac_socket_write_length_int(socket, srcl->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, srcx)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, srcy)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, w)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, h)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, fn)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, dstl->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, dstx)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, dsty)
        || guac_socket_write_string(socket, ";");

}


int guac_protocol_send_transform(guac_socket* socket, const guac_layer* layer,
        double a, double b, double c,
        double d, double e, double f) {

    return 
           guac_socket_write_string(socket, "9.transform,")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_double(socket, a)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_double(socket, b)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_double(socket, c)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_double(socket, d)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_double(socket, e)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_double(socket, f)
        || guac_socket_write_string(socket, ";");

}


