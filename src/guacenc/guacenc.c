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

#include "encode.h"
#include "guacenc.h"
#include "log.h"
#include "parse.h"

#include <libavcodec/avcodec.h>

#include <getopt.h>
#include <stdio.h>

int main(int argc, char* argv[]) {

    int i;

    /* Load defaults */
    const char* codec = GUACENC_DEFAULT_CODEC;
    const char* suffix = GUACENC_DEFAULT_SUFFIX;
    int width = GUACENC_DEFAULT_WIDTH;
    int height = GUACENC_DEFAULT_HEIGHT;
    int bitrate = GUACENC_DEFAULT_BITRATE;

    /* Parse arguments */
    int opt;
    while ((opt = getopt(argc, argv, "V:s:d:r:")) != -1) {

        /* -V: Video codec */
        if (opt == 'V')
            codec = optarg;

        /* -s: Output file suffix */
        else if (opt == 's')
            suffix = optarg;

        /* -d: Dimensions */
        else if (opt == 'd') {
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

        /* Invalid option */
        else {
            goto invalid_options;
        }

    }

    /* Log start */
    guacenc_log(GUAC_LOG_INFO, "Guacamole video encoder (guacenc) "
            "version " VERSION);

    /* Prepare libavcodec */
    avcodec_register_all();

    /* Track number of overall failures */
    int total_files = argc - optind;
    int failures = 0;

    /* Abort if no files given */
    if (total_files <= 0) {
        guacenc_log(GUAC_LOG_INFO, "No input files specified. Nothing to do.");
        return 0;
    }

    guacenc_log(GUAC_LOG_INFO, "%i input file(s) provided.", total_files);

    guacenc_log(GUAC_LOG_INFO, "Video will be encoded as \"%s\" at %ix%i "
            "and %i bps.", codec, width, height, bitrate);

    guacenc_log(GUAC_LOG_INFO, "Output files will end with \".%s\".", suffix);


    /* Encode all input files */
    for (i = optind; i < argc; i++) {

        /* Get current filename */
        const char* path = argv[i];

        /* Generate output filename */
        char out_path[4096];
        int len = snprintf(out_path, sizeof(out_path), "%s.%s", path, suffix);

        /* Do not write if filename exceeds maximum length */
        if (len >= sizeof(out_path)) {
            guacenc_log(GUAC_LOG_ERROR, "Cannot write output file \"%s.%s\": "
                    "Name too long", path, suffix);
            continue;
        }

        /* Attempt encoding, log granular success/failure at debug level */
        if (guacenc_encode(path, out_path, codec, width, height, bitrate)) {
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
            " [-d WIDTHxHEIGHT]"
            " [-s SUFFIX]"
            " [-V CODEC]"
            " [-b BITRATE]\n", argv[0]);

    return 1;

}

