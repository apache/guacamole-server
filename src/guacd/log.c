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

#include <guacamole/client.h>
#include <guacamole/error.h>

#include <stdarg.h>
#include <stdio.h>
#include <syslog.h>
#include <unistd.h>

/* Log prefix, defaulting to "guacd" */
char log_prefix[64] = "guacd";

void vguacd_log_info(const char* format, va_list args) {

    /* Copy log message into buffer */
    char message[2048];
    vsnprintf(message, sizeof(message), format, args);

    /* Log to syslog */
    syslog(LOG_INFO, "%s", message);

    /* Log to STDERR */
    fprintf(stderr, "%s[%i]: INFO:  %s\n", log_prefix, getpid(), message);

}

void vguacd_log_error(const char* format, va_list args) {

    /* Copy log message into buffer */
    char message[2048];
    vsnprintf(message, sizeof(message), format, args);

    /* Log to syslog */
    syslog(LOG_ERR, "%s", message);

    /* Log to STDERR */
    fprintf(stderr, "%s[%i]: ERROR: %s\n", log_prefix, getpid(), message);

}

void guacd_log_info(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vguacd_log_info(format, args);
    va_end(args);
}

void guacd_log_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vguacd_log_error(format, args);
    va_end(args);
}

void guacd_client_log_info(guac_client* client, const char* format,
        va_list args) {
    vguacd_log_info(format, args);
}

void guacd_client_log_error(guac_client* client, const char* format,
        va_list args) {
    vguacd_log_error(format, args);
}

void guacd_log_guac_error(const char* message) {

    if (guac_error != GUAC_STATUS_SUCCESS) {

        /* If error message provided, include in log */
        if (guac_error_message != NULL)
            guacd_log_error("%s: %s: %s",
                    message,
                    guac_status_string(guac_error),
                    guac_error_message);

        /* Otherwise just log with standard status string */
        else
            guacd_log_error("%s: %s",
                    message,
                    guac_status_string(guac_error));

    }

    /* Just log message if no status code */
    else
        guacd_log_error("%s", message);

}

void guacd_client_log_guac_error(guac_client* client, const char* message) {

    if (guac_error != GUAC_STATUS_SUCCESS) {

        /* If error message provided, include in log */
        if (guac_error_message != NULL)
            guac_client_log_error(client, "%s: %s: %s",
                    message,
                    guac_status_string(guac_error),
                    guac_error_message);

        /* Otherwise just log with standard status string */
        else
            guac_client_log_error(client, "%s: %s",
                    message,
                    guac_status_string(guac_error));

    }

    /* Just log message if no status code */
    else
        guac_client_log_error(client, "%s", message);

}

