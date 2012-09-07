
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is libguac.
 *
 * The Initial Developer of the Original Code is
 * Michael Jumper.
 * Portions created by the Initial Developer are Copyright (C) 2010
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "client.h"
#include "client-handlers.h"
#include "error.h"
#include "layer.h"
#include "plugin.h"
#include "pool.h"
#include "protocol.h"
#include "socket.h"
#include "time.h"

guac_layer __GUAC_DEFAULT_LAYER = {
    .index = 0,
    .uri = "layer://0",
};

const guac_layer* GUAC_DEFAULT_LAYER = &__GUAC_DEFAULT_LAYER;

guac_resource* guac_client_alloc_resource(guac_client* client) {

    /* Init new layer */
    guac_resource* resource = malloc(sizeof(guac_resource));
    resource->index = guac_pool_next_int(client->__resource_pool);
    resource->accept_handler = NULL;
    resource->reject_handler = NULL;
    resource->data = NULL;

    /* Resize resource map if necessary */
    if (resource->index >= client->__available_resource_slots) {
        client->__available_resource_slots = resource->index * 2;
        client->__resource_map = realloc(client->__resource_map,
                sizeof(guac_resource*) * client->__available_resource_slots);
    }

    /* Store resource in map */
    client->__resource_map[resource->index] = resource;

    return resource;

}

guac_layer* guac_client_alloc_layer(guac_client* client) {

    /* Init new layer */
    guac_layer* allocd_layer = malloc(sizeof(guac_layer));
    allocd_layer->index = guac_pool_next_int(client->__layer_pool)+1;
    allocd_layer->uri = malloc(64);
    snprintf(allocd_layer->uri, 64, "layer://%i", allocd_layer->index);

    return allocd_layer;

}

guac_layer* guac_client_alloc_buffer(guac_client* client) {

    /* Init new layer */
    guac_layer* allocd_layer = malloc(sizeof(guac_layer));
    allocd_layer->index = -guac_pool_next_int(client->__buffer_pool) - 1;
    allocd_layer->uri = malloc(64);
    snprintf(allocd_layer->uri, 64, "layer://%i", allocd_layer->index);

    return allocd_layer;

}

void guac_client_free_resource(guac_client* client, guac_resource* resource) {

    /* Release index to pool */
    guac_pool_free_int(client->__resource_pool, resource->index);

    /* Free resource */
    free(resource);

}

void guac_client_free_buffer(guac_client* client, guac_layer* layer) {

    /* Release index to pool */
    guac_pool_free_int(client->__buffer_pool, -layer->index - 1);

    /* Free layer */
    free(layer);

}

void guac_client_free_layer(guac_client* client, guac_layer* layer) {

    /* Release index to pool */
    guac_pool_free_int(client->__layer_pool, layer->index - 1);

    /* Free layer */
    free(layer);

}

guac_client* guac_client_alloc() {

    /* Allocate new client */
    guac_client* client = malloc(sizeof(guac_client));
    if (client == NULL) {
        guac_error = GUAC_STATUS_NO_MEMORY;
        guac_error_message = "Could not allocate memory for client";
        return NULL;
    }

    /* Init new client */
    memset(client, 0, sizeof(guac_client));

    client->last_received_timestamp =
        client->last_sent_timestamp = guac_timestamp_current();

    client->state = GUAC_CLIENT_RUNNING;

    /* Allocate buffer and layer pools */
    client->__buffer_pool = guac_pool_alloc(GUAC_BUFFER_POOL_INITIAL_SIZE);
    client->__layer_pool = guac_pool_alloc(GUAC_BUFFER_POOL_INITIAL_SIZE);

    /* Allocate resource pool */
    client->__resource_pool = guac_pool_alloc(0);
    client->__available_resource_slots = GUAC_RESOURCE_MAP_INITIAL_SIZE;
    client->__resource_map =
        malloc(sizeof(guac_resource*) * client->__available_resource_slots);

    return client;

}

void guac_client_free(guac_client* client) {

    if (client->free_handler) {

        /* FIXME: Errors currently ignored... */
        client->free_handler(client);

    }

    /* Free layer pools */
    guac_pool_free(client->__buffer_pool);
    guac_pool_free(client->__layer_pool);

    /* Free resource pool */
    guac_pool_free(client->__resource_pool);
    free(client->__resource_map);

    free(client);
}

int guac_client_handle_instruction(guac_client* client, guac_instruction* instruction) {

    /* For each defined instruction */
    __guac_instruction_handler_mapping* current = __guac_instruction_handler_map;
    while (current->opcode != NULL) {

        /* If recognized, call handler */
        if (strcmp(instruction->opcode, current->opcode) == 0)
            return current->handler(client, instruction);

        current++;
    }

    /* If unrecognized, ignore */
    return 0;

}

void vguac_client_log_info(guac_client* client, const char* format,
        va_list ap) {

    /* Call handler if defined */
    if (client->log_info_handler != NULL)
        client->log_info_handler(client, format, ap);

}

void vguac_client_log_error(guac_client* client, const char* format,
        va_list ap) {

    /* Call handler if defined */
    if (client->log_error_handler != NULL)
        client->log_error_handler(client, format, ap);

}

void guac_client_log_info(guac_client* client, const char* format, ...) {

    va_list args;
    va_start(args, format);

    vguac_client_log_info(client, format, args);

    va_end(args);

}

void guac_client_log_error(guac_client* client, const char* format, ...) {

    va_list args;
    va_start(args, format);

    vguac_client_log_error(client, format, args);

    va_end(args);

}

void guac_client_stop(guac_client* client) {
    client->state = GUAC_CLIENT_STOPPING;
}

