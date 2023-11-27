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

#ifndef GUAC_VNC_DISPLAY_H
#define GUAC_VNC_DISPLAY_H

#include "config.h"

#include <rfb/rfbclient.h>
#include <rfb/rfbproto.h>

/**
 * Display size update module for VNC.
 */
typedef struct guac_vnc_display {

    /**
     * The guac_client instance handling the relevant VNC connection.
     */
    guac_client* client;

    /**
     * The timestamp of the last display update request, or 0 if no request
     * has been sent yet.
     */
    guac_timestamp last_request;

    /**
     * The last requested screen width, in pixels.
     */
    int requested_width;

    /**
     * The last requested screen height, in pixels.
     */
    int requested_height;

} guac_vnc_display;

/**
 * Callback invoked by libVNCServer when it receives a new binary image data
 * from the VNC server. The image itself will be stored in the designated sub-
 * rectangle of client->framebuffer.
 *
 * @param client
 *     The VNC client associated with the VNC session in which the new image
 *     was received.
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
 */
void guac_vnc_update(rfbClient* client, int x, int y, int w, int h);

/**
 * Callback invoked by libVNCServer when it receives a CopyRect message.
 * CopyRect specified a rectangle of source data within the display and a
 * set of X/Y coordinates to which that rectangle should be copied.
 *
 * @param client
 *     The VNC client associated with the VNC session in which the CopyRect
 *     was received.
 *
 * @param src_x
 *     The X coordinate of the upper-left corner of the source rectangle
 *     from which the image data should be copied, in pixels.
 *
 * @param src_y
 *     The Y coordinate of the upper-left corner of the source rectangle
 *     from which the image data should be copied, in pixels.
 *
 * @param w
 *     The width of the source and destination rectangles, in pixels.
 *
 * @param h
 *     The height of the source and destination rectangles, in pixels.
 *
 * @param dest_x
 *     The X coordinate of the upper-left corner of the destination rectangle
 *     in which the copied image data should be drawn, in pixels.
 *
 * @param dest_y
 *     The Y coordinate of the upper-left corner of the destination rectangle
 *     in which the copied image data should be drawn, in pixels.
 */
void guac_vnc_copyrect(rfbClient* client, int src_x, int src_y, int w, int h,
        int dest_x, int dest_y);

/**
 * Allocates a new VNC display update module, which will keep track of the data
 * needed to handle display updates.
 *
 * @param client
 *     The guac_client instance handling the relevant VNC connection.
 *
 * @return
 *     A newly-allocated VNC display update module.
 */
guac_vnc_display* guac_vnc_display_update_alloc(guac_client* client);

/**
 * Frees the resources associated with the data structure that keeps track of
 * items related to VNC display updates. Only resources specific to Guacamole
 * are freed. Resources that are part of the rfbClient will be freed separately.
 * If no resources are currently allocated for Display Update support, this
 * function has no effect.
 *
 * @param display
 *     The display update module to free.
 */
void guac_vnc_display_update_free(guac_vnc_display* display);

/**
 * Attempts to set the display size of the remote server to the size requested
 * by the client, usually as part of a client (browser) resize, if supported by
 * both the VNC client and the remote server.
 *
 * @param display
 *     The VNC display update object that tracks information related to display
 *     update requests.
 *
 * @param rfb_client
 *     The data structure containing the VNC client that is used by this
 *     connection.
 *
 * @param width
 *     The width that is being requested, in pixels.
 *
 * @param height
 *     The height that is being requested, in pixels.
 */
void guac_vnc_display_set_size(guac_vnc_display* display, rfbClient* rfb_client,
        int width, int height);

/**
 * Sets the pixel format to request of the VNC server. The request will be made
 * during the connection handshake with the VNC server using the values
 * specified by this function. Note that the VNC server is not required to
 * honor this request.
 *
 * @param client
 *     The VNC client associated with the VNC session whose desired pixel
 *     format should be set.
 *
 * @param color_depth
 *     The desired new color depth, in bits per pixel. Valid values are 8, 16,
 *     24, and 32.
 */
void guac_vnc_set_pixel_format(rfbClient* client, int color_depth);

/**
 * Overridden implementation of the rfb_MallocFrameBuffer function invoked by
 * libVNCServer when the display is being resized (or initially allocated).
 *
 * @param client
 *     The VNC client associated with the VNC session whose display needs to be
 *     allocated or reallocated.
 *
 * @return
 *     The original value returned by rfb_MallocFrameBuffer().
 */
rfbBool guac_vnc_malloc_framebuffer(rfbClient* rfb_client);

#endif

