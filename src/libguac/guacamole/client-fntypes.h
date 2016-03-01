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
 */
typedef int guac_client_free_handler(guac_client* client);

/**
 * Handler for logging messages.
 */
typedef void guac_client_log_handler(guac_client* client, guac_client_log_level level, const char* format, va_list args); 

/**
 * Handler which should initialize the given guac_client.
 */
typedef int guac_client_init_handler(guac_client* client);

#endif

