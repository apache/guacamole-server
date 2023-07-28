/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "config.h"

#include "guacamole/error.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_LIBPTHREAD
#include <pthread.h>
#endif

#ifdef CYGWIN_BUILD
#include <io.h>
#include <stdio.h>
#include <windows.h>
#endif

/*
 * Error strings
 */

const char* __GUAC_STATUS_SUCCESS_STR           = "Success";
const char* __GUAC_STATUS_NO_MEMORY_STR         = "Insufficient memory";
const char* __GUAC_STATUS_CLOSED_STR            = "Closed";
const char* __GUAC_STATUS_TIMEOUT_STR           = "Timed out";
const char* __GUAC_STATUS_IO_ERROR_STR          = "Input/output error";
const char* __GUAC_STATUS_INVALID_ARGUMENT_STR  = "Invalid argument";
const char* __GUAC_STATUS_INTERNAL_ERROR_STR    = "Internal error";
const char* __GUAC_STATUS_UNKNOWN_STATUS_STR    = "UNKNOWN STATUS CODE";
const char* __GUAC_STATUS_NO_SPACE_STR          = "Insufficient space";
const char* __GUAC_STATUS_INPUT_TOO_LARGE_STR   = "Input too large";
const char* __GUAC_STATUS_RESULT_TOO_LARGE_STR  = "Result too large";
const char* __GUAC_STATUS_PERMISSION_DENIED_STR = "Permission denied";
const char* __GUAC_STATUS_BUSY_STR              = "Resource busy";
const char* __GUAC_STATUS_NOT_AVAILABLE_STR     = "Resource not available";
const char* __GUAC_STATUS_NOT_SUPPORTED_STR     = "Not supported";
const char* __GUAC_STATUS_NOT_INPLEMENTED_STR   = "Not implemented";
const char* __GUAC_STATUS_TRY_AGAIN_STR         = "Temporary failure";
const char* __GUAC_STATUS_PROTOCOL_ERROR_STR    = "Protocol violation";
const char* __GUAC_STATUS_NOT_FOUND_STR         = "Not found";
const char* __GUAC_STATUS_CANCELED_STR          = "Canceled";
const char* __GUAC_STATUS_OUT_OF_RANGE_STR      = "Value out of range";
const char* __GUAC_STATUS_REFUSED_STR           = "Operation refused";
const char* __GUAC_STATUS_TOO_MANY_STR          = "Insufficient resources";
const char* __GUAC_STATUS_WOULD_BLOCK_STR       = "Operation would block";

#ifdef CYGWIN_BUILD

/*
 * A format for a backup message in case Windows can't produce one. 
 */
#define FALLBACK_ERROR_FORMAT "Failed to get Windows error for code %ul"

/*
 * The size of newly constructed fallback error buffers. Large enough to 
 * contain the longest possible DWORD and a null terminator.
 */
#define FALLBACK_ERROR_SIZE 64

/**
 * Get the error message associated with the most recent Windows error. This
 * message is allocated fresh every time this function is called, and must be
 * freed by the caller.
 * 
 */
static LPTSTR get_last_error_message() {

    /* This will be set to point to the buffer allocated by FormatMessage */
    LPTSTR error_buffer = NULL;
    
    /* Attempt to format message using system error message tables */
    DWORD length = FormatMessage(
        
        /* Fetch error messages from system message tables */
        FORMAT_MESSAGE_FROM_SYSTEM

        /* Allocate a buffer to hold the message text */
        |FORMAT_MESSAGE_ALLOCATE_BUFFER

        /* We're not supplying our own text inserts */
        |FORMAT_MESSAGE_IGNORE_INSERTS,  

        /* Don't pass any custom message tables */
        NULL,

        /* Pass the error code in to get the error message */
        guac_windows_error_code,

        /* Use default language settings */
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),

        /* 
         * Cast the address of the buffer back to a LPTSTR, even though it's
         * really a LPTSTR*. The function will internally treat it as a LPTSTR*,
         * populating `errorText` with the message text.
         */
        (LPTSTR)&error_buffer, 

        /* Minimum output buffer size */
        0,

        /* No extra arguments */
        NULL
    );
    
    /* If Windows failed to generate an error message */
    if (!length) {

        /* Allocate and format the fallback error message */
        error_buffer = malloc(FALLBACK_ERROR_SIZE * sizeof(TCHAR));

        /* Format the message */
        sprintf(error_buffer, FALLBACK_ERROR_FORMAT, guac_windows_error_code);
    }

    return error_buffer;

}

#endif

/**
 * Allocate and return a copy of the provided null-terminated string constant.
 * 
 * @param string_constant
 *     The null-terminated string constant to duplicated.
 * 
 * @return
 *     A newly allocated copy of the provided string constant.
 */
static char* guac_status_duplicate_constant(const char* string_constant) {

    /* Create a buffer to hold the message plus null terminator */
    char* string_copy = malloc((strlen(string_constant) + 1) * sizeof(char));

    /* Copy the constant into the buffer */
    strcpy(string_copy, string_constant);

    /* A newly-allocated copy of the constant */
    return string_copy;

}

char* guac_status_string(guac_status status) {

    switch (status) {

        /* No error */
		case GUAC_STATUS_SUCCESS:
            return guac_status_duplicate_constant(__GUAC_STATUS_SUCCESS_STR);

        /* Out of memory */
		case GUAC_STATUS_NO_MEMORY:
            return guac_status_duplicate_constant(__GUAC_STATUS_NO_MEMORY_STR);

        /* End of stream */
		case GUAC_STATUS_CLOSED:
            return guac_status_duplicate_constant(__GUAC_STATUS_CLOSED_STR);

        /* Timeout */
		case GUAC_STATUS_TIMEOUT:
            return guac_status_duplicate_constant(__GUAC_STATUS_TIMEOUT_STR);

        /* Further information in errno */
		case GUAC_STATUS_SEE_ERRNO:
            return guac_status_duplicate_constant(strerror(errno));
    
#ifdef CYGWIN_BUILD

        /* Further information available from the Windows API. */
        case GUAC_STATUS_SEE_WINDOWS_ERROR:
            return get_last_error_message();

#endif

        /* Input/output error */
		case GUAC_STATUS_IO_ERROR:
            return guac_status_duplicate_constant(__GUAC_STATUS_IO_ERROR_STR);

        /* Invalid argument */
		case GUAC_STATUS_INVALID_ARGUMENT:
            return guac_status_duplicate_constant(__GUAC_STATUS_INVALID_ARGUMENT_STR);

        /* Internal error */
		case GUAC_STATUS_INTERNAL_ERROR:
            return guac_status_duplicate_constant(__GUAC_STATUS_INTERNAL_ERROR_STR);

        /* Out of space */
		case GUAC_STATUS_NO_SPACE:
            return guac_status_duplicate_constant(__GUAC_STATUS_NO_SPACE_STR);

        /* Input too large */
        case GUAC_STATUS_INPUT_TOO_LARGE:
            return guac_status_duplicate_constant(__GUAC_STATUS_INPUT_TOO_LARGE_STR);

        /* Result too large */
        case GUAC_STATUS_RESULT_TOO_LARGE:
            return guac_status_duplicate_constant(__GUAC_STATUS_RESULT_TOO_LARGE_STR);

        /* Permission denied */
        case GUAC_STATUS_PERMISSION_DENIED:
            return guac_status_duplicate_constant(__GUAC_STATUS_PERMISSION_DENIED_STR);

        /* Resource is busy */
        case GUAC_STATUS_BUSY:
            return guac_status_duplicate_constant(__GUAC_STATUS_BUSY_STR);

        /* Resource not available */
        case GUAC_STATUS_NOT_AVAILABLE:
            return guac_status_duplicate_constant(__GUAC_STATUS_NOT_AVAILABLE_STR);

        /* Not supported */
        case GUAC_STATUS_NOT_SUPPORTED:
            return guac_status_duplicate_constant(__GUAC_STATUS_NOT_SUPPORTED_STR);

        /* Not implemented */
        case GUAC_STATUS_NOT_INPLEMENTED:
            return guac_status_duplicate_constant(__GUAC_STATUS_NOT_INPLEMENTED_STR);

        /* Temporary failure */
        case GUAC_STATUS_TRY_AGAIN:
            return guac_status_duplicate_constant(__GUAC_STATUS_TRY_AGAIN_STR);

        /* Guacamole protocol error */
        case GUAC_STATUS_PROTOCOL_ERROR:
            return guac_status_duplicate_constant(__GUAC_STATUS_PROTOCOL_ERROR_STR);

        /* Resource not found */
        case GUAC_STATUS_NOT_FOUND:
            return guac_status_duplicate_constant(__GUAC_STATUS_NOT_FOUND_STR);

        /* Operation canceled */
        case GUAC_STATUS_CANCELED:
            return guac_status_duplicate_constant(__GUAC_STATUS_CANCELED_STR);

        /* Value out of range */
        case GUAC_STATUS_OUT_OF_RANGE:
            return guac_status_duplicate_constant(__GUAC_STATUS_OUT_OF_RANGE_STR);

        /* Operation refused */
        case GUAC_STATUS_REFUSED:
            return guac_status_duplicate_constant(__GUAC_STATUS_REFUSED_STR);

        /* Too many resource in use */
        case GUAC_STATUS_TOO_MANY:
            return guac_status_duplicate_constant(__GUAC_STATUS_TOO_MANY_STR);

        /* Operation would block */
        case GUAC_STATUS_WOULD_BLOCK:
            return guac_status_duplicate_constant(__GUAC_STATUS_WOULD_BLOCK_STR);

        /* Unknown status code */
        default:
            return guac_status_duplicate_constant(__GUAC_STATUS_UNKNOWN_STATUS_STR);

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

char** __guac_error_message() {

    /* Pointer for thread-local data */
    char** message;

    /* Init error message key, if not already initialized */
    pthread_once(
        &__guac_error_message_key_init,
        __guac_alloc_error_message_key
    );

    /* Retrieve thread-local message variable */
    message = (char**) pthread_getspecific(__guac_error_message_key);

    /* Allocate thread-local message variable if not already allocated */
    if (message == NULL) {
        message = malloc(sizeof(char*));
        pthread_setspecific(__guac_error_message_key, message);
    }

    return message;

}

#ifdef CYGWIN_BUILD

/**
 * PThread key for the thread local Windows error code associated with
 * the current guac_status, if the status is GUAC_STATUS_SEE_WINDOWS_ERROR.
 */
static pthread_key_t guac_windows_code_key;

/**
 * A static pthread_once_t instance to ensure that the Windows error code
 * thread local is only created once.
 */
static pthread_once_t guac_windows_code_key_init = PTHREAD_ONCE_INIT;

/**
 * Allocate the PThread key for the Windows error code associated with the
 * current error.
 */
static void guac_alloc_windows_code_key() {

    /* Create key, destroy any allocated variable on thread exit */
    pthread_key_create(&guac_windows_code_key, __guac_free_pointer);

}

DWORD* __guac_windows_code() {

    /* Pointer for thread-local data */
    DWORD* windows_code;

    /* Init error code key, if not already initialized */
    pthread_once(&guac_windows_code_key_init, guac_alloc_windows_code_key);

    /* Get the value of the current thread local */
    windows_code = (DWORD*) pthread_getspecific(guac_windows_code_key);

    /* Allocate thread-local error code variable if not already allocated */
    if (windows_code == NULL) {
        windows_code = malloc(sizeof(DWORD));
        pthread_setspecific(guac_windows_code_key, windows_code);
    }

    return windows_code;

}

#endif

#else

/* Default (not-threadsafe) implementation */
static guac_status __guac_error_unsafe_storage;
static char** __guac_error_message_unsafe_storage;

guac_status* __guac_error() {
    return &__guac_error_unsafe_storage;
}

char** __guac_error_message() {
    return &__guac_error_message_unsafe_storage;
}

/* Warn about threadsafety */
#warn No threadsafe implementation of __guac_error exists for your platform, so a default non-threadsafe implementation has been used instead. This may lead to incorrect status codes being reported for failures. Please consider adding support for your platform, or filing a bug report with the Guacamole project.

#ifdef CYGWIN_BUILD

/**
 * Non-threadsafe storage for the Windows error code.
 */
static DWORD __guac_windows_error_unsafe_storage;

/**
 * Return non-threadsafe Windows error code.
 * 
 * @return
 *     The non-threadsafe Windows error code.
 */
DWORD* __guac_windows_code() {
    return &__guac_windows_error_unsafe_storage;
}

#endif

#endif

