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

#include "config.h"
#include "log.h"

#include <guacamole/client.h>
#include <guacamole/error.h>

#include <stdarg.h>
#include <stdio.h>

int guacenc_log_level = GUAC_LOG_DEBUG;

void vguacenc_log(guac_client_log_level level, const char* format,
        va_list args) {

    const char* priority_name;
    char message[2048];

    /* Don't bother if the log level is too high */
    if (level > guacenc_log_level)
        return;

    /* Copy log message into buffer */
    vsnprintf(message, sizeof(message), format, args);

    /* Convert log level to human-readable name */
    switch (level) {

        /* Error log level */
        case GUAC_LOG_ERROR:
            priority_name = "ERROR";
            break;

        /* Warning log level */
        case GUAC_LOG_WARNING:
            priority_name = "WARNING";
            break;

        /* Informational log level */
        case GUAC_LOG_INFO:
            priority_name = "INFO";
            break;

        /* Debug log level */
        case GUAC_LOG_DEBUG:
            priority_name = "DEBUG";
            break;

        /* Any unknown/undefined log level */
        default:
            priority_name = "UNKNOWN";
            break;
    }

    /* Log to STDERR */
    fprintf(stderr, GUACENC_LOG_NAME ": %s: %s\n", priority_name, message);

}

void guacenc_log(guac_client_log_level level, const char* format, ...) {
    va_list args;
    va_start(args, format);
    vguacenc_log(level, format, args);
    va_end(args);
}

