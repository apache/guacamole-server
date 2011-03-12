
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
#ifdef HAVE_CLOCK_GETTIME
#include <time.h>
#else
#include <sys/time.h>
#endif
#include <string.h>
#include <dlfcn.h>

#include "log.h"
#include "guacio.h"
#include "protocol.h"
#include "client.h"


png_byte** guac_alloc_png_buffer(int w, int h, int bpp) {

    png_byte** png_buffer;
    png_byte* row;
    int y;

    /* Allocate rows for PNG */
    png_buffer = (png_byte**) malloc(h * sizeof(png_byte*));
    for (y=0; y<h; y++) {
        row = (png_byte*) malloc(sizeof(png_byte) * bpp * w);
        png_buffer[y] = row;
    }

    return png_buffer;
}

void guac_free_png_buffer(png_byte** png_buffer, int h) {

    int y;

    /* Free PNG data */
    for (y = 0; y<h; y++)
        free(png_buffer[y]);
    free(png_buffer);

}

guac_client* __guac_alloc_client(GUACIO* io) {

    /* Allocate new client (not handoff) */
    guac_client* client = malloc(sizeof(guac_client));
    memset(client, 0, sizeof(guac_client));

    /* Init new client */
    client->io = io;

    return client;
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

        int result = guac_read_instruction(io, &instruction);
        if (result < 0) {
            GUAC_LOG_ERROR("Error reading instruction while waiting for select");
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
                    GUAC_LOG_ERROR("Could not open client plugin for protocol \"%s\": %s\n", protocol, dlerror());
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
                    GUAC_LOG_ERROR("Could not get guac_client_init in plugin: %s\n", error);
                    guac_send_error(io, "Invalid server-side client plugin.");
                    guac_flush(io);
                    guac_close(io);
                    guac_free_instruction_data(&instruction);
                    return NULL;
                }

                /* Get usage strig */
                client_args = (const char**) dlsym(client->client_plugin_handle, "GUAC_CLIENT_ARGS");

                if ((error = dlerror()) != NULL) {
                    GUAC_LOG_ERROR("Could not get GUAC_CLIENT_ in plugin: %s\n", error);
                    guac_send_error(io, "Invalid server-side client plugin.");
                    guac_flush(io);
                    guac_close(io);
                    guac_free_instruction_data(&instruction);
                    return NULL;
                }

                /* Send args */
                guac_send_args(io, client_args);
                guac_flush(io);

                guac_free_instruction_data(&instruction);
                break;

            } /* end if select */

            guac_free_instruction_data(&instruction);
        }

    }

    /* Wait for connect instruction */
    for (;;) {

        int result = guac_read_instruction(io, &instruction);
        if (result < 0) {
            GUAC_LOG_ERROR("Error reading instruction while waiting for connect");
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


void guac_free_client(guac_client* client) {

    if (client->free_handler) {
        if (client->free_handler(client))
            GUAC_LOG_ERROR("Error calling client free handler");
    }

    guac_close(client->io);

    /* Unload client plugin */
    if (dlclose(client->client_plugin_handle)) {
        GUAC_LOG_ERROR("Could not close client plugin while unloading client: %s", dlerror());
    }

    free(client);
}


long __guac_current_timestamp() {

#ifdef HAVE_CLOCK_GETTIME

    struct timespec current;

    /* Get current time */
    clock_gettime(CLOCK_REALTIME, &current);
    
    /* Calculate milliseconds */
    return current.tv_sec * 1000 + current.tv_nsec / 1000000;

#else

    struct timeval current;

    /* Get current time */
    gettimeofday(&current, NULL);
    
    /* Calculate milliseconds */
    return current.tv_sec * 1000 + current.tv_usec / 1000;

#endif

}

void guac_start_client(guac_client* client) {

    GUACIO* io = client->io;
    guac_instruction instruction;
    int wait_result;
    long last_received_timestamp;
    long last_sent_timestamp;

    /* Init timestamps */
    last_received_timestamp = last_sent_timestamp = __guac_current_timestamp();

    /* VNC Client Loop */
    for (;;) {

        /* Handle server messages */
        if (client->handle_messages) {

            /* Get previous GUACIO state */
            int last_total_written = io->total_written;

            /* Only handle messages if synced within threshold */
            if (last_sent_timestamp - last_received_timestamp < 200) {

                int retval = client->handle_messages(client);
                if (retval) {
                    GUAC_LOG_ERROR("Error handling server messages");
                    return;
                }

                /* If data was written during message handling */
                if (io->total_written != last_total_written) {
                    last_sent_timestamp = __guac_current_timestamp();
                    guac_send_sync(io, last_sent_timestamp);
                }

                guac_flush(io);
            }

        }

        wait_result = guac_instructions_waiting(io);
        if (wait_result > 0) {

            int retval;
            retval = guac_read_instruction(io, &instruction); /* 0 if no instructions finished yet, <0 if error or EOF */

            if (retval > 0) {
           
                do {

                    if (strcmp(instruction.opcode, "sync") == 0) {
                        last_received_timestamp = atol(instruction.argv[0]);
                        if (last_received_timestamp > last_sent_timestamp) {
                            guac_send_error(io, "Received sync from future.");
                            guac_free_instruction_data(&instruction);
                            return;
                        }
                    }
                    else if (strcmp(instruction.opcode, "mouse") == 0) {
                        if (client->mouse_handler)
                            if (
                                    client->mouse_handler(
                                        client,
                                        atoi(instruction.argv[0]), /* x */
                                        atoi(instruction.argv[1]), /* y */
                                        atoi(instruction.argv[2])  /* mask */
                                    )
                               ) {

                                GUAC_LOG_ERROR("Error handling mouse instruction");
                                guac_free_instruction_data(&instruction);
                                return;

                            }
                    }

                    else if (strcmp(instruction.opcode, "key") == 0) {
                        if (client->key_handler)
                            if (
                                    client->key_handler(
                                        client,
                                        atoi(instruction.argv[0]), /* keysym */
                                        atoi(instruction.argv[1])  /* pressed */
                                    )
                               ) {

                                GUAC_LOG_ERROR("Error handling key instruction");
                                guac_free_instruction_data(&instruction);
                                return;

                            }
                    }

                    else if (strcmp(instruction.opcode, "clipboard") == 0) {
                        if (client->clipboard_handler)
                            if (
                                    client->clipboard_handler(
                                        client,
                                        guac_unescape_string_inplace(instruction.argv[0]) /* data */
                                    )
                               ) {

                                GUAC_LOG_ERROR("Error handling clipboard instruction");
                                guac_free_instruction_data(&instruction);
                                return;

                            }
                    }

                    else if (strcmp(instruction.opcode, "disconnect") == 0) {
                        GUAC_LOG_INFO("Client requested disconnect");
                        guac_free_instruction_data(&instruction);
                        return;
                    }

                    guac_free_instruction_data(&instruction);

                } while ((retval = guac_read_instruction(io, &instruction)) > 0);

                if (retval < 0) {
                    GUAC_LOG_ERROR("Error reading instruction from stream");
                    return;
                }
            }

            if (retval < 0) {
                GUAC_LOG_ERROR("Error or end of stream");
                return; /* EOF or error */
            }

            /* Otherwise, retval == 0 implies unfinished instruction */

        }
        else if (wait_result < 0) {
            GUAC_LOG_ERROR("Error waiting for next instruction");
            return;
        }

    }

}

