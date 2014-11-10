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

#ifndef _GUAC_ERROR_TYPES_H
#define _GUAC_ERROR_TYPES_H

/**
 * Type definitions related to return values and errors.
 *
 * @file error-types.h
 */

/**
 * Return codes shared by all Guacamole functions which can fail.
 */
typedef enum guac_status {

    /**
     * No errors occurred and the operation was successful.
     */
    GUAC_STATUS_SUCCESS = 0,

    /**
     * Insufficient memory to complete the operation.
     */
    GUAC_STATUS_NO_MEMORY,

    /**
     * The resource associated with the operation can no longer be used as it
     * has reached the end of its normal lifecycle.
     */
    GUAC_STATUS_CLOSED,

    /**
     * Time ran out before the operation could complete.
     */
    GUAC_STATUS_TIMEOUT,

    /**
     * An error occurred, and further information about the error is already
     * stored in errno.
     */
    GUAC_STATUS_SEE_ERRNO,

    /**
     * An I/O error prevented the operation from succeeding.
     */
    GUAC_STATUS_IO_ERROR,

    /**
     * The operation could not be performed because an invalid argument was
     * given.
     */
    GUAC_STATUS_INVALID_ARGUMENT,

    /**
     * The operation failed due to a bug in the software or a serious system
     * problem.
     */
    GUAC_STATUS_INTERNAL_ERROR,

    /**
     * Insufficient space remaining to complete the operation.
     */
    GUAC_STATUS_NO_SPACE,

    /**
     * The operation failed because the input provided is too large.
     */
    GUAC_STATUS_INPUT_TOO_LARGE,

    /**
     * The operation failed because the result could not be stored in the
     * space provided.
     */
    GUAC_STATUS_RESULT_TOO_LARGE,

    /**
     * Permission was denied to perform the operation.
     */
    GUAC_STATUS_PERMISSION_DENIED,

    /**
     * The operation could not be performed because associated resources are
     * busy.
     */
    GUAC_STATUS_BUSY,

    /**
     * The operation could not be performed because, while the associated
     * resources do exist, they are not currently available for use.
     */
    GUAC_STATUS_NOT_AVAILABLE,

    /**
     * The requested operation is not supported.
     */
    GUAC_STATUS_NOT_SUPPORTED,

    /**
     * Support for the requested operation is not yet implemented.
     */
    GUAC_STATUS_NOT_INPLEMENTED,

    /**
     * The operation is temporarily unable to be performed, but may succeed if
     * reattempted.
     */
    GUAC_STATUS_TRY_AGAIN,

    /**
     * A violation of the Guacamole protocol occurred.
     */
    GUAC_STATUS_PROTOCOL_ERROR,

    /**
     * The operation could not be performed because the requested resources do
     * not exist.
     */
    GUAC_STATUS_NOT_FOUND,

    /**
     * The operation was canceled prior to completion.
     */
    GUAC_STATUS_CANCELED,

    /**
     * The operation could not be performed because input values are outside
     * the allowed range.
     */
    GUAC_STATUS_OUT_OF_RANGE,

    /**
     * The operation could not be performed because access to an underlying
     * resource is explicitly not allowed, though not necessarily due to
     * permissions.
     */
    GUAC_STATUS_REFUSED,

    /**
     * The operation failed because too many resources are already in use.
     */
    GUAC_STATUS_TOO_MANY,

    /**
     * The operation was not performed because it would otherwise block.
     */
    GUAC_STATUS_WOULD_BLOCK

} guac_status;

#endif

