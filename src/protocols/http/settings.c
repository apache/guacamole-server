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

#include "resolution.h"
#include "settings.h"
#include "argv.h"

#include <guacamole/user.h>
#include <stdlib.h>
#include <string.h>

/* Client plugin arguments */
const char* GUAC_HTTP_CLIENT_ARGS[] = {
    "url",
    "height",
    "width",
    "resolution",
    NULL
};

enum HTTP_ARGS_IDX {
   /**
     * The url of the website to be rendered.
     */
    IDX_URL,

    /**
     * The width of the display to request, in pixels. If omitted, a reasonable
     * value will be calculated based on the user's own display size and
     * resolution.
     */
    IDX_WIDTH,

    /**
     * The height of the display to request, in pixels. If omitted, a
     * reasonable value will be calculated based on the user's own display
     * size and resolution.
     */
    IDX_HEIGHT,

    /**
     * The resolution of the display to request, in DPI. If omitted, a
     * reasonable value will be calculated based on the user's own display
     * size and resolution.
     */
    IDX_DPI,

    HTTP_ARGS_COUNT
};

guac_http_settings* guac_http_parse_args(guac_user* user,
        int argc, const char** argv) {

    /* Validate arg count */
    if (argc != HTTP_ARGS_COUNT) {
        guac_user_log(user, GUAC_LOG_WARNING, "Incorrect number of connection "
                "parameters provided: expected %i, got %i.",
                HTTP_ARGS_COUNT, argc);
        return NULL;
    }

    /* Allocate a new guac_http_settings object */
    guac_http_settings* settings = calloc(1, sizeof(guac_http_settings));

        /* Set hostname */
    settings->url =
        guac_user_parse_args_string(user, GUAC_HTTP_CLIENT_ARGS, argv,
                IDX_URL, "");

    /* Use suggested resolution unless overridden */
    settings->resolution =
        guac_user_parse_args_int(user, GUAC_HTTP_CLIENT_ARGS, argv,
                IDX_DPI, guac_http_suggest_resolution(user));

    /* Use optimal width unless overridden */
    settings->width = user->info.optimal_width
                    * settings->resolution
                    / user->info.optimal_resolution;

    if (argv[IDX_WIDTH][0] != '\0')
        settings->width = atoi(argv[IDX_WIDTH]);

    /* Use default width if given width is invalid. */
    if (settings->width <= 0) {
        settings->width = HTTP_DEFAULT_WIDTH;
        guac_user_log(user, GUAC_LOG_ERROR,
                "Invalid width: \"%s\". Using default of %i.",
                argv[IDX_WIDTH], settings->width);
    }

    /* Round width down to nearest multiple of 4 */
    settings->width = settings->width & ~0x3;

    /* Use optimal height unless overridden */
    settings->height = user->info.optimal_height
                     * settings->resolution
                     / user->info.optimal_resolution;

    if (argv[IDX_HEIGHT][0] != '\0')
        settings->height = atoi(argv[IDX_HEIGHT]);

    /* Use default height if given height is invalid. */
    if (settings->height <= 0) {
        settings->height = HTTP_DEFAULT_HEIGHT;
        guac_user_log(user, GUAC_LOG_ERROR,
                "Invalid height: \"%s\". Using default of %i.",
                argv[IDX_HEIGHT], settings->height);
    }

    guac_user_log(user, GUAC_LOG_DEBUG,
            "Using resolution of %ix%i at %i DPI",
            settings->width,
            settings->height,
            settings->resolution);

    return settings;
}

void guac_http_settings_free(guac_http_settings* settings) {

    /* Free memory allocated for settings */
    if (settings->url)
        free(settings->url);

    /* TODO: Free memory for other strings and dynamically-allocated members of guac_http_settings if applicable */

    free(settings);
}
