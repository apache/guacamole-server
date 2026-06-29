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

#include <guacamole/client.h>

#include <glib.h>

/**
 * Translates a GLib log level into the closest-matching Guacamole log level.
 *
 * @param level
 *     The GLib log level flags associated with a log message.
 *
 * @return
 *     The Guacamole log level which most closely corresponds to the given
 *     GLib log level.
 */
static guac_client_log_level guac_spice_log_level(GLogLevelFlags level) {

    /* Mask off the log level bits, ignoring any additional flags */
    switch (level & G_LOG_LEVEL_MASK) {

        case G_LOG_LEVEL_ERROR:
        case G_LOG_LEVEL_CRITICAL:
            return GUAC_LOG_ERROR;

        case G_LOG_LEVEL_WARNING:
            return GUAC_LOG_WARNING;

        case G_LOG_LEVEL_MESSAGE:
        case G_LOG_LEVEL_INFO:
            return GUAC_LOG_INFO;

        default:
            return GUAC_LOG_DEBUG;

    }

}

/**
 * GLib log handler which routes all received log messages to the logging
 * facilities of the guac_client provided as user data.
 *
 * @param domain
 *     The log domain of the message, or NULL if not specified.
 *
 * @param level
 *     The GLib log level flags of the message.
 *
 * @param message
 *     The text of the log message.
 *
 * @param data
 *     The guac_client to which the message should be logged.
 */
static void guac_spice_log_handler(const gchar* domain, GLogLevelFlags level,
        const gchar* message, gpointer data) {

    guac_client* client = (guac_client*) data;

    guac_client_log(client, guac_spice_log_level(level), "%s: %s",
            domain != NULL ? domain : "spice", message);

}

void guac_spice_client_log_init(guac_client* client) {

    /* Route all log messages produced by spice-gtk and the underlying
     * GLib/GObject stack to the logging facilities of the given client */
    g_log_set_handler("GSpice",
            G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION,
            guac_spice_log_handler, client);

    g_log_set_handler(NULL,
            G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION,
            guac_spice_log_handler, client);

}
