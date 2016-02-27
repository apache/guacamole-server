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
#include "display.h"
#include "log.h"

#include <guacamole/client.h>
#include <guacamole/timestamp.h>

#include <inttypes.h>
#include <stdlib.h>

/**
 * Parses a guac_timestamp from the given string. The string is assumed to
 * consist solely of decimal digits with an optional leading minus sign. If the
 * given string contains other characters, the behavior of this function is
 * undefined.
 *
 * @param str
 *     The string to parse, which must contain only decimal digits and an
 *     optional leading minus sign.
 *
 * @return
 *     A guac_timestamp having the same value as the provided string.
 */
static guac_timestamp guacenc_parse_timestamp(const char* str) {

    int sign = 1;
    int64_t num = 0;

    for (; *str != '\0'; str++) {

        /* Flip sign for each '-' encountered */
        if (*str == '-')
            sign = -sign;

        /* If not '-', assume the character is a digit */
        else
            num = num * 10 + (*str - '0');

    }

    return (guac_timestamp) (num * sign);

}

int guacenc_handle_sync(guacenc_display* display, int argc, char** argv) {

    /* Verify argument count */
    if (argc < 1) {
        guacenc_log(GUAC_LOG_DEBUG, "\"sync\" instruction incomplete");
        return 1;
    }

    /* Parse arguments */
    guac_timestamp timestamp = guacenc_parse_timestamp(argv[0]);

    /* Update timestamp / flush frame */
    return guacenc_display_sync(display, timestamp);

}

