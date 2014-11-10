/*
 * Copyright (C) 2013 Glyptodon LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "config.h"

#include "client.h"
#include "log.h"

#include <guacamole/client.h>
#include <guacamole/error.h>
#include <guacamole/instruction.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/timestamp.h>

#include <pthread.h>
#include <stdlib.h>
#include <time.h>

/**
 * Sleep for the given number of milliseconds.
 *
 * @param millis The number of milliseconds to sleep.
 */
void __guacdd_sleep(int millis) {

    struct timespec sleep_period;

    sleep_period.tv_sec =   millis / 1000;
    sleep_period.tv_nsec = (millis % 1000) * 1000000L;

    nanosleep(&sleep_period, NULL);

}

void* __guacd_client_output_thread(void* data) {

    guac_client* client = (guac_client*) data;
    guac_socket* socket = client->socket;

    guac_client_log(client, GUAC_LOG_DEBUG,
            "Starting output thread.");

    /* Guacamole client output loop */
    while (client->state == GUAC_CLIENT_RUNNING) {

        /* Handle server messages */
        if (client->handle_messages) {

            /* Only handle messages if synced within threshold */
            if (client->last_sent_timestamp - client->last_received_timestamp
                    < GUACD_SYNC_THRESHOLD) {

                int retval = client->handle_messages(client);
                if (retval) {
                    guacd_client_log_guac_error(client, GUAC_LOG_DEBUG,
                            "Error handling server messages");
                    guac_client_stop(client);
                    return NULL;
                }

                /* Send sync instruction */
                client->last_sent_timestamp = guac_timestamp_current();
                if (guac_protocol_send_sync(socket, client->last_sent_timestamp)) {
                    guacd_client_log_guac_error(client, GUAC_LOG_DEBUG,
                            "Error sending \"sync\" instruction");
                    guac_client_stop(client);
                    return NULL;
                }

                /* Flush */
                if (guac_socket_flush(socket)) {
                    guacd_client_log_guac_error(client, GUAC_LOG_DEBUG,
                            "Error flushing output");
                    guac_client_stop(client);
                    return NULL;
                }

            }

            /* Do not spin while waiting for old sync */
            else
                __guacdd_sleep(GUACD_MESSAGE_HANDLE_FREQUENCY);

        }

        /* If no message handler, just sleep until next sync ping */
        else
            __guacdd_sleep(GUACD_SYNC_FREQUENCY);

    } /* End of output loop */

    guac_client_log(client, GUAC_LOG_DEBUG,
            "Output thread terminated.");

    guac_client_stop(client);
    return NULL;

}

void* __guacd_client_input_thread(void* data) {

    guac_client* client = (guac_client*) data;
    guac_socket* socket = client->socket;

    guac_client_log(client, GUAC_LOG_DEBUG,
            "Starting input thread.");

    /* Guacamole client input loop */
    while (client->state == GUAC_CLIENT_RUNNING) {

        /* Read instruction */
        guac_instruction* instruction =
            guac_instruction_read(socket, GUACD_USEC_TIMEOUT);

        /* Stop on error */
        if (instruction == NULL) {

            if (guac_error == GUAC_STATUS_TIMEOUT)
                guac_client_abort(client, GUAC_PROTOCOL_STATUS_CLIENT_TIMEOUT, "Client is not responding.");

            else {
                if (guac_error != GUAC_STATUS_CLOSED)
                    guacd_client_log_guac_error(client, GUAC_LOG_WARNING,
                            "Guacamole connection failure");
                guac_client_stop(client);
            }

            return NULL;
        }

        /* Reset guac_error and guac_error_message (client handlers are not
         * guaranteed to set these) */
        guac_error = GUAC_STATUS_SUCCESS;
        guac_error_message = NULL;

        /* Call handler, stop on error */
        if (guac_client_handle_instruction(client, instruction) < 0) {

            /* Log error */
            guacd_client_log_guac_error(client, GUAC_LOG_WARNING,
                    "Connection aborted");

            /* Log handler details */
            guac_client_log(client, GUAC_LOG_DEBUG,
                    "Failing instruction handler in client was \"%s\"",
                    instruction->opcode);

            guac_instruction_free(instruction);
            guac_client_stop(client);
            return NULL;
        }

        /* Free allocated instruction */
        guac_instruction_free(instruction);

    }

    guac_client_log(client, GUAC_LOG_DEBUG,
            "Input thread terminated.");

    return NULL;

}

int guacd_client_start(guac_client* client) {

    pthread_t input_thread, output_thread;

    if (pthread_create(&output_thread, NULL, __guacd_client_output_thread, (void*) client)) {
        guac_client_log(client, GUAC_LOG_ERROR, "Unable to start output thread");
        return -1;
    }

    if (pthread_create(&input_thread, NULL, __guacd_client_input_thread, (void*) client)) {
        guac_client_log(client, GUAC_LOG_ERROR, "Unable to start input thread");
        guac_client_stop(client);
        pthread_join(output_thread, NULL);
        return -1;
    }

    /* Wait for I/O threads */
    pthread_join(input_thread, NULL);
    pthread_join(output_thread, NULL);

    /* Done */
    return 0;

}

