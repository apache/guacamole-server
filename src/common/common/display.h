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

#ifndef GUAC_COMMON_DISPLAY_H
#define GUAC_COMMON_DISPLAY_H

#include "cursor.h"
#include "surface.h"

#include <guacamole/client.h>
#include <guacamole/socket.h>

#include <pthread.h>

/**
 * A list element representing a pairing of a Guacamole layer with a
 * corresponding guac_common_surface which wraps that layer. Adjacent layers
 * within the same list are pointed to with traditional prev/next pointers. The
 * order of layers in lists need not correspond in any way to the natural
 * ordering of those layers' indexes nor their stacking order (Z-order) within
 * the display.
 */
typedef struct guac_common_display_layer guac_common_display_layer;

struct guac_common_display_layer {

    /**
     * A Guacamole layer.
     */
    guac_layer* layer;

    /**
     * The surface which wraps the associated layer.
     */
    guac_common_surface* surface;

    /**
     * The layer immediately prior to this layer within the list containing
     * this layer, or NULL if this is the first layer/buffer in the list.
     */
    guac_common_display_layer* prev;

    /**
     * The layer immediately following this layer within the list containing
     * this layer, or NULL if this is the last layer/buffer in the list.
     */
    guac_common_display_layer* next;

};

/**
 * Abstracts a remote Guacamole display, having an associated client,
 * default surface, mouse cursor, and various allocated buffers and layers.
 */
typedef struct guac_common_display {

    /**
     * The client associate with this display.
     */
    guac_client* client;

    /**
     * The default surface of the client display.
     */
    guac_common_surface* default_surface;

    /**
     * Client-wide cursor, synchronized across all users.
     */
    guac_common_cursor* cursor;

    /**
     * The first element within a linked list of all currently-allocated
     * layers, or NULL if no layers are currently allocated. The default layer,
     * layer #0, is stored within default_surface and will not have a
     * corresponding element within this list.
     */
    guac_common_display_layer* layers;

    /**
     * The first element within a linked list of all currently-allocated
     * buffers, or NULL if no buffers are currently allocated.
     */
    guac_common_display_layer* buffers;

    /**
     * Non-zero if all graphical updates for this display should use lossless
     * compression, 0 otherwise. By default, newly-created displays will use
     * lossy compression when heuristics determine it is appropriate.
     */
    int lossless;

    /**
     * Mutex which is locked internally when access to the display must be
     * synchronized. All public functions of guac_common_display should be
     * considered threadsafe.
     */
    pthread_mutex_t _lock;

} guac_common_display;

/**
 * Allocates a new display, abstracting the cursor and buffer/layer allocation
 * operations of the given guac_client such that client state can be easily
 * synchronized to joining users.
 *
 * @param client
 *     The guac_client to associate with this display.
 *
 * @param width
 *     The initial width of the display, in pixels.
 *
 * @param height
 *     The initial height of the display, in pixels.
 *
 * @return
 *     The newly-allocated display.
 */
guac_common_display* guac_common_display_alloc(guac_client* client,
        int width, int height);

/**
 * Frees the given display, and any associated resources, including any
 * allocated buffers/layers.
 *
 * @param display
 *     The display to free.
 */
void guac_common_display_free(guac_common_display* display);

/**
 * Duplicates the state of the given display to the given socket. Any pending
 * changes to buffers, layers, or the default layer are not flushed.
 *
 * @param display
 *     The display whose state should be sent along the given socket.
 *
 * @param user
 *     The user receiving the display state.
 *
 * @param socket
 *     The socket over which the display state should be sent.
 */
void guac_common_display_dup(guac_common_display* display, guac_user* user,
        guac_socket* socket);

/**
 * Flushes pending changes to the given display. All pending operations will
 * become visible to any connected users.
 *
 * @param display
 *     The display to flush.
 */
void guac_common_display_flush(guac_common_display* display);

/**
 * Allocates a new layer, returning a new wrapped layer and corresponding
 * surface. The layer may be reused from a previous allocation, if that layer
 * has since been freed.
 *
 * @param display
 *     The display to allocate a new layer from.
 *
 * @param width
 *     The width of the layer to allocate, in pixels.
 *
 * @param height
 *     The height of the layer to allocate, in pixels.
 *
 * @return
 *     A newly-allocated layer.
 */
guac_common_display_layer* guac_common_display_alloc_layer(
        guac_common_display* display, int width, int height);

/**
 * Allocates a new buffer, returning a new wrapped buffer and corresponding
 * surface. The buffer may be reused from a previous allocation, if that buffer
 * has since been freed.
 *
 * @param display
 *     The display to allocate a new buffer from.
 *
 * @param width
 *     The width of the buffer to allocate, in pixels.
 *
 * @param height
 *     The height of the buffer to allocate, in pixels.
 *
 * @return
 *     A newly-allocated buffer.
 */
guac_common_display_layer* guac_common_display_alloc_buffer(
        guac_common_display* display, int width, int height);

/**
 * Frees the given surface and associated layer, returning the layer to the
 * given display for future use.
 *
 * @param display
 *     The display originally allocating the layer.
 *
 * @param display_layer
 *     The layer to free.
 */
void guac_common_display_free_layer(guac_common_display* display,
        guac_common_display_layer* display_layer);

/**
 * Frees the given surface and associated buffer, returning the buffer to the
 * given display for future use.
 *
 * @param display
 *     The display originally allocating the buffer.
 *
 * @param display_buffer
 *     The buffer to free.
 */
void guac_common_display_free_buffer(guac_common_display* display,
        guac_common_display_layer* display_buffer);

/**
 * Sets the overall lossless compression policy of the given display to the
 * given value, affecting all current and future layers/buffers maintained by
 * the display. By default, newly-created displays will use lossy compression
 * for graphical updates when heuristics determine that doing so is
 * appropriate. Specifying a non-zero value here will force all graphical
 * updates to always use lossless compression, whereas specifying zero will
 * restore the default policy.
 *
 * Note that this can also be adjusted on a per-layer / per-buffer basis with
 * guac_common_surface_set_lossless().
 *
 * @param display
 *     The display to modify.
 *
 * @param lossless
 *     Non-zero if all graphical updates for this display should use lossless
 *     compression, 0 otherwise.
 */
void guac_common_display_set_lossless(guac_common_display* display,
        int lossless);

#endif

