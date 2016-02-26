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
#include "log.h"

int main(int argc, char* argv[]) {

    int i;

    /* Log start */
    guacenc_log(GUAC_LOG_INFO, "Guacamole video encoder (guacenc) "
            "version " VERSION);

    /* Abort if no files given */
    if (argc == 1) {
        guacenc_log(GUAC_LOG_INFO, "No input files specified. Nothing to do.");
        return 0;
    }

    /* Track number of overall failures */
    int failures = 0;

    /* Encode all input files */
    for (i = 1; i < argc; i++) {

        /* Get current filename */
        const char* path = argv[i];

        /* Attempt encoding, log granular success/failure at debug level */
        if (guacenc_encode(path)) {
            failures++;
            guacenc_log(GUAC_LOG_DEBUG,
                    "%s was NOT successfully encoded.", path);
        }
        else
            guacenc_log(GUAC_LOG_DEBUG, "%s was successfully encoded.", path);

    }

    /* Warn if at least one file failed */
    if (failures != 0)
        guacenc_log(GUAC_LOG_WARNING, "Encoding failed for %i file(s).",
                failures);

    /* Notify of success */
    else
        guacenc_log(GUAC_LOG_INFO, "All files encoded successfully.");

    /* Encoding complete */
    return 0;

}

