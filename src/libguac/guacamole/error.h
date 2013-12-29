/*
 * Copyright (C) 2013 Glyptodon LLC
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


#ifndef _GUAC_ERROR_H
#define _GUAC_ERROR_H

/**
 * Provides functions and structures required for handling return values and
 * errors.
 *
 * @file error.h
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
     * The end of the input stream associated with the operation
     * has been reached.
     */
    GUAC_STATUS_NO_INPUT,

    /**
     * A timeout occurred while reading from the input stream associated
     * with the operation.
     */
    GUAC_STATUS_INPUT_TIMEOUT,

    /**
     * An error occurred, and further information about the error is already
     * stored in errno.
     */
    GUAC_STATUS_SEE_ERRNO,

    /**
     * An error prevented the operation from writing to its associated
     * output stream.
     */
    GUAC_STATUS_OUTPUT_ERROR,

    /**
     * The operation could not be performed because an invalid argument was
     * given.
     */
    GUAC_STATUS_BAD_ARGUMENT,

    /**
     * The state of the associated system prevents an operation from being
     * performed which would otherwise be allowed.
     */
    GUAC_STATUS_BAD_STATE

} guac_status;

/**
 * Returns a human-readable explanation of the status code given.
 */
const char* guac_status_string(guac_status status);

/**
 * Returns the status code associated with the error which occurred during the
 * last function call. This value will only be set by functions documented to
 * use it (most libguac functions), and is undefined if no error occurred.
 *
 * The storage of this value is thread-local. Assignment of a status code to
 * guac_error in one thread will not affect its value in another thread.
 */
#define guac_error (*__guac_error())

guac_status* __guac_error();

/**
 * Returns a message describing the error which occurred during the last
 * function call. If an error occurred, but no message is associated with it,
 * NULL is returned. This value is undefined if no error occurred.
 *
 * The storage of this value is thread-local. Assignment of a message to
 * guac_error_message in one thread will not affect its value in another
 * thread.
 */
#define guac_error_message (*__guac_error_message())

const char** __guac_error_message();

#endif
