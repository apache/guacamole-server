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

#ifndef GUAC_SPICE_DISPLAY_H
#define GUAC_SPICE_DISPLAY_H

#include "config.h"

#include <spice-client-glib-2.0/spice-client.h>

/* Define cairo_format_stride_for_width() if missing */
#ifndef HAVE_CAIRO_FORMAT_STRIDE_FOR_WIDTH
#define cairo_format_stride_for_width(format, width) (width*4)
#endif

/**
 * Callback invoked by the Spice library when it receives a new binary image
 * data from the Spice server. The image itself will be stored in the designated
 * sub-rectangle of client->framebuffer.
 *
 * @param channel
 *     The SpiceDisplayChannel that received the update event.
 *
 * @param x
 *     The X coordinate of the upper-left corner of the destination rectangle
 *     in which the image should be drawn, in pixels.
 *
 * @param y
 *     The Y coordinate of the upper-left corner of the destination rectangle
 *     in which the image should be drawn, in pixels.
 *
 * @param w
 *     The width of the image, in pixels.
 *
 * @param h
 *     The height of the image, in pixels.
 * 
 * @param guac_client
 *     The guac_client associated with the event.
 */
void guac_spice_client_display_update(SpiceDisplayChannel* channel, int x,
        int y, int w, int h, guac_client* client);

/**
 * The callback function invoked when the RED_DISPLAY_MARK command is received
 * from the Spice server and the display should be exposed.
 * 
 * @param channel
 *     The SpiceDisplayChannel on which the event was received.
 * 
 * @param mark
 *     Non-zero when the display mark has been received.
 * 
 * @param client
 *     The guac_client associated with this channel and event.
 */
void guac_spice_client_display_mark(SpiceDisplayChannel* channel, gint mark,
        guac_client* client);

/**
 * The callback function invoked when primary display buffer data is sent from
 * the Spice server to the client.
 * 
 * @param channel
 *     The SpiceDisplayChannel on which this event was received.
 * 
 * @param format
 *     The Spice format of the received data.
 * 
 * @param width
 *     The total width of the display.
 * 
 * @param height
 *     The total height of the display.
 * 
 * @param stride
 *     The buffer width padding.
 * 
 * @param shmid
 *     The identifier of the shared memory segment associated with the data, or
 *     -1 if shared memory is not in use.
 * 
 * @param imgdata
 *     A pointer to the buffer containing the surface data.
 * 
 * @param client
 *     The guac_client associated with this channel/event.
 */
void guac_spice_client_display_primary_create(SpiceDisplayChannel* channel,
        gint format, gint width, gint height, gint stride, gint shmid,
        gpointer imgdata, guac_client* client);

/**
 * The callback function invoked by the client when the primary surface is
 * destroyed and should no longer be accessed.
 * 
 * @param channel
 *     The SpiceDisplayChannel on which the primary surface destroy event was
 *     received.
 * 
 * @param client
 *     The guac_client associated with this channel/event.
 */
void guac_spice_client_display_primary_destroy(SpiceDisplayChannel* channel,
        guac_client* client);

/**
 * Callback function that is invoked when a channel specifies that a display
 * is marked as active and should be exposed.
 * 
 * @param channel
 *     The channel on which the mark was received.
 * 
 * @param marked
 *     TRUE if the display should be marked; otherwise false.
 * 
 * @param client
 *     The guac_client associated with the channel/event.
 */
void guac_spice_client_display_mark(SpiceDisplayChannel* channel,
        gboolean marked, guac_client* client);

/**
 * Callback invoked by the Spice client when it receives a CopyRect message.
 * CopyRect specified a rectangle of source data within the display and a
 * set of X/Y coordinates to which that rectangle should be copied.
 *
 * @param channel
 *     The SpiceDisplayChannel that received the gl_draw event.
 *
 * @param x
 *     The X coordinate of the upper-left corner of the source rectangle
 *     from which the image data should be copied, in pixels.
 *
 * @param y
 *     The Y coordinate of the upper-left corner of the source rectangle
 *     from which the image data should be copied, in pixels.
 *
 * @param w
 *     The width of the source and destination rectangles, in pixels.
 *
 * @param h
 *     The height of the source and destination rectangles, in pixels.
 * 
 * @param guac_client
 *     The guac_client associated with this gl_draw event.
 */
void guac_spice_client_display_gl_draw(SpiceDisplayChannel* channel, int x,
        int y, int w, int h, guac_client* client);

/**
 * The callback function invoked by the client when it receives a request to
 * change streaming mode.
 * 
 * @param channel
 *     The SpiceDisplayChannel that received the streaming mode change request.
 * 
 * @param streaming_mode
 *     TRUE if the display channel should be in streaming mode; otherwise FALSE.
 * 
 * @param guac_client
 *     The guac_client associated with this event.
 * 
 * @return
 *     A pointer to the display.
 */
void* guac_spice_client_streaming_handler(SpiceDisplayChannel* channel,
        gboolean streaming_mode, guac_client* client);

#endif /* GUAC_SPICE_DISPLAY_H */

