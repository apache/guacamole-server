/*
 * Copyright (C) 2016 Glyptodon, Inc.
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

#ifndef GUACENC_LOG_H
#define GUACENC_LOG_H

#include "config.h"

#include <guacamole/client.h>

#include <stdarg.h>

/**
 * The maximum level at which to log messages. All other messages will be
 * dropped.
 */
extern int guacenc_log_level;

/**
 * The string to prepend to all log messages.
 */
#define GUACENC_LOG_NAME "guacenc"

/**
 * Writes a message to guacenc's logs. This function takes a format and
 * va_list, similar to vprintf.
 *
 * @param level
 *     The level at which to log this message.
 *
 * @param format
 *     A printf-style format string to log.
 *
 * @param args
 *     The va_list containing the arguments to be used when filling the format
 *     string for printing.
 */
void vguacenc_log(guac_client_log_level level, const char* format,
        va_list args);

/**
 * Writes a message to guacenc's logs. This function accepts parameters
 * identically to printf.
 *
 * @param level
 *     The level at which to log this message.
 *
 * @param format
 *     A printf-style format string to log.
 *
 * @param ...
 *     Arguments to use when filling the format string for printing.
 */
void guacenc_log(guac_client_log_level level, const char* format, ...);

#endif

