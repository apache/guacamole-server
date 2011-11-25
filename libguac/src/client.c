
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

#include "log.h"
#include "guacio.h"
#include "protocol.h"
#include "client.h"
#include "client-handlers.h"

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

guac_layer __GUAC_DEFAULT_LAYER = {
    .index = 0,
    .__next = NULL,
    .__next_available = NULL
};

const guac_layer* GUAC_DEFAULT_LAYER = &__GUAC_DEFAULT_LAYER;

guac_client* __guac_alloc_client(GUACIO* io) {

    /* Allocate new client (not handoff) */
    guac_client* client = malloc(sizeof(guac_client));
    memset(client, 0, sizeof(guac_client));

    /* Init new client */
    client->io = io;
    client->last_received_timestamp = client->last_sent_timestamp = guac_current_timestamp();
    client->state = RUNNING;

    client->__all_layers        = NULL;
    client->__available_buffers = NULL;

    client->__next_buffer_index = -1;

    return client;
}

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

guac_client* guac_get_client(int client_fd, int usec_timeout) {

    guac_client* client;
    GUACIO* io = guac_open(client_fd);

    /* Pluggable client */
    char protocol_lib[256] = "libguac-client-";
    
    union {
        guac_client_init_handler* client_init;
        void* obj;
    } alias;

    char* error;

    /* Client args description */
    const char** client_args;

    /* Client arguments */
    int argc;
    char** argv;

    /* Instruction */
    guac_instruction* instruction;

    /* Wait for select instruction */
    for (;;) {

        int result;

        /* Wait for data until timeout */
        result = guac_instructions_waiting(io, usec_timeout);
        if (result == 0) {
            guac_send_error(io, "Select timeout.");
            guac_close(io);
            return NULL;
        }

        /* If error occurs while waiting, exit with failure */
        if (result < 0) {
            guac_close(io);
            return NULL;
        }

        instruction = guac_read_instruction(io, usec_timeout);
        if (instruction == NULL) {
            guac_close(io);
            return NULL;            
        }

        /* Select instruction read */
        else {

            if (strcmp(instruction->opcode, "select") == 0) {

                /* Get protocol from message */
                char* protocol = instruction->argv[0];

                strcat(protocol_lib, protocol);
                strcat(protocol_lib, ".so");

                /* Create new client */
                client = __guac_alloc_client(io);

                /* Load client plugin */
                client->__client_plugin_handle = dlopen(protocol_lib, RTLD_LAZY);
                if (!(client->__client_plugin_handle)) {
                    guac_log_error("Could not open client plugin for protocol \"%s\": %s\n", protocol, dlerror());
                    guac_send_error(io, "Could not load server-side client plugin.");
                    guac_flush(io);
                    guac_close(io);
                    guac_free_instruction(instruction);
                    return NULL;
                }

                dlerror(); /* Clear errors */

                /* Get init function */
                alias.obj = dlsym(client->__client_plugin_handle, "guac_client_init");

                if ((error = dlerror()) != NULL) {
                    guac_log_error("Could not get guac_client_init in plugin: %s\n", error);
                    guac_send_error(io, "Invalid server-side client plugin.");
                    guac_flush(io);
                    guac_close(io);
                    guac_free_instruction(instruction);
                    return NULL;
                }

                /* Get usage strig */
                client_args = (const char**) dlsym(client->__client_plugin_handle, "GUAC_CLIENT_ARGS");

                if ((error = dlerror()) != NULL) {
                    guac_log_error("Could not get GUAC_CLIENT_ARGS in plugin: %s\n", error);
                    guac_send_error(io, "Invalid server-side client plugin.");
                    guac_flush(io);
                    guac_close(io);
                    guac_free_instruction(instruction);
                    return NULL;
                }

                if (   /* Send args */
                       guac_send_args(io, client_args)
                    || guac_flush(io)
                   ) {
                    guac_close(io);
                    guac_free_instruction(instruction);
                    return NULL;
                }

                guac_free_instruction(instruction);
                break;

            } /* end if select */

            guac_free_instruction(instruction);
        }

    }

    /* Wait for connect instruction */
    for (;;) {

        int result;

        /* Wait for data until timeout */
        result = guac_instructions_waiting(io, usec_timeout);
        if (result == 0) {
            guac_send_error(io, "Connect timeout.");
            guac_close(io);
            return NULL;
        }

        /* If error occurs while waiting, exit with failure */
        if (result < 0) {
            guac_close(io);
            return NULL;
        }

        instruction = guac_read_instruction(io, usec_timeout);
        if (instruction == NULL) {
            guac_log_error("Error reading instruction while waiting for connect");
            guac_close(io);
            return NULL;            
        }

        /* Connect instruction read */
        else {

            if (strcmp(instruction->opcode, "connect") == 0) {

                /* Initialize client arguments */
                argc = instruction->argc;
                argv = instruction->argv;

                if (alias.client_init(client, argc, argv) != 0) {
                    /* NOTE: On error, proxy client will send appropriate error message */
                    guac_free_instruction(instruction);
                    guac_close(io);
                    return NULL;
                }

                guac_free_instruction(instruction);
                return client;

            } /* end if connect */

            guac_free_instruction(instruction);
        }

    }

}

void guac_free_client(guac_client* client) {

    if (client->free_handler) {
        if (client->free_handler(client))
            guac_log_error("Error calling client free handler");
    }

    guac_close(client->io);

    /* Unload client plugin */
    if (dlclose(client->__client_plugin_handle)) {
        guac_log_error("Could not close client plugin while unloading client: %s", dlerror());
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

