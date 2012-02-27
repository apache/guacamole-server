
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

#ifndef _GUAC_PROTOCOL_H
#define _GUAC_PROTOCOL_H

#include <cairo/cairo.h>

#include "socket.h"

/**
 * Provides functions and structures required for communicating using the
 * Guacamole protocol over a guac_socket connection, such as that provided by
 * guac_client objects.
 *
 * @file protocol.h
 */


/**
 * An arbitrary timestamp denoting a relative time value in milliseconds.
 */
typedef int64_t guac_timestamp;

/**
 * Composite modes used by Guacamole draw instructions. Each
 * composite mode maps to a unique channel mask integer.
 */
typedef enum guac_composite_mode {

    /*
     * A: Source where destination transparent = S n D'
     * B: Source where destination opaque      = S n D
     * C: Destination where source transparent = D n S'
     * D: Destination where source opaque      = D n S
     *
     * 0 = Active, 1 = Inactive
     */
                           /* ABCD */
    GUAC_COMP_ROUT  = 0x2, /* 0010 - Clears destination where source opaque  */
    GUAC_COMP_ATOP  = 0x6, /* 0110 - Fill where destination opaque only      */
    GUAC_COMP_XOR   = 0xA, /* 1010 - XOR                                     */
    GUAC_COMP_ROVER = 0xB, /* 1011 - Fill where destination transparent only */
    GUAC_COMP_OVER  = 0xE, /* 1110 - Draw normally                           */
    GUAC_COMP_PLUS  = 0xF, /* 1111 - Add                                     */

    /* Unimplemented in client: */
    /* NOT IMPLEMENTED:       0000 - Clear          */
    /* NOT IMPLEMENTED:       0011 - No operation   */
    /* NOT IMPLEMENTED:       0101 - Additive IN    */
    /* NOT IMPLEMENTED:       0111 - Additive ATOP  */
    /* NOT IMPLEMENTED:       1101 - Additive RATOP */

    /* Buggy in webkit browsers, as they keep channel C on in all cases: */
    GUAC_COMP_RIN   = 0x1, /* 0001 */
    GUAC_COMP_IN    = 0x4, /* 0100 */
    GUAC_COMP_OUT   = 0x8, /* 1000 */
    GUAC_COMP_RATOP = 0x9, /* 1001 */
    GUAC_COMP_SRC   = 0xC, /* 1100 */

    /* Bitwise composite operations (binary) */

    /*
     * A: S' & D'
     * B: S' & D
     * C: S  & D'
     * D: S  & D
     *
     * 0 = Active, 1 = Inactive
     */

    /* Constant functions */            /* ABCD */
    GUAC_COMP_BINARY_BLACK      = 0x10, /* 0000 */
    GUAC_COMP_BINARY_WHITE      = 0x1F, /* 1111 */

    /* Copy functions */
    GUAC_COMP_BINARY_SRC        = 0x13, /* 0011 */
    GUAC_COMP_BINARY_DEST       = 0x15, /* 0101 */
    GUAC_COMP_BINARY_NSRC       = 0x1C, /* 1100 */
    GUAC_COMP_BINARY_NDEST      = 0x1A, /* 1010 */

    /* AND / NAND */
    GUAC_COMP_BINARY_AND        = 0x11, /* 0001 */
    GUAC_COMP_BINARY_NAND       = 0x1E, /* 1110 */

    /* OR / NOR */
    GUAC_COMP_BINARY_OR         = 0x17, /* 0111 */
    GUAC_COMP_BINARY_NOR        = 0x18, /* 1000 */

    /* XOR / XNOR */
    GUAC_COMP_BINARY_XOR        = 0x16, /* 0110 */
    GUAC_COMP_BINARY_XNOR       = 0x19, /* 1001 */

    /* AND / NAND with inverted source */
    GUAC_COMP_BINARY_NSRC_AND   = 0x14, /* 0100 */
    GUAC_COMP_BINARY_NSRC_NAND  = 0x1B, /* 1011 */

    /* OR / NOR with inverted source */
    GUAC_COMP_BINARY_NSRC_OR    = 0x1D, /* 1101 */
    GUAC_COMP_BINARY_NSRC_NOR   = 0x12, /* 0010 */

    /* AND / NAND with inverted destination */
    GUAC_COMP_BINARY_NDEST_AND  = 0x12, /* 0010 */
    GUAC_COMP_BINARY_NDEST_NAND = 0x1D, /* 1101 */

    /* OR / NOR with inverted destination */
    GUAC_COMP_BINARY_NDEST_OR   = 0x1B, /* 1011 */
    GUAC_COMP_BINARY_NDEST_NOR  = 0x14  /* 0100 */

} guac_composite_mode;

typedef struct guac_layer guac_layer;

/**
 * Represents a single layer within the Guacamole protocol.
 */
struct guac_layer {

    /**
     * The index of this layer.
     */
    int index;

    /**
     * The next allocated layer in the list of all layers.
     */
    guac_layer* __next;

    /**
     * The next available (unused) layer in the list of
     * allocated but free'd layers.
     */
    guac_layer* __next_available;

};

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
     * Array of all arguments passed to this instruction.
     */
    char** argv;

} guac_instruction;


/**
 * Frees all memory allocated to the given instruction.
 *
 * @param instruction The instruction to free.
 */
void guac_instruction_free(guac_instruction* instruction);

/**
 * Sends an args instruction over the given guac_socket connection.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param args The NULL-terminated array of argument names (strings).
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_args(guac_socket* socket, const char** name);

/**
 * Sends a name instruction over the given guac_socket connection.
 *
 * @param socket The guac_socket connection to use.
 * @param name The name to send within the name instruction.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_name(guac_socket* socket, const char* name);

/**
 * Sends a sync instruction over the given guac_socket connection. The
 * current time in milliseconds should be passed in as the timestamp.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param timestamp The current timestamp (in milliseconds).
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_sync(guac_socket* socket, guac_timestamp timestamp);

/**
 * Sends an error instruction over the given guac_socket connection.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param error The description associated with the error.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_error(guac_socket* socket, const char* error);

/**
 * Sends a clipboard instruction over the given guac_socket connection.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param data The clipboard data to send.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_clipboard(guac_socket* socket, const char* data);

/**
 * Sends a size instruction over the given guac_socket connection.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param layer The layer to resize.
 * @param w The new width of the layer.
 * @param h The new height of the layer.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_size(guac_socket* socket, const guac_layer* layer,
        int w, int h);

/**
 * Sends a move instruction over the given guac_socket connection.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param layer The layer to move.
 * @param parent The parent layer the specified layer will be positioned
 *               relative to.
 * @param x The X coordinate of the layer.
 * @param y The Y coordinate of the layer.
 * @param z The Z index of the layer, relative to other layers in its parent.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_move(guac_socket* socket, const guac_layer* layer,
        const guac_layer* parent, int x, int y, int z);

/**
 * Sends a dispose instruction over the given guac_socket connection.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param layer The layer to dispose.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_dispose(guac_socket* socket, const guac_layer* layer);

/**
 * Sends a copy instruction over the given guac_socket connection.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param srcl The source layer.
 * @param srcx The X coordinate of the source rectangle.
 * @param srcy The Y coordinate of the source rectangle.
 * @param w The width of the source rectangle.
 * @param h The height of the source rectangle.
 * @param mode The composite mode to use.
 * @param dstl The destination layer.
 * @param dstx The X coordinate of the destination, where the source rectangle
 *             should be copied.
 * @param dsty The Y coordinate of the destination, where the source rectangle
 *             should be copied.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_copy(guac_socket* socket, 
        const guac_layer* srcl, int srcx, int srcy, int w, int h,
        guac_composite_mode mode, const guac_layer* dstl, int dstx, int dsty);

/**
 * Sends a rect instruction over the given guac_socket connection.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param mode The composite mode to use.
 * @param layer The destination layer.
 * @param x The X coordinate of the rectangle.
 * @param y The Y coordinate of the rectangle.
 * @param width The width of the rectangle.
 * @param height The height of the rectangle.
 * @param r The red component of the color of the rectangle.
 * @param g The green component of the color of the rectangle.
 * @param b The blue component of the color of the rectangle.
 * @param a The alpha (transparency) component of the color of the rectangle.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_rect(guac_socket* socket,
        guac_composite_mode mode, const guac_layer* layer,
        int x, int y, int width, int height,
        int r, int g, int b, int a);

/**
 * Sends a clip instruction over the given guac_socket connection.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param layer The layer to set the clipping region of.
 * @param x The X coordinate of the clipping rectangle.
 * @param y The Y coordinate of the clipping rectangle.
 * @param width The width of the clipping rectangle.
 * @param height The height of the clipping rectangle.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_clip(guac_socket* socket, const guac_layer* layer,
        int x, int y, int width, int height);

/**
 * Sends a png instruction over the given guac_socket connection. The PNG image data
 * given will be automatically base64-encoded for transmission.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param mode The composite mode to use.
 * @param layer The destination layer.
 * @param x The destination X coordinate.
 * @param y The destination Y coordinate.
 * @param surface A cairo surface containing the image data to send.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_png(guac_socket* socket, guac_composite_mode mode,
        const guac_layer* layer, int x, int y, cairo_surface_t* surface);

/**
 * Sends a cursor instruction over the given guac_socket connection. The PNG image
 * data given will be automatically base64-encoded for transmission.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param x The X coordinate of the cursor hotspot.
 * @param y The Y coordinate of the cursor hotspot.
 * @param srcl The source layer.
 * @param srcx The X coordinate of the source rectangle.
 * @param srcy The Y coordinate of the source rectangle.
 * @param w The width of the source rectangle.
 * @param h The height of the source rectangle.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_cursor(guac_socket* socket, int x, int y,
        const guac_layer* srcl, int srcx, int srcy, int w, int h);

/**
 * Returns whether new instruction data is available on the given guac_socket
 * connection for parsing.
 *
 * @param socket The guac_socket connection to use.
 * @param usec_timeout The maximum number of microseconds to wait before
 *                     giving up.
 * @return A positive value if data is available, negative on error, or
 *         zero if no data is currently available.
 */
int guac_protocol_instructions_waiting(guac_socket* socket, int usec_timeout);

/**
 * Reads a single instruction from the given guac_socket connection.
 *
 * If an error occurs reading the instruction, NULL is returned,
 * and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param usec_timeout The maximum number of microseconds to wait before
 *                     giving up.
 * @return A new instruction if data was successfully read, NULL on
 *         error or if the instruction could not be read completely
 *         because the timeout elapsed, in which case guac_error will be
 *         set to GUAC_STATUS_INPUT_TIMEOUT and subsequent calls to
 *         guac_protocol_read_instruction() will return the parsed instruction once
 *         enough data is available.
 */
guac_instruction* guac_protocol_read_instruction(guac_socket* socket, int usec_timeout);

/**
 * Reads a single instruction with the given opcode from the given guac_socket
 * connection.
 *
 * If an error occurs reading the instruction, NULL is returned,
 * and guac_error is set appropriately.
 *
 * If the instruction read is not the expected instruction, NULL is returned,
 * and guac_error is set to GUAC_STATUS_BAD_STATE.
 *
 * @param socket The guac_socket connection to use.
 * @param usec_timeout The maximum number of microseconds to wait before
 *                     giving up.
 * @param opcode The opcode of the instruction to read.
 * @return A new instruction if an instruction with the given opcode was read,
 *         NULL otherwise. If an instruction was read, but the instruction had
 *         a different opcode, NULL is returned and guac_error is set to
 *         GUAC_STATUS_BAD_STATE.
 */
guac_instruction* guac_protocol_expect_instruction(guac_socket* socket, int usec_timeout,
        const char* opcode);

/**
 * Returns an arbitrary timestamp. The difference between return values of any
 * two calls is equal to the amount of time in milliseconds between those 
 * calls. The return value from a single call will not have any useful
 * (or defined) meaning.
 *
 * @return An arbitrary millisecond timestamp.
 */
guac_timestamp guac_protocol_get_timestamp();

#endif

