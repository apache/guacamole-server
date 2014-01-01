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

#include "config.h"

#include "error.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_LIBPTHREAD
#include <pthread.h>
#endif

/* Error strings */

const char* __GUAC_STATUS_SUCCESS_STR        = "Success";
const char* __GUAC_STATUS_NO_MEMORY_STR      = "Insufficient memory";
const char* __GUAC_STATUS_NO_INPUT_STR       = "End of input stream";
const char* __GUAC_STATUS_INPUT_TIMEOUT_STR  = "Read timeout";
const char* __GUAC_STATUS_OUTPUT_ERROR_STR   = "Output error";
const char* __GUAC_STATUS_BAD_ARGUMENT_STR   = "Invalid argument";
const char* __GUAC_STATUS_BAD_STATE_STR      = "Illegal state";
const char* __GUAC_STATUS_INVALID_STATUS_STR = "UNKNOWN STATUS CODE";

const char* guac_status_string(guac_status status) {

    switch (status) {

        /* No error */
		case GUAC_STATUS_SUCCESS:
            return __GUAC_STATUS_SUCCESS_STR;

        /* Out of memory */
		case GUAC_STATUS_NO_MEMORY:
            return __GUAC_STATUS_NO_MEMORY_STR;

        /* End of input */
		case GUAC_STATUS_NO_INPUT:
            return __GUAC_STATUS_NO_INPUT_STR;

        /* Input timeout */
		case GUAC_STATUS_INPUT_TIMEOUT:
            return __GUAC_STATUS_INPUT_TIMEOUT_STR;

        /* Further information in errno */
		case GUAC_STATUS_SEE_ERRNO:
            return strerror(errno);

        /* Output error */
		case GUAC_STATUS_OUTPUT_ERROR:
            return __GUAC_STATUS_OUTPUT_ERROR_STR;

        /* Invalid argument */
		case GUAC_STATUS_BAD_ARGUMENT:
            return __GUAC_STATUS_BAD_ARGUMENT_STR;

        /* Illegal state */
		case GUAC_STATUS_BAD_STATE:
            return __GUAC_STATUS_BAD_STATE_STR;

        default:
            return __GUAC_STATUS_INVALID_STATUS_STR;

    }

}

#ifdef HAVE_LIBPTHREAD

/* PThread implementation of __guac_error */

static pthread_key_t  __guac_error_key;
static pthread_once_t __guac_error_key_init = PTHREAD_ONCE_INIT;

static pthread_key_t  __guac_error_message_key;
static pthread_once_t __guac_error_message_key_init = PTHREAD_ONCE_INIT;

static void __guac_free_pointer(void* pointer) {

    /* Free memory allocated to status variable */
    free(pointer);

}

static void __guac_alloc_error_key() {

    /* Create key, destroy any allocated variable on thread exit */
    pthread_key_create(&__guac_error_key, __guac_free_pointer);

}

static void __guac_alloc_error_message_key() {

    /* Create key, destroy any allocated variable on thread exit */
    pthread_key_create(&__guac_error_message_key, __guac_free_pointer);

}

guac_status* __guac_error() {

    /* Pointer for thread-local data */
    guac_status* status;

    /* Init error key, if not already initialized */
    pthread_once(&__guac_error_key_init, __guac_alloc_error_key);

    /* Retrieve thread-local status variable */
    status = (guac_status*) pthread_getspecific(__guac_error_key);

    /* Allocate thread-local status variable if not already allocated */
    if (status == NULL) {
        status = malloc(sizeof(guac_status));
        pthread_setspecific(__guac_error_key, status);
    }

    return status;

}

const char** __guac_error_message() {

    /* Pointer for thread-local data */
    const char** message;

    /* Init error message key, if not already initialized */
    pthread_once(
        &__guac_error_message_key_init,
        __guac_alloc_error_message_key
    );

    /* Retrieve thread-local message variable */
    message = (const char**) pthread_getspecific(__guac_error_message_key);

    /* Allocate thread-local message variable if not already allocated */
    if (message == NULL) {
        message = malloc(sizeof(const char*));
        pthread_setspecific(__guac_error_message_key, message);
    }

    return message;

}

#else

/* Default (not-threadsafe) implementation */
static guac_status __guac_error_unsafe_storage;
static const char** __guac_error_message_unsafe_storage;

guac_status* __guac_error() {
    return &__guac_error_unsafe_storage;
}

const char** __guac_error_message() {
    return &__guac_error_message_unsafe_storage;
}

/* Warn about threadsafety */
#warn No threadsafe implementation of __guac_error exists for your platform, so a default non-threadsafe implementation has been used instead. This may lead to incorrect status codes being reported for failures. Please consider adding support for your platform, or filing a bug report with the Guacamole project.

#endif

