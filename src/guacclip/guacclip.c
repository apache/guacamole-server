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

#include "guacclip.h"
#include "interpret.h"
#include "log.h"
#include "state.h"

#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Long-option identifiers for options which have no single-character
 * equivalent.
 */
enum {
    GUACCLIP_OPT_DIRECTION = 256,
    GUACCLIP_OPT_INCLUDE,
    GUACCLIP_OPT_MAX_ITEM_BYTES
};

int main(int argc, char* argv[]) {

    int i;

    /* Load defaults */
    bool force = false;
    const char* explicit_outdir = NULL;
    bool max_item_bytes_set = false;

    guacclip_options options = {
        .outdir           = NULL,
        .direction_filter = NULL,
        .include          = GUACCLIP_INCLUDE_ALL,
        .max_item_bytes   = GUACCLIP_DEFAULT_MAX_ITEM_BYTES
    };

    /* Define long options */
    static struct option long_options[] = {
        {"direction",      required_argument, NULL, GUACCLIP_OPT_DIRECTION},
        {"include",        required_argument, NULL, GUACCLIP_OPT_INCLUDE},
        {"max-item-bytes", required_argument, NULL, GUACCLIP_OPT_MAX_ITEM_BYTES},
        {NULL,             0,                 NULL, 0}
    };

    /* Parse arguments */
    int opt;
    while ((opt = getopt_long(argc, argv, "fo:", long_options, NULL)) != -1) {

        switch (opt) {

            /* -f: Force */
            case 'f':
                force = true;
                break;

            /* -o: Output directory */
            case 'o':
                explicit_outdir = optarg;
                break;

            /* --direction: Restrict to a single transfer direction */
            case GUACCLIP_OPT_DIRECTION:
                if (strcmp(optarg, "guest-to-client") != 0
                        && strcmp(optarg, "client-to-guest") != 0) {
                    guacclip_log(GUAC_LOG_ERROR, "Invalid --direction value "
                            "\"%s\" (expected guest-to-client or "
                            "client-to-guest).", optarg);
                    goto invalid_options;
                }
                options.direction_filter = optarg;
                break;

            /* --include: Restrict to image, text, or all content */
            case GUACCLIP_OPT_INCLUDE:
                if (strcmp(optarg, "image") == 0)
                    options.include = GUACCLIP_INCLUDE_IMAGE;
                else if (strcmp(optarg, "text") == 0)
                    options.include = GUACCLIP_INCLUDE_TEXT;
                else if (strcmp(optarg, "all") == 0)
                    options.include = GUACCLIP_INCLUDE_ALL;
                else {
                    guacclip_log(GUAC_LOG_ERROR, "Invalid --include value "
                            "\"%s\" (expected image, text, or all).", optarg);
                    goto invalid_options;
                }
                break;

            /* --max-item-bytes: Cap per-item size */
            case GUACCLIP_OPT_MAX_ITEM_BYTES: {
                char* endptr = NULL;
                long long value = strtoll(optarg, &endptr, 10);
                if (endptr == optarg || *endptr != '\0' || value < 0) {
                    guacclip_log(GUAC_LOG_ERROR, "Invalid --max-item-bytes "
                            "value \"%s\" (expected a non-negative integer).",
                            optarg);
                    goto invalid_options;
                }
                options.max_item_bytes = (size_t) value;
                max_item_bytes_set = true;
                break;
            }

            /* Invalid option */
            default:
                goto invalid_options;

        }

    }

    /* Log start */
    guacclip_log(GUAC_LOG_INFO, "Guacamole clipboard artifact extractor "
            "(guacclip) version " VERSION);

    /* Note the effective per-item size cap when using the built-in default */
    if (!max_item_bytes_set)
        guacclip_log(GUAC_LOG_DEBUG, "No --max-item-bytes specified; "
                "defaulting to a %zu-byte cap per clipboard item (specify "
                "--max-item-bytes 0 for no limit).", options.max_item_bytes);

    /* Track number of overall failures */
    int total_files = argc - optind;
    int failures = 0;

    /* Abort if no files given */
    if (total_files <= 0) {
        guacclip_log(GUAC_LOG_INFO, "No input files specified. Nothing to do.");
        return 0;
    }

    guacclip_log(GUAC_LOG_INFO, "%i input file(s) provided.", total_files);

    /* Interpret all input files */
    for (i = optind; i < argc; i++) {

        /* Get current filename */
        const char* path = argv[i];

        /* Determine effective output directory for this recording */
        char default_outdir[4096];
        if (explicit_outdir != NULL)
            options.outdir = explicit_outdir;
        else {
            int len = snprintf(default_outdir, sizeof(default_outdir),
                    "%s.clipboard", path);
            if (len < 0 || (size_t) len >= sizeof(default_outdir)) {
                guacclip_log(GUAC_LOG_ERROR, "Cannot derive output directory "
                        "for \"%s\": Name too long", path);
                failures++;
                continue;
            }
            options.outdir = default_outdir;
        }

        /* Attempt extraction, log granular success/failure at debug level */
        if (guacclip_interpret(path, &options, force)) {
            failures++;
            guacclip_log(GUAC_LOG_DEBUG,
                    "%s was NOT successfully interpreted.", path);
        }
        else
            guacclip_log(GUAC_LOG_DEBUG, "%s was successfully "
                    "interpreted.", path);

    }

    /* Warn if at least one file failed */
    if (failures != 0)
        guacclip_log(GUAC_LOG_WARNING, "Extraction failed for %i of %i "
                "file(s).", failures, total_files);

    /* Notify of success */
    else
        guacclip_log(GUAC_LOG_INFO, "All files interpreted successfully.");

    /* Interpreting complete */
    return failures == 0 ? 0 : 1;

    /* Display usage and exit with error if options are invalid */
invalid_options:

    fprintf(stderr, "USAGE: %s"
            " [-f]"
            " [-o OUTDIR]"
            " [--direction guest-to-client|client-to-guest]"
            " [--include image|text|all]"
            " [--max-item-bytes N]"
            " [FILE]...\n"
            "\n"
            "    --max-item-bytes N   Caps each extracted clipboard item to "
            "N bytes\n"
            "                         (default: %d, i.e. 64 MiB). Specify\n"
            "                         --max-item-bytes 0 to disable the "
            "cap.\n",
            argv[0], GUACCLIP_DEFAULT_MAX_ITEM_BYTES);

    return 1;

}
