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
#include "common/display.h"
#include "common/iconv.h"
#include "common/surface.h"
#include "spice.h"

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

void guac_spice_client_display_update(SpiceChannel* channel, int x,
        int y, int w, int h, guac_client* client) {

    guac_client_log(client, GUAC_LOG_TRACE,
            "Received request to update Spice display: %d, %d, %d, %d",
            x, y, w, h);
    guac_spice_client* spice_client = (guac_spice_client*) client->data;

    /* Retrieve the primary display buffer */
    SpiceDisplayPrimary primary;
    if (spice_display_channel_get_primary(channel, 0, &primary)) {

        /* Create cairo surface from decoded buffer */
        cairo_surface_t* surface = cairo_image_surface_create_for_data(
                primary.data,
                CAIRO_FORMAT_RGB24,
                primary.width,
                primary.height,
                primary.stride);

        /* A region smaller than the entire display should be updated. */
        if ((x > 0 || y > 0 ) && (w < primary.width || h < primary.height)) {

            cairo_surface_t* updateArea = cairo_surface_create_similar_image(surface, CAIRO_FORMAT_RGB24, w, h);
            cairo_t* updateContext = cairo_create(updateArea);
            cairo_set_operator(updateContext, CAIRO_OPERATOR_SOURCE);
            cairo_set_source_surface(updateContext, surface, 0 - x, 0 - y);
            cairo_rectangle(updateContext, 0, 0, w, h);
            cairo_fill(updateContext);
            guac_common_surface_draw(spice_client->display->default_surface, x, y, updateArea);
            cairo_surface_destroy(updateArea);

        }
    
        /* The entire display should be updated. */
        else {
            guac_common_surface_draw(spice_client->display->default_surface,
                    0, 0, surface);
        }

        /* Free surfaces */
        cairo_surface_destroy(surface);
    }

    /* Flush surface and mark end of frame. */
    guac_common_surface_flush(spice_client->display->default_surface);
    guac_client_end_frame(client);
    guac_socket_flush(client->socket);
}

void guac_spice_client_display_gl_draw(SpiceChannel* channel, int x,
        int y, int w, int h, guac_client* client) {

    guac_client_log(client, GUAC_LOG_TRACE, "Received GL draw request.");

    guac_spice_client* spice_client = (guac_spice_client*) client->data;

    /* Copy specified rectangle within default layer */
    guac_common_surface_copy(spice_client->display->default_surface,
            x, y, w, h,
            spice_client->display->default_surface, x, y);
    

}

void guac_spice_client_display_mark(SpiceChannel* channel, gint mark,
        guac_client* client) {
    
    guac_client_log(client, GUAC_LOG_DEBUG,
            "Received signal to mark display, which currently has no effect.");
    
}

void guac_spice_client_display_primary_create(SpiceChannel* channel,
        gint format, gint width, gint height, gint stride, gint shmid,
        gpointer imgdata, guac_client* client) {
    
    guac_client_log(client, GUAC_LOG_DEBUG, "Received request to create primary display.");

    /* Allocate the Guacamole display. */
    guac_spice_client* spice_client = (guac_spice_client*) client->data;
    spice_client->display = guac_common_display_alloc(client,
            width, height);

    /* Create a matching Cairo image surface. */
    guac_client_log(client, GUAC_LOG_TRACE, "Creating Cairo image surface.");
    cairo_surface_t* surface = cairo_image_surface_create_for_data(imgdata, CAIRO_FORMAT_RGB24,
            width, height, stride);

    /* Draw directly to default layer */
    guac_client_log(client, GUAC_LOG_TRACE, "Drawing to the default surface.");
    guac_common_surface_draw(spice_client->display->default_surface,
            0, 0, surface);

    /* Flush the default surface. */
    guac_client_log(client, GUAC_LOG_TRACE, "Flushing the default surface.");
    guac_common_surface_flush(spice_client->display->default_surface);

    /* Mark the end of the frame and flush the socket. */
    guac_client_end_frame(client);
    guac_socket_flush(client->socket);
    
    
}

void guac_spice_client_display_primary_destroy(SpiceChannel* channel,
        guac_client* client) {
    
    guac_client_log(client, GUAC_LOG_DEBUG, "Received request to destroy the primary display.");

    /* Free the Guacamole display. */
    guac_spice_client* spice_client = (guac_spice_client*) client->data;
    guac_common_display_free(spice_client->display);
    
}
    
void* guac_spice_client_streaming_handler(SpiceChannel* channel,
        gboolean streaming_mode, guac_client* client) {
    
    guac_client_log(client, GUAC_LOG_DEBUG, "Received call to streaming handler.");

    guac_spice_client* spice_client = (guac_spice_client*) client->data;
    return spice_client->display;
    
}


