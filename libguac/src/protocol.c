
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
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <png.h>

#include <sys/types.h>
#include <sys/socket.h>

#include "guacio.h"
#include "protocol.h"

char* guac_escape_string(const char* str) {

    char* escaped;
    char* current;

    int i;
    int length = 0;

    /* Determine length */
    for (i=0; str[i] != '\0'; i++) {
        switch (str[i]) {

            case ';':
            case ',':
            case '\\':
                length += 2;
                break;

            default:
                length++;

        }
    }

    /* Allocate new */
    escaped = malloc(length+1);

    current = escaped;
    for (i=0; str[i] != '\0'; i++) {
        switch (str[i]) {

            case ';':
                *(current++) = '\\';
                *(current++) = 's';
                break;

            case ',':
                *(current++) = '\\';
                *(current++) = 'c';
                break;

            case '\\':
                *(current++) = '\\';
                *(current++) = '\\';
                break;

            default:
                *(current++) = str[i];
        }

    }

    *current = '\0';

    return escaped;

}

char* guac_unescape_string_inplace(char* str) {

    char* from;
    char* to;

    from = to = str;
    for (;;) {

        char c = *(from++);

        if (c == '\\') {

            c = *(from++);
            if (c == 's')
                *(to++) = ';';

            else if (c == 'c')
                *(to++) = ',';

            else if (c == '\\')
                *(to++) = '\\';

            else if (c == '\0') {
                *(to++) = '\\';
                break;
            }

            else {
                *(to++) = '\\';
                *(to++) = c;
            }
        }

        else if (c == '\0')
            break;

        else
            *(to++) = c;

    }

    *to = '\0';

    return str;

}

void guac_send_name(GUACIO* io, const char* name) {

    char* escaped = guac_escape_string(name); 

    guac_write_string(io, "name:");
    guac_write_string(io, escaped);
    guac_write_string(io, ";");

    free(escaped);

}

void guac_send_size(GUACIO* io, int w, int h) {
    guac_write_string(io, "size:");
    guac_write_int(io, w);
    guac_write_string(io, ",");
    guac_write_int(io, h);
    guac_write_string(io, ";");
}

void guac_send_clipboard(GUACIO* io, const char* data) {

    char* escaped = guac_escape_string(data); 

    guac_write_string(io, "clipboard:");
    guac_write_string(io, escaped);
    guac_write_string(io, ";");

    free(escaped);

}

void guac_send_error(GUACIO* io, const char* error) {

    char* escaped = guac_escape_string(error); 

    guac_write_string(io, "error:");
    guac_write_string(io, escaped);
    guac_write_string(io, ";");

    free(escaped);

}

void guac_send_copy(GUACIO* io, int srcx, int srcy, int w, int h, int dstx, int dsty) {
    guac_write_string(io, "copy:");
    guac_write_int(io, srcx);
    guac_write_string(io, ",");
    guac_write_int(io, srcy);
    guac_write_string(io, ",");
    guac_write_int(io, w);
    guac_write_string(io, ",");
    guac_write_int(io, h);
    guac_write_string(io, ",");
    guac_write_int(io, dstx);
    guac_write_string(io, ",");
    guac_write_int(io, dsty);
    guac_write_string(io, ";");
}

void __guac_write_png(png_structp png, png_bytep data, png_size_t length) {

    if (guac_write_base64((GUACIO*) png->io_ptr, data, length) < 0) {
        perror("Error writing PNG");
        png_error(png, "Error writing PNG");
        return;
    }

}

void __guac_write_flush(png_structp png) {
}

void guac_send_png(GUACIO* io, int x, int y, png_byte** png_rows, int w, int h) {

    png_structp png;
    png_infop png_info;

    /* Write image */

    /* Set up PNG writer */
    png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) {
        perror("Error initializing libpng write structure");
        return;
    }

    png_info = png_create_info_struct(png);
    if (!png_info) {
        perror("Error initializing libpng info structure");
        png_destroy_write_struct(&png, NULL);
        return;
    }

    /* Set error handler */
    if (setjmp(png_jmpbuf(png))) {
        perror("Error setting handler");
        png_destroy_write_struct(&png, &png_info);
        return;
    }

    png_set_write_fn(png, io, __guac_write_png, __guac_write_flush);

    /* Set PNG IHDR */
    png_set_IHDR(
            png,
            png_info,
            w,
            h,
            8,
            PNG_COLOR_TYPE_RGB,
            PNG_INTERLACE_NONE,
            PNG_COMPRESSION_TYPE_DEFAULT,
            PNG_FILTER_TYPE_DEFAULT
    );
    
    guac_write_string(io, "png:");
    guac_write_int(io, x);
    guac_write_string(io, ",");
    guac_write_int(io, y);
    guac_write_string(io, ",");
    png_set_rows(png, png_info, png_rows);
    png_write_png(png, png_info, PNG_TRANSFORM_IDENTITY, NULL);

    if (guac_flush_base64(io) < 0) {
        perror("Error flushing PNG");
        png_error(png, "Error flushing PNG");
        return;
    }

    png_destroy_write_struct(&png, &png_info);

    guac_write_string(io, ";");

}


void guac_send_cursor(GUACIO* io, int x, int y, png_byte** png_rows, int w, int h) {

    png_structp png;
    png_infop png_info;

    /* Write image */

    /* Set up PNG writer */
    png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) {
        perror("Error initializing libpng write structure");
        return;
    }

    png_info = png_create_info_struct(png);
    if (!png_info) {
        perror("Error initializing libpng info structure");
        png_destroy_write_struct(&png, NULL);
        return;
    }

    /* Set error handler */
    if (setjmp(png_jmpbuf(png))) {
        perror("Error setting handler");
        png_destroy_write_struct(&png, &png_info);
        return;
    }

    png_set_write_fn(png, io, __guac_write_png, __guac_write_flush);

    /* Set PNG IHDR */
    png_set_IHDR(
            png,
            png_info,
            w,
            h,
            8,
            PNG_COLOR_TYPE_RGBA,
            PNG_INTERLACE_NONE,
            PNG_COMPRESSION_TYPE_DEFAULT,
            PNG_FILTER_TYPE_DEFAULT
    );
    
    guac_write_string(io, "cursor:");
    guac_write_int(io, x);
    guac_write_string(io, ",");
    guac_write_int(io, y);
    guac_write_string(io, ",");
    png_set_rows(png, png_info, png_rows);
    png_write_png(png, png_info, PNG_TRANSFORM_IDENTITY, NULL);

    if (guac_flush_base64(io) < 0) {
        perror("Error flushing PNG");
        png_error(png, "Error flushing PNG");
        return;
    }

    png_destroy_write_struct(&png, &png_info);

    guac_write_string(io, ";");

}


int __guac_fill_instructionbuf(GUACIO* io) {

    int retval;
    
    /* Attempt to fill buffer */
    retval = recv(
            io->fd,
            io->instructionbuf + io->instructionbuf_used_length,
            io->instructionbuf_size - io->instructionbuf_used_length,
            0
    );

    if (retval < 0)
        return retval;

    io->instructionbuf_used_length += retval;

    /* Expand buffer if necessary */
    if (io->instructionbuf_used_length > io->instructionbuf_size / 2) {
        io->instructionbuf_size *= 2;
        io->instructionbuf = realloc(io->instructionbuf, io->instructionbuf_size);
    }

    return retval;

}

/* Returns new instruction if one exists, or NULL if no more instructions. */
int guac_read_instruction(GUACIO* io, guac_instruction* parsed_instruction) {

    int retval;
    int i = 0;
    int argcount = 0;
    int j;
    int current_arg = 0;
    
    /* Loop until a instruction is read */
    for (;;) {

        /* Search for end of instruction */
        for (; i < io->instructionbuf_used_length; i++) {

            /* Count arguments as we look for the end */
            if (io->instructionbuf[i] == ',')
                argcount++;
            else if (io->instructionbuf[i] == ':' && argcount == 0)
                argcount++;

            /* End found ... */
            else if (io->instructionbuf[i] == ';') {

                /* Parse new instruction */
                char* instruction = malloc(i+1);
                memcpy(instruction, io->instructionbuf, i+1);
                instruction[i] = '\0'; /* Replace semicolon with null terminator. */

                parsed_instruction->opcode = NULL;

                parsed_instruction->argc = argcount;
                parsed_instruction->argv = malloc(sizeof(char*) * argcount);

                for (j=0; j<i; j++) {

                    /* If encountered a colon, and no opcode parsed yet, set opcode and following argument */
                    if (instruction[j] == ':' && parsed_instruction->opcode == NULL) {
                        instruction[j] = '\0';
                        parsed_instruction->argv[current_arg++] = &(instruction[j+1]);
                        parsed_instruction->opcode = instruction;
                    }

                    /* If encountered a comma, set following argument */
                    else if (instruction[j] == ',') {
                        instruction[j] = '\0';
                        parsed_instruction->argv[current_arg++] = &(instruction[j+1]);
                    }
                }

                /* If no arguments, set opcode to entire instruction */
                if (parsed_instruction->opcode == NULL)
                    parsed_instruction->opcode = instruction;

                /* Found. Reset buffer */
                memmove(io->instructionbuf, io->instructionbuf + i + 1, io->instructionbuf_used_length - i - 1);
                io->instructionbuf_used_length -= i + 1;

                /* Done */
                return 1;
            }

        }

        /* No instruction yet? Get more data ... */
        retval = guac_select(io, 1000);
        if (retval <= 0)
            return retval;

        retval = __guac_fill_instructionbuf(io);
        if (retval < 0)
            return retval;

        if (retval == 0)
            return -1; /* EOF */
    }

}

void guac_free_instruction_data(guac_instruction* instruction) {
    free(instruction->opcode);

    if (instruction->argv)
        free(instruction->argv);
}

void guac_free_instruction(guac_instruction* instruction) {
    guac_free_instruction_data(instruction);
    free(instruction);
}


int guac_instructions_waiting(GUACIO* io) {

    if (io->instructionbuf_used_length > 0)
        return 1;

    return guac_select(io, 1000);
}

