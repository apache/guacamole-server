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

#include "client.h"
#include "common/cursor.h"
#include "common/display.h"
#include "common/surface.h"
#include "spice.h"
#include "spice-constants.h"

#include <cairo/cairo.h>
#include <guacamole/client.h>
#include <guacamole/layer.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <spice-client-glib-2.0/spice-client.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <syslog.h>

void guac_spice_cursor_hide(SpiceChannel* channel, guac_client* client) {
    
    guac_client_log(client, GUAC_LOG_TRACE, "Hiding the cursor.");

    /* Set the cursor to a blank image, hiding it. */
    guac_spice_client* spice_client = (guac_spice_client*) client->data;
    guac_common_cursor_set_blank(spice_client->display->cursor);  
}

void guac_spice_cursor_move(SpiceChannel* channel, int x, int y,
        guac_client* client) {

    guac_client_log(client, GUAC_LOG_TRACE, "Cursor move signal received: %d, %d", x, y);

    /* Update the cursor with the new coordinates. */
    guac_spice_client* spice_client = (guac_spice_client*) client->data;
    guac_common_cursor_update(spice_client->display->cursor, client->__owner, x, y, spice_client->display->cursor->button_mask);
}

void guac_spice_cursor_reset(SpiceChannel* channel, guac_client* client) {
    guac_client_log(client, GUAC_LOG_DEBUG,
            "Cursor reset signal received, not yet implemented");
}

void guac_spice_cursor_set(SpiceChannel* channel, int width, int height,
        int x, int y, gpointer* rgba, guac_client* client) {

    guac_client_log(client, GUAC_LOG_TRACE, "Cursor set signal received.");

    guac_spice_client* spice_client = (guac_spice_client*) client->data;
    int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);

    /* Update stored cursor information */
    guac_common_cursor_set_argb(spice_client->display->cursor, x, y,
            (const unsigned char*) rgba, width, height, stride);

}
