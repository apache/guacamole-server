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

#include "config.h"
#include "argv.h"
#include "client.h"
#include "serial.h"
#include "settings.h"
#include "stream.h"
#include "user.h"

#include <langinfo.h>
#include <locale.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <guacamole/argv.h>
#include <guacamole/client.h>
#include <guacamole/mem.h>
#include <guacamole/recording.h>
#include <guacamole/socket.h>

/**
 * A pending join handler implementation that will synchronize the connection
 * state for all pending users prior to them being promoted to full user.
 *
 * @param client
 *     The client whose pending users are about to be promoted.
 *
 * @return
 *     Always zero.
 */
static int guac_serial_join_pending_handler(guac_client* client) {

    guac_serial_client* serial_client = (guac_serial_client*) client->data;

    /* Synchronize the terminal state to all pending users */
    if (serial_client->term != NULL) {
        guac_socket* broadcast_socket = client->pending_socket;
        guac_terminal_sync_users(serial_client->term, client, broadcast_socket);
        guac_serial_send_current_argv_batch(client, broadcast_socket);
        guac_socket_flush(broadcast_socket);
    }

    return 0;

}

int guac_client_init(guac_client* client) {

    /* Set client args */
    client->args = GUAC_SERIAL_CLIENT_ARGS;

    /* Allocate client instance data */
    guac_serial_client* serial_client = guac_mem_zalloc(sizeof(guac_serial_client));
    client->data = serial_client;

    /* Set handlers */
    client->join_handler = guac_serial_user_join_handler;
    client->join_pending_handler = guac_serial_join_pending_handler;
    client->free_handler = guac_serial_client_free_handler;
    client->leave_handler = guac_serial_user_leave_handler;

    /* Register handlers for argument values that may be sent after the handshake */
    guac_argv_register(GUAC_SERIAL_ARGV_COLOR_SCHEME, guac_serial_argv_callback, NULL, GUAC_ARGV_OPTION_ECHO);
    guac_argv_register(GUAC_SERIAL_ARGV_FONT_NAME, guac_serial_argv_callback, NULL, GUAC_ARGV_OPTION_ECHO);
    guac_argv_register(GUAC_SERIAL_ARGV_FONT_SIZE, guac_serial_argv_callback, NULL, GUAC_ARGV_OPTION_ECHO);

    /* Set locale and warn if not UTF-8 */
    setlocale(LC_CTYPE, "");
    if (strcmp(nl_langinfo(CODESET), "UTF-8") != 0) {
        guac_client_log(client, GUAC_LOG_INFO,
                "Current locale does not use UTF-8. Some characters may "
                "not render correctly.");
    }

    /* Success */
    return 0;

}

int guac_serial_client_free_handler(guac_client* client) {

    guac_serial_client* serial_client = (guac_serial_client*) client->data;

    /* Signal shutdown so the client thread's read loop, reconnect waits, and
     * auto-reconnect loop all observe the stopped state and exit promptly. The
     * stream's own file descriptor is not closed here (it is swapped under a
     * lock during reconnects); the worker unblocks via its bounded poll timeout
     * and the descriptor is closed when the stream is freed below, after the
     * worker has been joined. */
    guac_client_stop(client);

    /* Stop the terminal — closing its input pipe to unblock the input thread —
     * WITHOUT freeing it yet. The client thread may still be writing serial
     * output to the terminal until it has fully exited, so the terminal must
     * outlive the join below to avoid a use-after-free. */
    if (serial_client->term != NULL)
        guac_terminal_stop(serial_client->term);

    /* Wait for the client thread to fully exit before touching anything it
     * uses. Joined only when it was successfully created, so early-failure
     * setup paths neither leak the joinable thread nor join an invalid one. */
    if (serial_client->client_thread_valid) {
        pthread_join(serial_client->client_thread, NULL);
        serial_client->client_thread_valid = 0;
    }

    /* Clean up recording, if in progress */
    if (serial_client->recording != NULL)
        guac_recording_free(serial_client->recording);

    /* With no thread left to touch it, the terminal can now be freed safely */
    if (serial_client->term != NULL)
        guac_terminal_free(serial_client->term);

    /* Free the serial stream (frees any libtelnet session) */
    if (serial_client->stream != NULL)
        guac_serial_stream_close(serial_client->stream);

    /* Free settings */
    if (serial_client->settings != NULL)
        guac_serial_settings_free(serial_client->settings);

    guac_mem_free(serial_client);
    return 0;

}
