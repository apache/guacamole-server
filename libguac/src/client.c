
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

#include "thread.h"
#include "log.h"
#include "guacio.h"
#include "protocol.h"
#include "client.h"
#include "client-handlers.h"

guac_layer __GUAC_DEFAULT_LAYER = {
    .index = 0,
    .next = NULL,
    .next_available = NULL
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

    client->all_layers         = NULL;
    client->available_layers   = NULL;
    client->available_buffers = NULL;

    client->next_buffer_index = -1;

    return client;
}

guac_layer* guac_client_alloc_layer(guac_client* client, int index) {

    guac_layer* allocd_layer;

    /* If available layers, pop off first available layer */
    if (client->available_layers != NULL) {
        allocd_layer = client->available_layers;
        client->available_layers = client->available_layers->next_available;
        allocd_layer->next_available = NULL;
    }

    /* If no available layers, allocate new layer, add to all_layers list */
    else {

        /* Init new layer */
        allocd_layer = malloc(sizeof(guac_layer));

        /* Add to all_layers list */
        allocd_layer->next = client->all_layers;
        client->all_layers = allocd_layer;

    }

    allocd_layer->index = index;
    return allocd_layer;

}

void guac_client_free_layer(guac_client* client, guac_layer* layer) {
    layer->next = client->available_layers;
    client->available_layers = layer;
}

guac_layer* guac_client_alloc_buffer(guac_client* client) {

    guac_layer* allocd_layer;

    /* If available layers, pop off first available buffer */
    if (client->available_buffers != NULL) {
        allocd_layer = client->available_buffers;
        client->available_buffers = client->available_buffers->next_available;
        allocd_layer->next_available = NULL;
    }

    /* If no available buffer, allocate new buffer, add to all_layers list */
    else {

        /* Init new layer */
        allocd_layer = malloc(sizeof(guac_layer));
        allocd_layer->index = client->next_buffer_index--;

        /* Add to all_layers list */
        allocd_layer->next = client->all_layers;
        client->all_layers = allocd_layer;

    }

    return allocd_layer;

}

void guac_client_free_buffer(guac_client* client, guac_layer* layer) {
    layer->next = client->available_buffers;
    client->available_buffers = layer;
}

guac_client* guac_get_client(int client_fd) {

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
    guac_instruction instruction;

    /* Wait for select instruction */
    for (;;) {

        int result;

        /* Wait for data until timeout */
        result = guac_instructions_waiting(io);
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

        result = guac_read_instruction(io, &instruction);
        if (result < 0) {
            guac_close(io);
            return NULL;            
        }

        /* Select instruction read */
        if (result > 0) {

            if (strcmp(instruction.opcode, "select") == 0) {

                /* Get protocol from message */
                char* protocol = instruction.argv[0];

                strcat(protocol_lib, protocol);
                strcat(protocol_lib, ".so");

                /* Create new client */
                client = __guac_alloc_client(io);

                /* Load client plugin */
                client->client_plugin_handle = dlopen(protocol_lib, RTLD_LAZY);
                if (!(client->client_plugin_handle)) {
                    guac_log_error("Could not open client plugin for protocol \"%s\": %s\n", protocol, dlerror());
                    guac_send_error(io, "Could not load server-side client plugin.");
                    guac_flush(io);
                    guac_close(io);
                    guac_free_instruction_data(&instruction);
                    return NULL;
                }

                dlerror(); /* Clear errors */

                /* Get init function */
                alias.obj = dlsym(client->client_plugin_handle, "guac_client_init");

                if ((error = dlerror()) != NULL) {
                    guac_log_error("Could not get guac_client_init in plugin: %s\n", error);
                    guac_send_error(io, "Invalid server-side client plugin.");
                    guac_flush(io);
                    guac_close(io);
                    guac_free_instruction_data(&instruction);
                    return NULL;
                }

                /* Get usage strig */
                client_args = (const char**) dlsym(client->client_plugin_handle, "GUAC_CLIENT_ARGS");

                if ((error = dlerror()) != NULL) {
                    guac_log_error("Could not get GUAC_CLIENT_ARGS in plugin: %s\n", error);
                    guac_send_error(io, "Invalid server-side client plugin.");
                    guac_flush(io);
                    guac_close(io);
                    guac_free_instruction_data(&instruction);
                    return NULL;
                }

                if (   /* Send args */
                       guac_send_args(io, client_args)
                    || guac_flush(io)
                   ) {
                    guac_close(io);
                    guac_free_instruction_data(&instruction);
                    return NULL;
                }

                guac_free_instruction_data(&instruction);
                break;

            } /* end if select */

            guac_free_instruction_data(&instruction);
        }

    }

    /* Wait for connect instruction */
    for (;;) {

        int result;

        /* Wait for data until timeout */
        result = guac_instructions_waiting(io);
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

        result = guac_read_instruction(io, &instruction);
        if (result < 0) {
            guac_log_error("Error reading instruction while waiting for connect");
            guac_close(io);
            return NULL;            
        }

        /* Connect instruction read */
        if (result > 0) {

            if (strcmp(instruction.opcode, "connect") == 0) {

                /* Initialize client arguments */
                argc = instruction.argc;
                argv = instruction.argv;

                if (alias.client_init(client, argc, argv) != 0) {
                    /* NOTE: On error, proxy client will send appropriate error message */
                    guac_free_instruction_data(&instruction);
                    guac_close(io);
                    return NULL;
                }

                guac_free_instruction_data(&instruction);
                return client;

            } /* end if connect */

            guac_free_instruction_data(&instruction);
        }

    }

}

void guac_client_stop(guac_client* client) {
    client->state = STOPPING;
}

void guac_free_client(guac_client* client) {

    if (client->free_handler) {
        if (client->free_handler(client))
            guac_log_error("Error calling client free handler");
    }

    guac_close(client->io);

    /* Unload client plugin */
    if (dlclose(client->client_plugin_handle)) {
        guac_log_error("Could not close client plugin while unloading client: %s", dlerror());
    }

    /* Free all layers */
    while (client->all_layers != NULL) {
        guac_layer* layer = client->all_layers;
        client->all_layers = layer->next;
        free(layer);
    }

    free(client);
}


void* __guac_client_output_thread(void* data) {

    guac_client* client = (guac_client*) data;
    GUACIO* io = client->io;

    guac_timestamp_t last_ping_timestamp = guac_current_timestamp();

    /* Guacamole client output loop */
    while (client->state == RUNNING) {

        /* Occasionally ping client with repeat of last sync */
        guac_timestamp_t timestamp = guac_current_timestamp();
        if (timestamp - last_ping_timestamp > GUAC_SYNC_FREQUENCY) {
            last_ping_timestamp = timestamp;
            if (
                   guac_send_sync(io, client->last_sent_timestamp)
                || guac_flush(io)
               ) {
                guac_client_stop(client);
                return NULL;
            }
        }

        /* Handle server messages */
        if (client->handle_messages) {

            /* Get previous GUACIO state */
            int last_total_written = io->total_written;

            /* Only handle messages if synced within threshold */
            if (client->last_sent_timestamp - client->last_received_timestamp
                    < GUAC_SYNC_THRESHOLD) {

                int retval = client->handle_messages(client);
                if (retval) {
                    guac_log_error("Error handling server messages");
                    guac_client_stop(client);
                    return NULL;
                }

                /* If data was written during message handling */
                if (io->total_written != last_total_written) {

                    /* Sleep as necessary */
                    guac_sleep(GUAC_SERVER_MESSAGE_HANDLE_FREQUENCY);

                    /* Send sync instruction */
                    client->last_sent_timestamp = guac_current_timestamp();
                    if (guac_send_sync(io, client->last_sent_timestamp)) {
                        guac_client_stop(client);
                        return NULL;
                    }

                }

                if (guac_flush(io)) {
                    guac_client_stop(client);
                    return NULL;
                }

            }

            /* If sync threshold exceeded, don't spin waiting for resync */
            else
                guac_sleep(GUAC_SERVER_MESSAGE_HANDLE_FREQUENCY);

        }

        /* If no message handler, just sleep until next sync ping */
        else
            guac_sleep(GUAC_SYNC_FREQUENCY);

    } /* End of output loop */

    guac_client_stop(client);
    return NULL;

}

void* __guac_client_input_thread(void* data) {

    guac_client* client = (guac_client*) data;
    GUACIO* io = client->io;

    guac_instruction instruction;

    /* Guacamole client input loop */
    while (client->state == RUNNING && guac_read_instruction(io, &instruction) > 0) {

        /* Call handler, stop on error */
        if (guac_client_handle_instruction(client, &instruction) < 0) {
            guac_free_instruction_data(&instruction);
            break;
        }

        /* Free allocate instruction data */
        guac_free_instruction_data(&instruction);

    }

    guac_client_stop(client);
    return NULL;

}

int guac_start_client(guac_client* client) {

    guac_thread_t input_thread, output_thread;

    if (guac_thread_create(&output_thread, __guac_client_output_thread, (void*) client)) {
        return -1;
    }

    if (guac_thread_create(&input_thread, __guac_client_input_thread, (void*) client)) {
        guac_client_stop(client);
        guac_thread_join(output_thread);
        return -1;
    }

    /* Wait for I/O threads */
    guac_thread_join(input_thread);
    guac_thread_join(output_thread);

    /* Done */
    return 0;

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

