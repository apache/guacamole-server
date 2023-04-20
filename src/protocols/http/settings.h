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

#ifndef GUAC_HTTP_SETTINGS_H
#define GUAC_HTTP_SETTINGS_H

#include "config.h"

#include <guacamole/user.h>

/**
 * The default width of the screen to be used if no specific width is
 * provided by the user.
 */
#define HTTP_DEFAULT_WIDTH 1024

/**
 * The default height of the screen to be used if no specific height is
 * provided by the user.
 */
#define HTTP_DEFAULT_HEIGHT 768

/**
 * The default resolution (DPI) to assume if no specific resolution is provided.
 */
#define HTTP_DEFAULT_RESOLUTION 96

typedef struct guac_http_settings {

    /**
     * The URL of the website to be browsed with the HTTP protocol.
     */
    char* url;

    /**
     * The width of the screen.
     */
    int width;

    /**
     * The height of the screen.
     */
    int height;

    /**
     * The resolution of the screen (in DPI).
     */
    int resolution;

} guac_http_settings;

/**
 * NULL-terminated array of accepted client args.
 */
extern const char* GUAC_HTTP_CLIENT_ARGS[];

/**
 * Parses all given args, storing them in a newly-allocated settings object. If
 * the args fail to parse, NULL is returned.
 *
 * @param user
 *     The user who submitted the given arguments while joining the
 *     connection.
 *
 * @param argc
 *     The number of arguments within the argv array.
 *
 * @param argv
 *     The values of all arguments provided by the user.
 *
 * @return
 *     A newly-allocated settings object which must be freed with
 *     guac_http_settings_free() when no longer needed. If the arguments fail
 *     to parse, NULL is returned.
 */
guac_http_settings* guac_http_parse_args(guac_user* user,
        int argc, const char** argv);

/**
 * Frees the given guac_http_settings object, having been previously allocated
 * via guac_http_parse_args().
 *
 * @param settings
 *     The settings object to free.
 */
void guac_http_settings_free(guac_http_settings* settings);
#endif
