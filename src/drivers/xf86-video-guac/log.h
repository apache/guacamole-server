
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

#ifndef __GUAC_DRV_LOG_H
#define __GUAC_DRV_LOG_H

#include <guacamole/client.h>

/**
 * Logs an arbitrary message at the given log level. The X server's own logging
 * system will be used.
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
void vguac_drv_log(guac_client_log_level level, const char* format,
        va_list args);

/**
 * Logs an arbitrary message at the given log level. The X server's own logging
 * system will be used.
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
void guac_drv_log(guac_client_log_level level, const char* format, ...);

/**
 * Logs an arbitrary message at the given log level. The logging system
 * associated with the given guac_client will be used, which will likely be the
 * X server's own logging system.
 *
 * @param client
 *     The guac_client for which the given message is being logged.
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
void guac_drv_client_log(guac_client* client, guac_client_log_level level,
        const char* format, va_list args);

/**
 * Logs an error message using the logging facilities of the X server,
 * automatically including any information present in guac_error.
 *
 * @param level
 *     The level at which to log this message.
 *
 * @param message
 *     The message to log.
 */
void guac_drv_log_guac_error(guac_client_log_level level, const char* message);

/**
 * Logs an error message using the logging facilities of the given guac_client,
 * automatically including any information present in guac_error. This function
 * accepts parameters identically to printf.
 *
 * @param client
 *     The guac_client for which the given message is being logged.
 *
 * @param level
 *     The level at which to log this message.
 *
 * @param message
 *     The message to log.
 */
void guac_drv_client_log_guac_error(guac_client* client,
        guac_client_log_level level, const char* message);

/**
 * Logs a reasonable explanatory message regarding handshake failure based on
 * the current value of guac_error.
 */
void guac_drv_log_handshake_failure();

#endif

