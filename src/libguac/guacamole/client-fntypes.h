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

#ifndef _GUAC_CLIENT_FNTYPES_H
#define _GUAC_CLIENT_FNTYPES_H

/**
 * Function type definitions related to the Guacamole client structure,
 * guac_client.
 *
 * @file client-fntypes.h
 */

#include "client-types.h"
#include "object-types.h"
#include "protocol-types.h"
#include "stream-types.h"
#include "user-types.h"

#include <stdarg.h>

/**
 * Handler for freeing up any extra data allocated by the client
 * implementation.
 *
 * @param client
 *     The client whose extra data should be freed (if any).
 *
 * @return
 *     Zero if the data was successfully freed, non-zero if an error prevents
 *     the data from being freed.
 */
typedef int guac_client_free_handler(guac_client* client);

/**
 * Handler for logging messages related to a given guac_client instance.
 *
 * @param client
 *     The client related to the message being logged.
 *
 * @param level
 *     The log level at which to log the given message.
 *
 * @param format
 *     A printf-style format string, defining the message to be logged.
 *
 * @param args
 *     The va_list containing the arguments to be used when filling the
 *     conversion specifiers ("%s", "%i", etc.) within the format string.
 */
typedef void guac_client_log_handler(guac_client* client,
        guac_client_log_level level, const char* format, va_list args);

/**
 * The entry point of a client plugin which must initialize the given
 * guac_client. In practice, this function will be called "guac_client_init".
 *
 * @param client
 *     The guac_client that must be initialized.
 *
 * @return
 *     Zero on success, non-zero if initialization fails for any reason.
 */
typedef int guac_client_init_handler(guac_client* client);

#endif

