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

#include "guacamole/mem.h"
#include "guacamole/error.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>


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
const char* __GUAC_STATUS_NOT_IMPLEMENTED_STR   = "Not implemented";
const char* __GUAC_STATUS_TRY_AGAIN_STR         = "Temporary failure";
const char* __GUAC_STATUS_PROTOCOL_ERROR_STR    = "Protocol violation";
const char* __GUAC_STATUS_NOT_FOUND_STR         = "Not found";
const char* __GUAC_STATUS_CANCELED_STR          = "Canceled";
const char* __GUAC_STATUS_OUT_OF_RANGE_STR      = "Value out of range";
const char* __GUAC_STATUS_REFUSED_STR           = "Operation refused";
const char* __GUAC_STATUS_TOO_MANY_STR          = "Insufficient resources";
const char* __GUAC_STATUS_WOULD_BLOCK_STR       = "Operation would block";

const char* guac_status_string(guac_status status) {

    switch (status) {

        /* No error */
		case GUAC_STATUS_SUCCESS:
            return __GUAC_STATUS_SUCCESS_STR;

        /* Out of memory */
		case GUAC_STATUS_NO_MEMORY:
            return __GUAC_STATUS_NO_MEMORY_STR;

        /* End of stream */
		case GUAC_STATUS_CLOSED:
            return __GUAC_STATUS_CLOSED_STR;

        /* Timeout */
		case GUAC_STATUS_TIMEOUT:
            return __GUAC_STATUS_TIMEOUT_STR;

        /* Further information in errno */
		case GUAC_STATUS_SEE_ERRNO:
            return strerror(errno);

        /* Input/output error */
		case GUAC_STATUS_IO_ERROR:
            return __GUAC_STATUS_IO_ERROR_STR;

        /* Invalid argument */
		case GUAC_STATUS_INVALID_ARGUMENT:
            return __GUAC_STATUS_INVALID_ARGUMENT_STR;

        /* Internal error */
		case GUAC_STATUS_INTERNAL_ERROR:
            return __GUAC_STATUS_INTERNAL_ERROR_STR;

        /* Out of space */
		case GUAC_STATUS_NO_SPACE:
            return __GUAC_STATUS_NO_SPACE_STR;

        /* Input too large */
        case GUAC_STATUS_INPUT_TOO_LARGE:
            return __GUAC_STATUS_INPUT_TOO_LARGE_STR;

        /* Result too large */
        case GUAC_STATUS_RESULT_TOO_LARGE:
            return __GUAC_STATUS_RESULT_TOO_LARGE_STR;

        /* Permission denied */
        case GUAC_STATUS_PERMISSION_DENIED:
            return __GUAC_STATUS_PERMISSION_DENIED_STR;

        /* Resource is busy */
        case GUAC_STATUS_BUSY:
            return __GUAC_STATUS_BUSY_STR;

        /* Resource not available */
        case GUAC_STATUS_NOT_AVAILABLE:
            return __GUAC_STATUS_NOT_AVAILABLE_STR;

        /* Not supported */
        case GUAC_STATUS_NOT_SUPPORTED:
            return __GUAC_STATUS_NOT_SUPPORTED_STR;

        /* Not implemented */
        case GUAC_STATUS_NOT_IMPLEMENTED:
            return __GUAC_STATUS_NOT_IMPLEMENTED_STR;

        /* Temporary failure */
        case GUAC_STATUS_TRY_AGAIN:
            return __GUAC_STATUS_TRY_AGAIN_STR;

        /* Guacamole protocol error */
        case GUAC_STATUS_PROTOCOL_ERROR:
            return __GUAC_STATUS_PROTOCOL_ERROR_STR;

        /* Resource not found */
        case GUAC_STATUS_NOT_FOUND:
            return __GUAC_STATUS_NOT_FOUND_STR;

        /* Operation canceled */
        case GUAC_STATUS_CANCELED:
            return __GUAC_STATUS_CANCELED_STR;

        /* Value out of range */
        case GUAC_STATUS_OUT_OF_RANGE:
            return __GUAC_STATUS_OUT_OF_RANGE_STR;

        /* Operation refused */
        case GUAC_STATUS_REFUSED:
            return __GUAC_STATUS_REFUSED_STR;

        /* Too many resource in use */
        case GUAC_STATUS_TOO_MANY:
            return __GUAC_STATUS_TOO_MANY_STR;

        /* Operation would block */
        case GUAC_STATUS_WOULD_BLOCK:
            return __GUAC_STATUS_WOULD_BLOCK_STR;

        /* Unknown status code */
        default:
            return __GUAC_STATUS_UNKNOWN_STATUS_STR;

    }

}

/* Thread-safe implementation using our own thread-local API */

#include "guacamole/thread-local.h"

static guac_thread_local_key_t error_key;
static guac_thread_local_key_t error_message_key;
static guac_thread_local_once_t error_keys_once = GUAC_THREAD_LOCAL_ONCE_INIT;

/* Error storage structure */
typedef struct {
    guac_status error;
    const char* message;
} guac_error_storage_t;

/* Initialize thread-local keys once */
static void init_error_keys(void) {
    guac_thread_local_key_create(&error_key, free);
    guac_thread_local_key_create(&error_message_key, NULL);
}

/* Get or create error storage for current thread */
static guac_error_storage_t* get_error_storage(void) {
    guac_thread_local_once(&error_keys_once, init_error_keys);

    guac_error_storage_t* storage = guac_thread_local_getspecific(error_key);
    if (storage == NULL) {
        storage = guac_mem_zalloc(sizeof(guac_error_storage_t));
        if (storage != NULL) {
            guac_thread_local_setspecific(error_key, storage);
        }
    }
    return storage;
}

/* Fallback storage for error cases */
static guac_status fallback_error = GUAC_STATUS_INTERNAL_ERROR;
static const char* fallback_message = NULL;

guac_status* __guac_error() {
    guac_error_storage_t* storage = get_error_storage();
    if (storage) {
        return &storage->error;
    }
    /* Return fallback if storage allocation fails */
    return &fallback_error;
}

const char** __guac_error_message() {
    guac_error_storage_t* storage = get_error_storage();
    if (storage) {
        return &storage->message;
    }
    /* Return fallback if storage allocation fails */
    return &fallback_message;
}

