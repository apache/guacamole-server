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
#include "log.h"

#include <guacamole/client.h>
#include <guacamole/error.h>

#include <stdarg.h>
#include <stdio.h>
#include <syslog.h>
#include <unistd.h>

int guacd_log_level = GUAC_LOG_INFO;

void vguacd_log(guac_client_log_level level, const char* format,
        va_list args) {

    const char* priority_name;
    int priority;

    char message[2048];

    /* Don't bother if the log level is too high */
    if (level > guacd_log_level)
        return;

    /* Copy log message into buffer */
    vsnprintf(message, sizeof(message), format, args);

    /* Convert log level to syslog priority */
    switch (level) {

        /* Error log level */
        case GUAC_LOG_ERROR:
            priority = LOG_ERR;
            priority_name = "ERROR";
            break;

        /* Warning log level */
        case GUAC_LOG_WARNING:
            priority = LOG_WARNING;
            priority_name = "WARNING";
            break;

        /* Informational log level */
        case GUAC_LOG_INFO:
            priority = LOG_INFO;
            priority_name = "INFO";
            break;

        /* Debug log level */
        case GUAC_LOG_DEBUG:
            priority = LOG_DEBUG;
            priority_name = "DEBUG";
            break;

        /* Any unknown/undefined log level */
        default:
            priority = LOG_INFO;
            priority_name = "UNKNOWN";
            break;
    }

    /* Log to syslog */
    syslog(priority, "%s", message);

    /* Log to STDERR */
    fprintf(stderr, GUACD_LOG_NAME "[%i]: %s:\t%s\n",
            getpid(), priority_name, message);

}

void guacd_log(guac_client_log_level level, const char* format, ...) {
    va_list args;
    va_start(args, format);
    vguacd_log(level, format, args);
    va_end(args);
}

void guacd_client_log(guac_client* client, guac_client_log_level level,
        const char* format, va_list args) {
    vguacd_log(level, format, args);
}

void guacd_log_guac_error(guac_client_log_level level, const char* message) {

    if (guac_error != GUAC_STATUS_SUCCESS) {

        /* If error message provided, include in log */
        if (guac_error_message != NULL)
            guacd_log(level, "%s: %s",
                    message,
                    guac_error_message);

        /* Otherwise just log with standard status string */
        else
            guacd_log(level, "%s: %s",
                    message,
                    guac_status_string(guac_error));

    }

    /* Just log message if no status code */
    else
        guacd_log(level, "%s", message);

}

void guacd_client_log_guac_error(guac_client* client,
        guac_client_log_level level, const char* message) {

    if (guac_error != GUAC_STATUS_SUCCESS) {

        /* If error message provided, include in log */
        if (guac_error_message != NULL)
            guac_client_log(client, level, "%s: %s",
                    message,
                    guac_error_message);

        /* Otherwise just log with standard status string */
        else
            guac_client_log(client, level, "%s: %s",
                    message,
                    guac_status_string(guac_error));

    }

    /* Just log message if no status code */
    else
        guac_client_log(client, level, "%s", message);

}

void guacd_log_handshake_failure() {

    if (guac_error == GUAC_STATUS_CLOSED)
        guacd_log(GUAC_LOG_INFO,
                "Guacamole connection closed during handshake");
    else if (guac_error == GUAC_STATUS_PROTOCOL_ERROR)
        guacd_log(GUAC_LOG_ERROR,
                "Guacamole protocol violation. Perhaps the version of "
                "guacamole-client is incompatible with this version of "
                "guacd?");
    else
        guacd_log(GUAC_LOG_WARNING,
                "Guacamole handshake failed: %s",
                guac_status_string(guac_error));

}

