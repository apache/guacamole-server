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
#include "log.h"

#include <guacamole/client.h>

#include <stdlib.h>

int guacenc_handle_cursor(int argc, char** argv) {

    /* Verify argument count */
    if (argc < 7) {
        guacenc_log(GUAC_LOG_DEBUG, "\"cursor\" instruction incomplete");
        return 1;
    }

    /* Parse arguments */
    int hotspot_x = atoi(argv[0]);
    int hotspot_y = atoi(argv[1]);
    int src_index = atoi(argv[2]);
    int src_x = atoi(argv[3]);
    int src_y = atoi(argv[4]);
    int src_w = atoi(argv[5]);
    int src_h = atoi(argv[6]);

    /* STUB */
    guacenc_log(GUAC_LOG_DEBUG, "cursor: hotspot (%i, %i) "
            "src_layer=%i (%i, %i) %ix%i", hotspot_x, hotspot_y,
            src_index, src_x, src_y, src_w, src_h);
    return 0;

}

