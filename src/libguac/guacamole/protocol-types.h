/*
 * Copyright (C) 2014 Glyptodon LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef _GUAC_PROTOCOL_TYPES_H
#define _GUAC_PROTOCOL_TYPES_H

/**
 * Type definitions related to the Guacamole protocol.
 *
 * @file protocol-types.h
 */

/**
 * Set of all possible status codes returned by protocol operations. These
 * codes relate to Guacamole server/client communication, and not to internal
 * communication of errors within libguac and linked software.
 *
 * In general:
 *
 *     0x0000 - 0x00FF: Successful operations.
 *     0x0100 - 0x01FF: Operations that failed due to implementation status.
 *     0x0200 - 0x02FF: Operations that failed due to environmental.
 *     0x0300 - 0x03FF: Operations that failed due to user action.
 *
 * There is a general correspondence of these status codes with HTTP response
 * codes.
 */
typedef enum guac_protocol_status {

    /**
     * The operation succeeded.
     */
    GUAC_PROTOCOL_STATUS_SUCCESS = 0x0000,

    /**
     * The requested operation is unsupported.
     */
    GUAC_PROTOCOL_STATUS_UNSUPPORTED = 0x0100,

    /**
     * The operation could not be performed due to an internal failure.
     */
    GUAC_PROTOCOL_STATUS_SERVER_ERROR = 0x0200,

    /**
     * The operation could not be performed due as the server is busy.
     */
    GUAC_PROTOCOL_STATUS_SERVER_BUSY = 0x0201,

    /**
     * The operation could not be performed because the upstream server
     * is not responding.
     */
    GUAC_PROTOCOL_STATUS_UPSTREAM_TIMEOUT = 0x202,

    /**
     * The operation was unsuccessful due to an error or otherwise
     * unexpected condition of the upstream server.
     */
    GUAC_PROTOCOL_STATUS_UPSTREAM_ERROR = 0x203,

    /**
     * The operation could not be performed as the requested resource
     * does not exist.
     */
    GUAC_PROTOCOL_STATUS_RESOURCE_NOT_FOUND = 0x204,

    /**
     * The operation could not be performed as the requested resource is
     * already in use.
     */
    GUAC_PROTOCOL_STATUS_RESOURCE_CONFLICT = 0x205,

    /**
     * The operation could not be performed because bad parameters were
     * given.
     */
    GUAC_PROTOCOL_STATUS_CLIENT_BAD_REQUEST = 0x300,

    /**
     * Permission was denied to perform the operation, as the user is not
     * yet authorized (not yet logged in, for example).
     */
    GUAC_PROTOCOL_STATUS_CLIENT_UNAUTHORIZED = 0x0301,

    /**
     * Permission was denied to perform the operation, and this permission
     * will not be granted even if the user is authorized.
     */
    GUAC_PROTOCOL_STATUS_CLIENT_FORBIDDEN = 0x0303,

    /**
     * The client took too long to respond.
     */
    GUAC_PROTOCOL_STATUS_CLIENT_TIMEOUT = 0x308,

    /**
     * The client sent too much data.
     */
    GUAC_PROTOCOL_STATUS_CLIENT_OVERRUN = 0x30D,

    /**
     * The client sent data of an unsupported or unexpected type.
     */
    GUAC_PROTOCOL_STATUS_CLIENT_BAD_TYPE = 0x30F,

    /**
     * The operation failed because the current client is already
     * using too many resources.
     */
    GUAC_PROTOCOL_STATUS_CLIENT_TOO_MANY = 0x31D

} guac_protocol_status;

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

    /* Bitwise composite operations (binary) */

    /*
     * A: S' & D'
     * B: S' & D
     * C: S  & D'
     * D: S  & D
     *
     * 0 = Active, 1 = Inactive
     */

} guac_composite_mode;

/**
 * Default transfer functions. There is no current facility in the
 * Guacamole protocol to define custom transfer functions.
 */
typedef enum guac_transfer_function {

    /* Constant functions */               /* ABCD */
    GUAC_TRANSFER_BINARY_BLACK      = 0x0, /* 0000 */
    GUAC_TRANSFER_BINARY_WHITE      = 0xF, /* 1111 */

    /* Copy functions */
    GUAC_TRANSFER_BINARY_SRC        = 0x3, /* 0011 */
    GUAC_TRANSFER_BINARY_DEST       = 0x5, /* 0101 */
    GUAC_TRANSFER_BINARY_NSRC       = 0xC, /* 1100 */
    GUAC_TRANSFER_BINARY_NDEST      = 0xA, /* 1010 */

    /* AND / NAND */
    GUAC_TRANSFER_BINARY_AND        = 0x1, /* 0001 */
    GUAC_TRANSFER_BINARY_NAND       = 0xE, /* 1110 */

    /* OR / NOR */
    GUAC_TRANSFER_BINARY_OR         = 0x7, /* 0111 */
    GUAC_TRANSFER_BINARY_NOR        = 0x8, /* 1000 */

    /* XOR / XNOR */
    GUAC_TRANSFER_BINARY_XOR        = 0x6, /* 0110 */
    GUAC_TRANSFER_BINARY_XNOR       = 0x9, /* 1001 */

    /* AND / NAND with inverted source */
    GUAC_TRANSFER_BINARY_NSRC_AND   = 0x4, /* 0100 */
    GUAC_TRANSFER_BINARY_NSRC_NAND  = 0xB, /* 1011 */

    /* OR / NOR with inverted source */
    GUAC_TRANSFER_BINARY_NSRC_OR    = 0xD, /* 1101 */
    GUAC_TRANSFER_BINARY_NSRC_NOR   = 0x2, /* 0010 */

    /* AND / NAND with inverted destination */
    GUAC_TRANSFER_BINARY_NDEST_AND  = 0x2, /* 0010 */
    GUAC_TRANSFER_BINARY_NDEST_NAND = 0xD, /* 1101 */

    /* OR / NOR with inverted destination */
    GUAC_TRANSFER_BINARY_NDEST_OR   = 0xB, /* 1011 */
    GUAC_TRANSFER_BINARY_NDEST_NOR  = 0x4  /* 0100 */

} guac_transfer_function;

/**
 * Supported line cap styles
 */
typedef enum guac_line_cap_style {
    GUAC_LINE_CAP_BUTT   = 0x0,
    GUAC_LINE_CAP_ROUND  = 0x1,
    GUAC_LINE_CAP_SQUARE = 0x2
} guac_line_cap_style;

/**
 * Supported line join styles
 */
typedef enum guac_line_join_style {
    GUAC_LINE_JOIN_BEVEL = 0x0,
    GUAC_LINE_JOIN_MITER = 0x1,
    GUAC_LINE_JOIN_ROUND = 0x2
} guac_line_join_style;

#endif

