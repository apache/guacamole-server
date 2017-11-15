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

#include "common/cursor.h"
#include "common/display.h"
#include "common/surface.h"

#include <guacamole/client.h>
#include <guacamole/socket.h>

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

/**
 * Synchronizes all surfaces within the given linked list to the given socket.
 * If the provided pointer to the linked list is NULL, this function has no
 * effect.
 *
 * @param layers
 *     The head element of the linked list of layers to synchronize, which may
 *     be NULL if the list is currently empty.
 *
 * @param user
 *     The user receiving the layers.
 *
 * @param socket
 *     The socket over which each layer should be sent.
 */
static void guac_common_display_dup_layers(guac_common_display_layer* layers,
        guac_user* user, guac_socket* socket) {

    guac_common_display_layer* current = layers;

    /* Synchronize all surfaces in given list */
    while (current != NULL) {
        guac_common_surface_dup(current->surface, user, socket);
        current = current->next;
    }

}

/**
 * Frees all layers and associated surfaces within the given list, as well as
 * their corresponding list elements. If the provided pointer to the linked
 * list is NULL, this function has no effect.
 *
 * @param layers
 *     The head element of the linked list of layers to free, which may be NULL
 *     if the list is currently empty.
 *
 * @param client
 *     The client owning the layers wrapped by each of the layers in the list.
 */
static void guac_common_display_free_layers(guac_common_display_layer* layers,
        guac_client* client) {

    guac_common_display_layer* current = layers;

    /* Free each surface in given list */
    while (current != NULL) {

        guac_common_display_layer* next = current->next;
        guac_layer* layer = current->layer;

        /* Free surface */
        guac_common_surface_free(current->surface);

        /* Destroy layer within remotely-connected client */
        guac_protocol_send_dispose(client->socket, layer);

        /* Free layer or buffer depending on index */
        if (layer->index < 0)
            guac_client_free_buffer(client, layer);
        else if (layer->index > 0)
            guac_client_free_layer(client, layer);

        /* Free current element and advance to next */
        free(current);
        current = next;

    }

}

/**
 * Allocates a display and a cursor which are used to represent the remote
 * display and cursor.
 *
 * @param client
 *     The client owning the cursor.
 *
 * @param width
 *     The desired width of the display.
 *
 * @param height
 *     The desired height of the display.
 *
 * @return
 *     The newly-allocated display or NULL if display cannot be allocated.
 */
guac_common_display* guac_common_display_alloc(guac_client* client,
        int width, int height) {

    /* Allocate display */
    guac_common_display* display = malloc(sizeof(guac_common_display));
    if (display == NULL)
        return NULL;

    /* Allocate shared cursor */
    display->cursor = guac_common_cursor_alloc(client);
    if (display->cursor == NULL) {
        free(display);
        return NULL;
    }

    pthread_mutex_init(&display->_lock, NULL);

    /* Associate display with given client */
    display->client = client;

    display->default_surface = guac_common_surface_alloc(client,
            client->socket, GUAC_DEFAULT_LAYER, width, height);

    /* No initial layers or buffers */
    display->layers = NULL;
    display->buffers = NULL;

    return display;

}

void guac_common_display_free(guac_common_display* display) {

    /* Free shared cursor */
    guac_common_cursor_free(display->cursor);

    /* Free default surface */
    guac_common_surface_free(display->default_surface);

    /* Free all layers and buffers */
    guac_common_display_free_layers(display->buffers, display->client);
    guac_common_display_free_layers(display->layers, display->client);

    pthread_mutex_destroy(&display->_lock);
    free(display);

}

void guac_common_display_dup(guac_common_display* display, guac_user* user,
        guac_socket* socket) {

    pthread_mutex_lock(&display->_lock);

    /* Sunchronize shared cursor */
    guac_common_cursor_dup(display->cursor, user, socket);

    /* Synchronize default surface */
    guac_common_surface_dup(display->default_surface, user, socket);

    /* Synchronize all layers and buffers */
    guac_common_display_dup_layers(display->layers, user, socket);
    guac_common_display_dup_layers(display->buffers, user, socket);

    pthread_mutex_unlock(&display->_lock);

}

void guac_common_display_flush(guac_common_display* display) {

    pthread_mutex_lock(&display->_lock);

    guac_common_display_layer* current = display->layers;

    /* Flush all surfaces */
    while (current != NULL) {
        guac_common_surface_flush(current->surface);
        current = current->next;
    }

    guac_common_surface_flush(display->default_surface);

    pthread_mutex_unlock(&display->_lock);

}

/**
 * Allocates and inserts a new element into the given linked list of display
 * layers, associating it with the given layer and surface.
 *
 * @param head
 *     A pointer to the head pointer of the list of layers. The head pointer
 *     will be updated by this function to point to the newly-allocated
 *     display layer.
 *
 * @param layer
 *     The Guacamole layer to associated with the new display layer.
 *
 * @param surface
 *     The surface associated with the given Guacamole layer and which should
 *     be associated with the new display layer.
 *
 * @return
 *     The newly-allocated display layer, which has been associated with the
 *     provided layer and surface.
 */
static guac_common_display_layer* guac_common_display_add_layer(
        guac_common_display_layer** head, guac_layer* layer,
        guac_common_surface* surface) {

    guac_common_display_layer* old_head = *head;

    guac_common_display_layer* display_layer =
        malloc(sizeof(guac_common_display_layer));

    /* Init layer/surface pair */
    display_layer->layer = layer;
    display_layer->surface = surface;

    /* Insert list element as the new head */
    display_layer->prev = NULL;
    display_layer->next = old_head;
    *head = display_layer;

    /* Update old head to point to new element, if it existed */
    if (old_head != NULL)
        old_head->prev = display_layer;

    return display_layer;

}

/**
 * Removes the given display layer from the linked list whose head pointer is
 * provided.
 *
 * @param head
 *     A pointer to the head pointer of the list of layers. The head pointer
 *     will be updated by this function if necessary, and will be set to NULL
 *     if the display layer being removed is the only layer in the list.
 *
 * @param display_layer
 *     The display layer to remove from the given list.
 */
static void guac_common_display_remove_layer(guac_common_display_layer** head,
        guac_common_display_layer* display_layer) {

    /* Update previous element, if it exists */
    if (display_layer->prev != NULL)
        display_layer->prev->next = display_layer->next;

    /* If there is no previous element, update the list head */
    else
        *head = display_layer->next;

    /* Update next element, if it exists */
    if (display_layer->next != NULL)
        display_layer->next->prev = display_layer->prev;

}

guac_common_display_layer* guac_common_display_alloc_layer(
        guac_common_display* display, int width, int height) {

    pthread_mutex_lock(&display->_lock);

    /* Allocate Guacamole layer */
    guac_layer* layer = guac_client_alloc_layer(display->client);

    /* Allocate corresponding surface */
    guac_common_surface* surface = guac_common_surface_alloc(display->client,
            display->client->socket, layer, width, height);

    /* Add layer and surface to list */
    guac_common_display_layer* display_layer =
        guac_common_display_add_layer(&display->layers, layer, surface);

    pthread_mutex_unlock(&display->_lock);
    return display_layer;

}

guac_common_display_layer* guac_common_display_alloc_buffer(
        guac_common_display* display, int width, int height) {

    pthread_mutex_lock(&display->_lock);

    /* Allocate Guacamole buffer */
    guac_layer* buffer = guac_client_alloc_buffer(display->client);

    /* Allocate corresponding surface */
    guac_common_surface* surface = guac_common_surface_alloc(display->client,
            display->client->socket, buffer, width, height);

    /* Add buffer and surface to list */
    guac_common_display_layer* display_layer =
        guac_common_display_add_layer(&display->buffers, buffer, surface);

    pthread_mutex_unlock(&display->_lock);
    return display_layer;

}

void guac_common_display_free_layer(guac_common_display* display,
        guac_common_display_layer* display_layer) {

    pthread_mutex_lock(&display->_lock);

    /* Remove list element from list */
    guac_common_display_remove_layer(&display->layers, display_layer);

    /* Free associated layer and surface */
    guac_common_surface_free(display_layer->surface);
    guac_client_free_layer(display->client, display_layer->layer);

    /* Free list element */
    free(display_layer);

    pthread_mutex_unlock(&display->_lock);

}

void guac_common_display_free_buffer(guac_common_display* display,
        guac_common_display_layer* display_buffer) {

    pthread_mutex_lock(&display->_lock);

    /* Remove list element from list */
    guac_common_display_remove_layer(&display->buffers, display_buffer);

    /* Free associated layer and surface */
    guac_common_surface_free(display_buffer->surface);
    guac_client_free_buffer(display->client, display_buffer->layer);

    /* Free list element */
    free(display_buffer);

    pthread_mutex_unlock(&display->_lock);

}

