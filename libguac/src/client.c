
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>

#include "socket.h"
#include "protocol.h"
#include "client.h"
#include "client-handlers.h"
#include "error.h"

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

guac_layer __GUAC_DEFAULT_LAYER = {
    .index = 0,
    .__next = NULL,
    .__next_available = NULL
};

const guac_layer* GUAC_DEFAULT_LAYER = &__GUAC_DEFAULT_LAYER;

guac_layer* guac_client_alloc_layer(guac_client* client, int index) {

    guac_layer* allocd_layer;

    /* Init new layer */
    allocd_layer = malloc(sizeof(guac_layer));

    /* Add to __all_layers list */
    allocd_layer->__next = client->__all_layers;
    client->__all_layers = allocd_layer;

    allocd_layer->index = index;
    return allocd_layer;

}

guac_layer* guac_client_alloc_buffer(guac_client* client) {

    guac_layer* allocd_layer;

    /* If available layers, pop off first available buffer */
    if (client->__available_buffers != NULL) {
        allocd_layer = client->__available_buffers;
        client->__available_buffers = client->__available_buffers->__next_available;
        allocd_layer->__next_available = NULL;
    }

    /* If no available buffer, allocate new buffer, add to __all_layers list */
    else {

        /* Init new layer */
        allocd_layer = malloc(sizeof(guac_layer));
        allocd_layer->index = client->__next_buffer_index--;

        /* Add to __all_layers list */
        allocd_layer->__next = client->__all_layers;
        client->__all_layers = allocd_layer;

    }

    return allocd_layer;

}

void guac_client_free_buffer(guac_client* client, guac_layer* layer) {

    /* Add layer to pool of available buffers */
    layer->__next_available = client->__available_buffers;
    client->__available_buffers = layer;

}

guac_client_plugin* guac_client_plugin_open(const char* protocol) {

    guac_client_plugin* plugin;

    /* Reference to dlopen()'d plugin */
    void* client_plugin_handle;

    /* Client args description */
    const char** client_args;

    /* Pluggable client */
    char protocol_lib[256] = "libguac-client-";
    
    union {
        guac_client_init_handler* client_init;
        void* obj;
    } alias;

    /* Add protocol and .so suffix to protocol_lib */
    strcat(protocol_lib, protocol);
    strcat(protocol_lib, ".so");

    /* Load client plugin */
    client_plugin_handle = dlopen(protocol_lib, RTLD_LAZY);
    if (!client_plugin_handle) {
        guac_error = GUAC_STATUS_BAD_ARGUMENT;
        guac_error_message = dlerror();
        return NULL;
    }

    dlerror(); /* Clear errors */

    /* Get init function */
    alias.obj = dlsym(client_plugin_handle, "guac_client_init");

    /* Fail if cannot find guac_client_init */
    if (dlerror() != NULL) {
        guac_error = GUAC_STATUS_BAD_ARGUMENT;
        guac_error_message = dlerror();
        return NULL;
    }

    /* Get usage strig */
    client_args = (const char**) dlsym(client_plugin_handle, "GUAC_CLIENT_ARGS");

    /* Fail if cannot find GUAC_CLIENT_ARGS */
    if (dlerror() != NULL) {
        guac_error = GUAC_STATUS_BAD_ARGUMENT;
        guac_error_message = dlerror();
        return NULL;
    }

    /* Allocate plugin */
    plugin = malloc(sizeof(guac_client_plugin));
    if (plugin == NULL) {
        guac_error = GUAC_STATUS_NO_MEMORY;
        guac_error_message = "Could not allocate memory for client plugin";
        return NULL;
    } 

    /* Init and return plugin */
    plugin->__client_plugin_handle = client_plugin_handle;
    plugin->init_handler = alias.client_init;
    plugin->args = client_args;
    return plugin;

}

int guac_client_plugin_close(guac_client_plugin* plugin) {

    /* Unload client plugin */
    if (dlclose(plugin->__client_plugin_handle)) {
        guac_error = GUAC_STATUS_BAD_STATE;
        guac_error_message = dlerror();
        return -1;
    }

    /* Free plugin handle */
    free(plugin);
    return 0;

}

guac_client* guac_client_plugin_get_client(guac_client_plugin* plugin,
        guac_socket* socket, int argc, char** argv) {

    /* Allocate new client */
    guac_client* client = malloc(sizeof(guac_client));
    if (client == NULL) {
        guac_error = GUAC_STATUS_NO_MEMORY;
        guac_error_message = "Could not allocate memory for client";
        return NULL;
    }

    /* Init new client */
    memset(client, 0, sizeof(guac_client));

    client->socket = socket;
    client->last_received_timestamp =
        client->last_sent_timestamp = guac_protocol_get_timestamp();

    client->state = GUAC_CLIENT_RUNNING;

    client->__all_layers        = NULL;
    client->__available_buffers = NULL;

    client->__next_buffer_index = -1;

    if (plugin->init_handler(client, argc, argv) != 0) {
        free(client);
        return NULL;
    }

    return client;

}

void guac_client_free(guac_client* client) {

    if (client->free_handler) {

        /* FIXME: Errors currently ignored... */
        client->free_handler(client);

    }

    /* Free all layers */
    while (client->__all_layers != NULL) {

        /* Get layer, update layer pool head */
        guac_layer* layer = client->__all_layers;
        client->__all_layers = layer->__next;

        /* Free layer */
        free(layer);

    }

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

