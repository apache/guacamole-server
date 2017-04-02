
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
#include "log.h"

#include <xorg-server.h>
#include <xf86.h>

#include <guacamole/client.h>
#include <guacamole/error.h>

int guac_drv_log_level = GUAC_LOG_INFO;

void vguac_drv_log(guac_client_log_level level, const char* format,
        va_list args) {

    MessageType type;

    /* Don't bother if the log level is too high */
    if (level > guac_drv_log_level)
        return;

    /* Copy log message into buffer */
    char message[2048];
    vsnprintf(message, sizeof(message), format, args);

    /* Derive XF86 message type from Guacamole log level */
    switch (level) {

        /* Error */
        case GUAC_LOG_ERROR:
            type = X_ERROR;
            break;

        /* Warning */
        case GUAC_LOG_WARNING:
            type = X_WARNING;
            break;

        /* Info */
        case GUAC_LOG_INFO:
            type = X_INFO;
            break;

        /* Info */
        case GUAC_LOG_DEBUG:
            type = X_DEBUG;
            break;

        /* Unknown log level */
        default:
            type = X_UNKNOWN;

    }

    xf86Msg(type, "%s: %s\n", GUAC_DRV_LOG_NAME, message);

}

void guac_drv_log(guac_client_log_level level, const char* format, ...) {
    va_list args;
    va_start(args, format);
    vguac_drv_log(level, format, args);
    va_end(args);
}

void guac_drv_client_log(guac_client* client, guac_client_log_level level,
        const char* format, va_list args) {
    vguac_drv_log(level, format, args);
}

void guac_drv_log_guac_error(guac_client_log_level level, const char* message) {

    if (guac_error != GUAC_STATUS_SUCCESS) {

        /* If error message provided, include in log */
        if (guac_error_message != NULL)
            guac_drv_log(level, "%s: %s",
                    message, guac_error_message);

        /* Otherwise just log with standard status string */
        else
            guac_drv_log(level, "%s: %s",
                    message, guac_status_string(guac_error));

    }

    /* Just log message if no status code */
    else
        guac_drv_log(level, "%s", message);

}

void guac_drv_client_log_guac_error(guac_client* client,
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

void guac_drv_log_handshake_failure() {

    if (guac_error == GUAC_STATUS_CLOSED)
        guac_drv_log(GUAC_LOG_INFO,
                "Guacamole connection closed during handshake");
    else if (guac_error == GUAC_STATUS_PROTOCOL_ERROR)
        guac_drv_log(GUAC_LOG_ERROR,
                "Guacamole protocol violation. Perhaps the version of "
                "guacamole-client is incompatible with this version of "
                "the video driver?");
    else
        guac_drv_log(GUAC_LOG_WARNING,
                "Guacamole handshake failed: %s",
                guac_status_string(guac_error));

}

