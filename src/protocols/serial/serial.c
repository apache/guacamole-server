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
#include "rfc2217.h"
#include "serial.h"
#include "settings.h"
#include "stream.h"
#include "terminal/terminal.h"

#include <guacamole/client.h>
#include <guacamole/error.h>
#include <guacamole/mem.h>
#include <guacamole/proctitle.h>
#include <guacamole/protocol.h>
#include <guacamole/recording.h>

#include <poll.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**
 * Input thread, started by the main serial client thread. This thread
 * continuously reads from the terminal's STDIN and transfers all read data to
 * the serial line.
 *
 * @param data
 *     The current guac_client instance.
 *
 * @return
 *     Always NULL.
 */
static void* guac_serial_input_thread(void* data) {

    /* Thread name serial-stdin: reads terminal STDIN and forwards it to the
     * serial line. */
    guac_thread_name_set("serial-stdin");

    guac_client* client = (guac_client*) data;
    guac_serial_client* serial_client = (guac_serial_client*) client->data;

    char buffer[8192];
    int bytes_read;

    /* Write all data read from STDIN to the serial stream */
    while ((bytes_read = guac_terminal_read_stdin(serial_client->term, buffer,
            sizeof(buffer))) > 0) {
        if (guac_serial_stream_write(serial_client->stream, buffer, bytes_read) < 0)
            break;
    }

    return NULL;

}

/**
 * Waits for data on the given file descriptor for up to one second. The return
 * value is identical to that of poll(): 0 on timeout, < 0 on error, and > 0
 * when the descriptor is readable or has hung up.
 *
 * @param fd
 *     The file descriptor to wait for.
 *
 * @return
 *     A value greater than zero when data is available or the descriptor has
 *     hung up, zero on timeout, and less than zero on error.
 */
static int guac_serial_wait(int fd) {

    /* Watch for readable data as well as hangup/error conditions */
    struct pollfd fds[] = {{
        .fd      = fd,
        .events  = POLLIN | POLLHUP | POLLERR,
        .revents = 0,
    }};

    int wait_result;
    GUAC_RETRY_EINTR(wait_result, poll(fds, 1, 1000));

    return wait_result;

}

void* guac_serial_client_thread(void* data) {

    /* Thread name serial-worker: main serial client thread; runs the serial
     * session and event loop. */
    guac_thread_name_set("serial-worker");

    guac_client* client = (guac_client*) data;
    guac_serial_client* serial_client = (guac_serial_client*) client->data;
    guac_serial_settings* settings = serial_client->settings;

    pthread_t input_thread;
    char buffer[8192];
    int wait_result;

    /* Set process title to the device or hostname reached by this connection */
    const char* endpoint = settings->type == GUAC_SERIAL_TYPE_LOCAL
            ? settings->device : settings->hostname;
    guac_process_title_set_endpoint(GUAC_SERIAL_PROCESS_TITLE_NAME,
            NULL, endpoint, NULL);

    /* Set up screen recording, if requested */
    if (settings->recording_path != NULL) {
        serial_client->recording = guac_recording_create(client,
                settings->recording_path,
                settings->recording_name,
                settings->create_recording_path,
                !settings->recording_exclude_output,
                !settings->recording_exclude_mouse,
                0, /* Touch events not supported */
                settings->recording_include_keys,
                settings->recording_write_existing,
                settings->recording_include_clipboard);
    }

    /* Create terminal options with required parameters */
    guac_terminal_options* options = guac_terminal_options_create(
            settings->width, settings->height, settings->resolution);

    /* Set optional parameters */
    options->clipboard_buffer_size = settings->clipboard_buffer_size;
    options->disable_copy = settings->disable_copy;
    options->max_scrollback = settings->max_scrollback;
    options->font_name = settings->font_name;
    options->font_size = settings->font_size;
    options->color_scheme = settings->color_scheme;
    options->backspace = settings->backspace;
    options->linux_console_keys = (strcmp(settings->terminal_type, "linux") == 0);

    /* Create terminal */
    serial_client->term = guac_terminal_create(client, options);

    /* Free options struct now that it's been used */
    guac_mem_free(options);

    /* Fail if terminal init failed */
    if (serial_client->term == NULL) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR,
                "Terminal initialization failed");
        return NULL;
    }

    /* Set up typescript, if requested */
    if (settings->typescript_path != NULL) {
        guac_terminal_create_typescript(serial_client->term,
                settings->typescript_path,
                settings->typescript_name,
                settings->create_typescript_path,
                settings->typescript_write_existing);
    }

    /* Open the serial stream via the configured transport */
    serial_client->stream = guac_serial_stream_open(client, settings);
    if (serial_client->stream == NULL) {
        /* Already aborted within guac_serial_stream_open() */
        return NULL;
    }

    /* Connection established */
    guac_client_log(client, GUAC_LOG_INFO, "Serial connection established.");

    /* Allow the terminal to render; there is no login detection */
    guac_terminal_start(serial_client->term);

    /* Send current values of exposed arguments to owner only */
    guac_client_for_owner(client, guac_serial_send_current_argv, serial_client);

    /* Start input thread */
    if (pthread_create(&input_thread, NULL, guac_serial_input_thread,
            (void*) client)) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR,
                "Unable to start input thread");
        return NULL;
    }

    /* While data available, write to terminal */
    while ((wait_result = guac_serial_wait(serial_client->stream->fd)) >= 0) {

        /* Resume waiting if no data available */
        if (wait_result == 0)
            continue;

        int bytes_read;
        GUAC_RETRY_EINTR(bytes_read,
                read(serial_client->stream->fd, buffer, sizeof(buffer)));

        /* EOF, EIO, or hangup indicates the serial line has closed */
        if (bytes_read <= 0)
            break;

        /* RFC2217 output must be de-framed; all other transports are raw */
        if (serial_client->stream->backend == GUAC_SERIAL_BACKEND_RFC2217)
            guac_serial_rfc2217_recv(serial_client->stream, buffer, bytes_read);
        else
            guac_terminal_write(serial_client->term, buffer, bytes_read);

    }

    /* Kill client and wait for input thread to die */
    guac_client_stop(client);
    pthread_join(input_thread, NULL);

    guac_client_log(client, GUAC_LOG_INFO, "Serial connection ended.");
    return NULL;

}
