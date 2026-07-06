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

#include "encode.h"
#include "guacenc.h"
#include "log.h"
#include "parse.h"

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[]) {

    int i;

    /* Load defaults */
    bool force = false;
    const char* out_file = NULL;
    int width = GUACENC_DEFAULT_WIDTH;
    int height = GUACENC_DEFAULT_HEIGHT;
    int bitrate = GUACENC_DEFAULT_BITRATE;

    /* Parse arguments */
    int opt;
    while ((opt = getopt(argc, argv, "s:r:fo:")) != -1) {

        /* -s: Dimensions (WIDTHxHEIGHT) */
        if (opt == 's') {
            if (guacenc_parse_dimensions(optarg, &width, &height)) {
                guacenc_log(GUAC_LOG_ERROR, "Invalid dimensions.");
                goto invalid_options;
            }
        }

        /* -r: Bitrate (bits per second) */
        else if (opt == 'r') {
            if (guacenc_parse_int(optarg, &bitrate)) {
                guacenc_log(GUAC_LOG_ERROR, "Invalid bitrate.");
                goto invalid_options;
            }
        }

        /* -f: Force */
        else if (opt == 'f')
            force = true;

        /* -o: Output file ("-" for STDOUT) */
        else if (opt == 'o')
            out_file = optarg;

        /* Invalid option */
        else {
            goto invalid_options;
        }

    }

    /* Log start */
    guacenc_log(GUAC_LOG_INFO, "Guacamole video encoder (guacenc) "
            "version " VERSION);

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58, 10, 100)
    /* Prepare libavcodec */
    avcodec_register_all();
#endif

#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58, 9, 100)
    av_register_all();
#endif

    /* Track number of overall failures */
    int total_files = argc - optind;
    int failures = 0;

    /* Abort if no files given */
    if (total_files <= 0) {
        guacenc_log(GUAC_LOG_INFO, "No input files specified. Nothing to do.");
        return 0;
    }

    /* A single explicit output cannot receive the encodings of multiple
     * input files */
    if (out_file != NULL && total_files != 1) {
        guacenc_log(GUAC_LOG_ERROR, "Only one input file may be given if "
                "the output is specified with the -o option.");
        goto invalid_options;
    }

    guacenc_log(GUAC_LOG_INFO, "%i input file(s) provided.", total_files);

    guacenc_log(GUAC_LOG_INFO, "Video will be encoded at %ix%i "
            "and %i bps.", width, height, bitrate);

    /* Encode all input files */
    for (i = optind; i < argc; i++) {

        /* Get current filename */
        const char* path = argv[i];

        /* Use the explicitly-provided output path, if any, streaming to
         * STDOUT if "-" was given as the output */
        const char* out_path;
        char out_buffer[4096];
        if (out_file != NULL)
            out_path = strcmp(out_file, "-") ? out_file : GUACENC_STDOUT_PATH;

        /* Otherwise, generate output filename from input filename */
        else {

            int len = snprintf(out_buffer, sizeof(out_buffer), "%s.m4v", path);

            /* Do not write if filename exceeds maximum length */
            if (len >= sizeof(out_buffer)) {
                guacenc_log(GUAC_LOG_ERROR, "Cannot write output file for "
                        "\"%s\": Name too long", path);
                continue;
            }

            out_path = out_buffer;

        }

        /* Attempt encoding, log granular success/failure at debug level */
        if (guacenc_encode(path, out_path, "mpeg4",
                    width, height, bitrate, force)) {
            failures++;
            guacenc_log(GUAC_LOG_DEBUG,
                    "%s was NOT successfully encoded.", path);
        }
        else
            guacenc_log(GUAC_LOG_DEBUG, "%s was successfully encoded.", path);

    }

    /* Warn if at least one file failed */
    if (failures != 0)
        guacenc_log(GUAC_LOG_WARNING, "Encoding failed for %i of %i file(s).",
                failures, total_files);

    /* Notify of success */
    else
        guacenc_log(GUAC_LOG_INFO, "All files encoded successfully.");

    /* Encoding complete */
    return 0;

    /* Display usage and exit with error if options are invalid */
invalid_options:

    fprintf(stderr, "USAGE: %s"
            " [-s WIDTHxHEIGHT]"
            " [-r BITRATE]"
            " [-f]"
            " [-o OUTPUT]"
            " [FILE]...\n", argv[0]);

    return 1;

}

