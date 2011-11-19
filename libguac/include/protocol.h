
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

#include "guacio.h"

/**
 * Provides functions and structures required for communicating using the
 * Guacamole protocol over a GUACIO connection, such as that provided by
 * guac_client objects.
 *
 * @file protocol.h
 */


/**
 * The number of milliseconds to wait for messages in any phase before
 * timing out and closing the connection with an error.
 */
#define GUAC_TIMEOUT      15000

/**
 * The number of microseconds to wait for messages in any phase before
 * timing out and closing the conncetion with an error. This is always
 * equal to GUAC_TIMEOUT * 1000.
 */
#define GUAC_USEC_TIMEOUT (GUAC_TIMEOUT*1000)

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
    GUAC_COMP_SRC   = 0xC  /* 1100 */

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
    guac_layer* next;

    /**
     * The next available (unused) layer in the list of
     * allocated but free'd layers.
     */
    guac_layer* next_available;

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
 * Sends an args instruction over the given GUACIO connection.
 *
 * @param io The GUACIO connection to use.
 * @param args The NULL-terminated array of argument names (strings).
 * @return GUAC_STATUS_SUCCESS on success, any other status code on error.
 */
guac_status guac_send_args(GUACIO* io, const char** name);

/**
 * Sends a name instruction over the given GUACIO connection.
 *
 * @param io The GUACIO connection to use.
 * @param name The name to send within the name instruction.
 * @return GUAC_STATUS_SUCCESS on success, any other status code on error.
 */
guac_status guac_send_name(GUACIO* io, const char* name);

/**
 * Sends a sync instruction over the given GUACIO connection. The
 * current time in milliseconds should be passed in as the timestamp.
 *
 * @param io The GUACIO connection to use.
 * @param timestamp The current timestamp (in milliseconds).
 * @return GUAC_STATUS_SUCCESS on success, any other status code on error.
 */
guac_status guac_send_sync(GUACIO* io, guac_timestamp timestamp);

/**
 * Sends an error instruction over the given GUACIO connection.
 *
 * @param io The GUACIO connection to use.
 * @param error The description associated with the error.
 * @return GUAC_STATUS_SUCCESS on success, any other status code on error.
 */
guac_status guac_send_error(GUACIO* io, const char* error);

/**
 * Sends a clipboard instruction over the given GUACIO connection.
 *
 * @param io The GUACIO connection to use.
 * @param data The clipboard data to send.
 * @return GUAC_STATUS_SUCCESS on success, any other status code on error.
 */
guac_status guac_send_clipboard(GUACIO* io, const char* data);

/**
 * Sends a size instruction over the given GUACIO connection.
 *
 * @param io The GUACIO connection to use.
 * @param w The width of the display.
 * @param h The height of the display.
 * @return GUAC_STATUS_SUCCESS on success, any other status code on error.
 */
guac_status guac_send_size(GUACIO* io, int w, int h);

/**
 * Sends a copy instruction over the given GUACIO connection.
 *
 * @param io The GUACIO connection to use.
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
 * @return GUAC_STATUS_SUCCESS on success, any other status code on error.
 */
guac_status guac_send_copy(GUACIO* io, 
        const guac_layer* srcl, int srcx, int srcy, int w, int h,
        guac_composite_mode mode, const guac_layer* dstl, int dstx, int dsty);

/**
 * Sends a rect instruction over the given GUACIO connection.
 *
 * @param io The GUACIO connection to use.
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
 * @return GUAC_STATUS_SUCCESS on success, any other status code on error.
 */
guac_status guac_send_rect(GUACIO* io,
        guac_composite_mode mode, const guac_layer* layer,
        int x, int y, int width, int height,
        int r, int g, int b, int a);

/**
 * Sends a clip instruction over the given GUACIO connection.
 *
 * @param io The GUACIO connection to use.
 * @param layer The layer to set the clipping region of.
 * @param x The X coordinate of the clipping rectangle.
 * @param y The Y coordinate of the clipping rectangle.
 * @param width The width of the clipping rectangle.
 * @param height The height of the clipping rectangle.
 * @return GUAC_STATUS_SUCCESS on success, any other status code on error.
 */
guac_status guac_send_clip(GUACIO* io, const guac_layer* layer,
        int x, int y, int width, int height);

/**
 * Sends a png instruction over the given GUACIO connection. The PNG image data
 * given will be automatically base64-encoded for transmission.
 *
 * @param io The GUACIO connection to use.
 * @param mode The composite mode to use.
 * @param layer The destination layer.
 * @param x The destination X coordinate.
 * @param y The destination Y coordinate.
 * @param surface A cairo surface containing the image data to send.
 * @return GUAC_STATUS_SUCCESS on success, any other status code on error.
 */
guac_status guac_send_png(GUACIO* io, guac_composite_mode mode,
        const guac_layer* layer, int x, int y, cairo_surface_t* surface);

/**
 * Sends a cursor instruction over the given GUACIO connection. The PNG image
 * data given will be automatically base64-encoded for transmission.
 *
 * @param io The GUACIO connection to use.
 * @param x The X coordinate of the cursor hotspot.
 * @param y The Y coordinate of the cursor hotspot.
 * @param surface A cairo surface containing the image data to send.
 * @return GUAC_STATUS_SUCCESS on success, any other status code on error.
 */
guac_status guac_send_cursor(GUACIO* io, int x, int y, cairo_surface_t* surface);

/**
 * Returns whether new instruction data is available on the given GUACIO
 * connection for parsing.
 *
 * @param io The GUACIO connection to use.
 * @return GUAC_STATUS_SUCCESS if data is available,
 *         GUAC_STATUS_NO_INPUT if no data is currently available,
 *         or any other status code on error.
 */
guac_status guac_instructions_waiting(GUACIO* io);

/**
 * Reads a single instruction from the given GUACIO connection.
 *
 * @param io The GUACIO connection to use.
 * @param parsed_instruction A pointer to a guac_instruction structure which
 *                           will be populated with data read from the given
 *                           GUACIO connection.
 * @return GUAC_STATUS_SUCCESS if data was successfully read, 
 *         GUAC_STATUS_INPUT_UNAVAILABLE if the instruction could not be read
 *         completely because GUAC_TIMEOUT elapsed, in which case subsequent
 *         calls to guac_read_instruction() will return the parsed instruction
 *         once enough data is available, or any other status code on error.
 */
guac_status guac_read_instruction(GUACIO* io, guac_instruction* parsed_instruction);

guac_timestamp guac_current_timestamp();
void guac_sleep(int millis);

#endif

