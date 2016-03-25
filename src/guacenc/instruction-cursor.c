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
#include "display.h"
#include "log.h"

#include <guacamole/client.h>

#include <stdlib.h>

int guacenc_handle_cursor(guacenc_display* display, int argc, char** argv) {

    /* Verify argument count */
    if (argc < 7) {
        guacenc_log(GUAC_LOG_WARNING, "\"cursor\" instruction incomplete");
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

    /* Nothing to do with cursor (yet) */
    guacenc_log(GUAC_LOG_DEBUG, "Ignoring cursor: hotspot (%i, %i) "
            "src_layer=%i (%i, %i) %ix%i", hotspot_x, hotspot_y,
            src_index, src_x, src_y, src_w, src_h);

    return 0;

}

