
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

#ifndef __PROTOCOL_H
#define __PROTOCOL_H

#include <png.h>

#include "guacio.h"

/**
 * Provides functions and structures required for communicating using the
 * Guacamole protocol over a GUACIO connection, such as that provided by
 * guac_client objects.
 *
 * @file protocol.h
 */


/**
 * Represents a single instruction within the Guacamole protocol.
 */
typedef struct guac_instruction {

    /**
     * The opcode of the instruction.
     */
    char* opcode;

    /**
     * The number of arguments passed to this instruction.
     */
    int argc;

    /**
     * Array of all arguments passed to this instruction. Strings
     * are not already unescaped.
     */
    char** argv;

} guac_instruction;


/**
 * Frees all memory allocated to the given instruction opcode
 * and arguments. The instruction structure itself will not
 * be freed.
 *
 * @param instruction The instruction to free.
 */
void guac_free_instruction_data(guac_instruction* instruction);


/**
 * Frees all memory allocated to the given instruction. This
 * includes freeing memory allocated for the structure
 * itself.
 *
 * @param instruction The instruction to free.
 */
void guac_free_instruction(guac_instruction* instruction);

/**
 * Escapes the given string as necessary to be passed within
 * a Guacamole instruction. The returned string must later be
 * released with a call to free().
 *
 * @param str The string to escape.
 * @return A new escaped string, which must be freed with free().
 */
char* guac_escape_string(const char* str);

/**
 * Unescapes the given string in-place, as an unescaped string
 * is always the same length or shorter than the original.
 *
 * @param str The string to unescape.
 * @return A pointer to the original string, which is now unescaped.
 */
char* guac_unescape_string_inplace(char* str);

/**
 * Sends a name instruction over the given GUACIO connection. The
 * name given will be automatically escaped for transmission.
 *
 * @param io The GUACIO connection to use.
 * @param name The name to send within the name instruction.
 */
void guac_send_name(GUACIO* io, const char* name);

/**
 * Sends an error instruction over the given GUACIO connection. The
 * error description given will be automatically escaped for
 * transmission.
 *
 * @param io The GUACIO connection to use.
 * @param error The description associated with the error.
 */
void guac_send_error(GUACIO* io, const char* error);

/**
 * Sends a clipboard instruction over the given GUACIO connection. The
 * clipboard data given will be automatically escaped for transmission.
 *
 * @param io The GUACIO connection to use.
 * @param data The clipboard data to send.
 */
void guac_send_clipboard(GUACIO* io, const char* data);

/**
 * Sends a size instruction over the given GUACIO connection.
 *
 * @param io The GUACIO connection to use.
 * @param w The width of the display.
 * @param h The height of the display.
 */
void guac_send_size(GUACIO* io, int w, int h);

/**
 * Sends a copy instruction over the given GUACIO connection.
 *
 * @param io The GUACIO connection to use.
 * @param srcx The X coordinate of the source rectangle.
 * @param srcy The Y coordinate of the source rectangle.
 * @param w The width of the source rectangle.
 * @param h The height of the source rectangle.
 * @param dstx The X coordinate of the destination, where the source rectangle
 *             should be copied.
 * @param dsty The Y coordinate of the destination, where the source rectangle
 *             should be copied.
 */
void guac_send_copy(GUACIO* io, int srcx, int srcy, int w, int h, int dstx, int dsty);

/**
 * Sends a png instruction over the given GUACIO connection. The PNG image data
 * given will be automatically base64-encoded for transmission.
 *
 * @param io The GUACIO connection to use.
 * @param x The destination X coordinate.
 * @param y The destination Y coordinate.
 * @param png_rows A libpng-compatible PNG image buffer containing the image
 *                 data to send.
 * @param w The width of the image in the image buffer.
 * @param h The height of the image in the image buffer.
 */
void guac_send_png(GUACIO* io, int x, int y, png_byte** png_rows, int w, int h);

/**
 * Sends a cursor instruction over the given GUACIO connection. The PNG image
 * data given will be automatically base64-encoded for transmission.
 *
 * @param io The GUACIO connection to use.
 * @param x The destination X coordinate.
 * @param y The destination Y coordinate.
 * @param png_rows A libpng-compatible PNG image buffer containing the image
 *                 data to send.
 * @param w The width of the image in the image buffer.
 * @param h The height of the image in the image buffer.
 */
void guac_send_cursor(GUACIO* io, int x, int y, png_byte** png_rows, int w, int h);

/**
 * Returns whether new instruction data is available on the given GUACIO
 * connection for parsing.
 *
 * @param io The GUACIO connection to use.
 * @return A positive value if data is available, negative on error, or
 *         zero if no data is currently available.
 */
int guac_instructions_waiting(GUACIO* io);

/**
 * Reads a single instruction from the given GUACIO connection.
 *
 * @param io The GUACIO connection to use.
 * @param parsed_instruction A pointer to a guac_instruction structure which
 *                           will be populated with data read from the given
 *                           GUACIO connection.
 * @return A positive value if data was successfully read, negative on
 *         error, or zero if the instrucion could not be read completely,
 *         in which case, subsequent calls to guac_read_instruction() will
 *         return the parsed instruction once enough data is available.
 */
int guac_read_instruction(GUACIO* io, guac_instruction* parsed_instruction);

#endif

