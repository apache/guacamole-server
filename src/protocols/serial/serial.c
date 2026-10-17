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

#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**
 * The reason a serial read loop exited, used to produce a concise, accurate
 * disconnect message.
 */
typedef enum guac_serial_disconnect_reason {

    /**
     * The remote/device closed the connection cleanly (read() returned 0).
     */
    GUAC_SERIAL_DISCONNECT_EOF,

    /**
     * A device I/O error occurred (errno EIO), typically an unplugged device.
     */
    GUAC_SERIAL_DISCONNECT_IO_ERROR,

    /**
     * The connection was reset by the peer (errno ECONNRESET).
     */
    GUAC_SERIAL_DISCONNECT_RESET,

    /**
     * The descriptor hung up or reported an error condition (POLLHUP/POLLERR).
     */
    GUAC_SERIAL_DISCONNECT_HANGUP,

    /**
     * An otherwise-unclassified read or poll error occurred.
     */
    GUAC_SERIAL_DISCONNECT_ERROR,

    /**
     * The client is shutting down; the loop exited for teardown, not a fault.
     */
    GUAC_SERIAL_DISCONNECT_STOPPING

} guac_serial_disconnect_reason;

/**
 * Returns the letter (N/E/O/M/S) representing the given parity scheme, as used
 * in the "8N1"-style line settings summary.
 *
 * @param parity
 *     The parity scheme.
 *
 * @return
 *     The single-character parity letter.
 */
static char guac_serial_parity_letter(guac_serial_parity parity) {
    switch (parity) {
        case GUAC_SERIAL_PARITY_ODD:   return 'O';
        case GUAC_SERIAL_PARITY_EVEN:  return 'E';
        case GUAC_SERIAL_PARITY_MARK:  return 'M';
        case GUAC_SERIAL_PARITY_SPACE: return 'S';
        default:                       return 'N';
    }
}

/**
 * Writes a human-readable description of the connection's endpoint (the local
 * device path, or "host:port" for network connections) into the given buffer.
 *
 * @param settings
 *     The connection settings.
 *
 * @param buffer
 *     The buffer to write the endpoint description into.
 *
 * @param size
 *     The size of the buffer, in bytes.
 */
static void guac_serial_endpoint(guac_serial_settings* settings, char* buffer,
        int size) {
    if (settings->type == GUAC_SERIAL_TYPE_LOCAL)
        snprintf(buffer, size, "%s", settings->device);
    else if (settings->reverse_connect)
        snprintf(buffer, size, "listen:%s", settings->port);
    else
        snprintf(buffer, size, "%s:%s", settings->hostname, settings->port);
}

/**
 * Writes a concise, dim, bracketed status banner to the terminal on its own
 * line, so a blank screen is diagnosable.
 *
 * @param term
 *     The terminal to write the banner to.
 *
 * @param text
 *     The banner text (without surrounding brackets or styling).
 */
static void guac_serial_write_banner(guac_terminal* term, const char* text) {
    char buffer[512];
    int length = snprintf(buffer, sizeof(buffer),
            "\r\n\x1b[2m[serial: %s]\x1b[0m\r\n", text);
    if (length > 0) {
        if (length >= (int) sizeof(buffer))
            length = sizeof(buffer) - 1;
        guac_terminal_write(term, buffer, length);
    }
}

/**
 * Returns the wire terminator bytes and length for the given line ending.
 *
 * @param line_ending
 *     The configured line ending.
 *
 * @param terminator
 *     Set to the terminator byte sequence.
 *
 * @param length
 *     Set to the length of the terminator, in bytes.
 */
static void guac_serial_line_terminator(guac_serial_line_ending line_ending,
        const char** terminator, int* length) {
    switch (line_ending) {
        case GUAC_SERIAL_LINE_ENDING_LF:   *terminator = "\n";   *length = 1; break;
        case GUAC_SERIAL_LINE_ENDING_CRLF: *terminator = "\r\n"; *length = 2; break;
        default:                           *terminator = "\r";   *length = 1; break;
    }
}

/**
 * Translates outgoing line endings in the given input buffer to the configured
 * terminator, collapsing any of "\r", "\n", or "\r\n" to a single terminator.
 * A small cross-call state machine (saw_cr) ensures a "\r\n" pair split across
 * buffers, or within one buffer, produces exactly one terminator. Non-newline
 * bytes are passed through unchanged.
 *
 * @param line_ending
 *     The configured line ending.
 *
 * @param in
 *     The input buffer.
 *
 * @param in_length
 *     The number of bytes in the input buffer.
 *
 * @param out
 *     The output buffer, which must have capacity for at least twice
 *     in_length bytes.
 *
 * @param saw_cr
 *     Pointer to the carry state: non-zero if the previous byte processed was
 *     a carriage return whose terminator was already emitted. Must be
 *     initialized to zero before the first call and preserved across calls.
 *
 * @return
 *     The number of bytes written to the output buffer.
 */
static int guac_serial_translate_line_ending(guac_serial_line_ending line_ending,
        const char* in, int in_length, char* out, int* saw_cr) {

    const char* terminator;
    int terminator_length;
    guac_serial_line_terminator(line_ending, &terminator, &terminator_length);

    int out_length = 0;
    for (int i = 0; i < in_length; i++) {

        char c = in[i];

        /* A carriage return always emits one terminator */
        if (c == '\r') {
            memcpy(out + out_length, terminator, terminator_length);
            out_length += terminator_length;
            *saw_cr = 1;
        }

        /* A line feed emits a terminator unless it completes a "\r\n" pair */
        else if (c == '\n') {
            if (!*saw_cr) {
                memcpy(out + out_length, terminator, terminator_length);
                out_length += terminator_length;
            }
            *saw_cr = 0;
        }

        /* All other bytes pass through unchanged */
        else {
            out[out_length++] = c;
            *saw_cr = 0;
        }

    }

    return out_length;

}

/**
 * Input thread, started by the main serial client thread. This thread
 * continuously reads from the terminal's STDIN, translates outgoing line
 * endings, optionally echoes input locally, and transfers the data to the
 * serial stream. It survives reconnects (the stream drops input while the
 * transport is down) and exits only when STDIN closes at teardown.
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
    guac_serial_settings* settings = serial_client->settings;

    char buffer[8192];
    char translated[2 * sizeof(buffer)];
    int saw_cr = 0;
    int bytes_read;

    /* Write all data read from STDIN to the serial stream */
    while ((bytes_read = guac_terminal_read_stdin(serial_client->term, buffer,
            sizeof(buffer))) > 0) {

        /* Translate outgoing line endings before pacing/writing */
        int length = guac_serial_translate_line_ending(settings->line_ending,
                buffer, bytes_read, translated, &saw_cr);

        guac_serial_stream_write(serial_client->stream, translated, length);

        /* Echo the user's typed bytes locally, if enabled */
        if (settings->local_echo)
            guac_terminal_write(serial_client->term, buffer, bytes_read);

    }

    return NULL;

}

/**
 * Runs the read loop for the currently-open serial stream, writing received
 * data to the terminal until the connection drops or the client stops.
 *
 * @param client
 *     The guac_client associated with the connection.
 *
 * @param serial_client
 *     The serial client data, whose stream must currently be open.
 *
 * @return
 *     The reason the loop exited.
 */
static guac_serial_disconnect_reason guac_serial_run_read_loop(
        guac_client* client, guac_serial_client* serial_client) {

    char buffer[8192];

    while (client->state == GUAC_CLIENT_RUNNING) {

        int fd = serial_client->stream->fd;

        /* Wait up to one second for data, hangup, or error */
        struct pollfd pfd = { .fd = fd, .events = POLLIN, .revents = 0 };
        int wait_result;
        GUAC_RETRY_EINTR(wait_result, poll(&pfd, 1, 1000));

        if (wait_result < 0)
            return GUAC_SERIAL_DISCONNECT_ERROR;

        /* Nothing to read yet */
        if (wait_result == 0)
            continue;

        /* Hangup/error with no pending data ends the connection */
        if ((pfd.revents & (POLLHUP | POLLERR | POLLNVAL))
                && !(pfd.revents & POLLIN))
            return GUAC_SERIAL_DISCONNECT_HANGUP;

        int bytes_read;
        GUAC_RETRY_EINTR(bytes_read, read(fd, buffer, sizeof(buffer)));

        /* Classify the disconnect */
        if (bytes_read == 0)
            return GUAC_SERIAL_DISCONNECT_EOF;
        if (bytes_read < 0) {
            if (errno == EIO)
                return GUAC_SERIAL_DISCONNECT_IO_ERROR;
            if (errno == ECONNRESET)
                return GUAC_SERIAL_DISCONNECT_RESET;
            return GUAC_SERIAL_DISCONNECT_ERROR;
        }

        /* RFC2217 and telnet output must be de-framed; all other transports are
         * raw */
        if (serial_client->stream->backend == GUAC_SERIAL_BACKEND_RFC2217
                || serial_client->stream->backend == GUAC_SERIAL_BACKEND_TELNET)
            guac_serial_rfc2217_recv(serial_client->stream, buffer, bytes_read);
        else
            guac_terminal_write(serial_client->term, buffer, bytes_read);

    }

    return GUAC_SERIAL_DISCONNECT_STOPPING;

}

/**
 * Logs the reason a serial connection dropped, at a level appropriate to
 * whether it represents a normal end-of-connection or a fault.
 *
 * @param client
 *     The guac_client associated with the connection.
 *
 * @param reason
 *     The reason the connection dropped.
 */
static void guac_serial_log_disconnect(guac_client* client,
        guac_serial_disconnect_reason reason) {

    switch (reason) {

        case GUAC_SERIAL_DISCONNECT_EOF:
            guac_client_log(client, GUAC_LOG_INFO,
                    "Serial connection dropped: remote/device closed the "
                    "connection (EOF).");
            break;

        case GUAC_SERIAL_DISCONNECT_IO_ERROR:
            guac_client_log(client, GUAC_LOG_WARNING,
                    "Serial connection dropped: device I/O error (unplugged?).");
            break;

        case GUAC_SERIAL_DISCONNECT_RESET:
            guac_client_log(client, GUAC_LOG_WARNING,
                    "Serial connection dropped: connection reset.");
            break;

        case GUAC_SERIAL_DISCONNECT_HANGUP:
            guac_client_log(client, GUAC_LOG_WARNING,
                    "Serial connection dropped: hangup.");
            break;

        case GUAC_SERIAL_DISCONNECT_ERROR:
            guac_client_log(client, GUAC_LOG_WARNING,
                    "Serial connection dropped: read error.");
            break;

        case GUAC_SERIAL_DISCONNECT_STOPPING:
            /* Normal teardown; nothing to report as a fault */
            break;

    }

}

/**
 * Sleeps for the given number of seconds, returning early if the client stops
 * running. The wait is broken into short slices so that teardown is prompt.
 *
 * @param client
 *     The client whose shutdown state is checked.
 *
 * @param seconds
 *     The number of seconds to wait.
 *
 * @return
 *     Non-zero if the client stopped running during the wait, zero otherwise.
 */
static int guac_serial_reconnect_wait(guac_client* client, int seconds) {

    int milliseconds = seconds * 1000;
    while (milliseconds > 0 && client->state == GUAC_CLIENT_RUNNING) {
        int slice = milliseconds < 100 ? milliseconds : 100;
        usleep((useconds_t) slice * 1000);
        milliseconds -= slice;
    }

    return client->state != GUAC_CLIENT_RUNNING;

}

void* guac_serial_client_thread(void* data) {

    /* Thread name serial-worker: main serial client thread; runs the serial
     * session and event loop. */
    guac_thread_name_set("serial-worker");

    guac_client* client = (guac_client*) data;
    guac_serial_client* serial_client = (guac_serial_client*) client->data;
    guac_serial_settings* settings = serial_client->settings;

    pthread_t input_thread;

    /* Human-readable endpoint and "9600 8N1"-style line settings summary */
    char endpoint[256];
    guac_serial_endpoint(settings, endpoint, sizeof(endpoint));

    char line_settings[64];
    snprintf(line_settings, sizeof(line_settings), "%d %d%c%d",
            settings->baud_rate, settings->data_bits,
            guac_serial_parity_letter(settings->parity), settings->stop_bits);

    /* Set process title to the device, listen endpoint, or hostname reached by
     * this connection */
    const char* title_host;
    if (settings->type == GUAC_SERIAL_TYPE_LOCAL)
        title_host = settings->device;
    else if (settings->reverse_connect)
        title_host = endpoint;
    else
        title_host = settings->hostname;
    guac_process_title_set_endpoint(GUAC_SERIAL_PROCESS_TITLE_NAME,
            NULL, title_host, NULL);

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

    /* Allocate the persistent stream (not yet connected) so it survives
     * reconnects for the session lifetime */
    serial_client->stream = guac_serial_stream_alloc(client, settings);

    /* Allow the terminal to render; there is no login detection. Started before
     * the first connect so that (re)connection banners are visible. */
    guac_terminal_start(serial_client->term);

    /* Send current values of exposed arguments to owner only */
    guac_client_for_owner(client, guac_serial_send_current_argv, serial_client);

    /* Start input thread (persists across reconnects) */
    if (pthread_create(&input_thread, NULL, guac_serial_input_thread,
            (void*) client)) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR,
                "Unable to start input thread");
        return NULL;
    }

    int connected = 0;
    int first_attempt = 1;
    int ever_connected = 0;

    while (client->state == GUAC_CLIENT_RUNNING) {

        /* (Re)connect if not currently connected */
        if (!connected) {

            /* Pace reconnect attempts, but do not wait before the very first
             * connect attempt */
            if (!first_attempt) {
                if (guac_serial_reconnect_wait(client,
                        GUAC_SERIAL_RECONNECT_INTERVAL))
                    break;
            }

            int result = guac_serial_stream_reopen(serial_client->stream,
                    client, settings);

            if (result != 0) {

                /* A failed very-first connect with reconnect disabled aborts,
                 * exactly as before */
                if (first_attempt && !settings->auto_reconnect) {
                    guac_client_abort(client, serial_client->stream->open_status,
                            "Unable to establish serial connection to %s.",
                            endpoint);
                    break;
                }

                /* Announce the wait so a blank screen is diagnosable */
                if (first_attempt)
                    guac_serial_write_banner(serial_client->term,
                            "connecting…");

                first_attempt = 0;
                continue;

            }

            /* Connected */
            connected = 1;
            first_attempt = 0;

            if (ever_connected) {
                guac_client_log(client, GUAC_LOG_INFO, "Serial connection "
                        "re-established to %s (%s).", endpoint, line_settings);
                guac_serial_write_banner(serial_client->term, "reconnected");
            }
            else
                guac_client_log(client, GUAC_LOG_INFO, "Serial connection "
                        "established to %s (%s).", endpoint, line_settings);

            ever_connected = 1;

            /* Status banner with press-Enter hint (§7) */
            char banner[384];
            snprintf(banner, sizeof(banner),
                    "connected to %s @ %s — press Enter if blank",
                    endpoint, line_settings);
            guac_serial_write_banner(serial_client->term, banner);

        }

        /* Run the read loop until the connection drops */
        guac_serial_disconnect_reason reason =
                guac_serial_run_read_loop(client, serial_client);
        connected = 0;

        /* Stop quietly if the client is shutting down */
        if (reason == GUAC_SERIAL_DISCONNECT_STOPPING
                || client->state != GUAC_CLIENT_RUNNING)
            break;

        guac_serial_log_disconnect(client, reason);

        /* Without auto-reconnect, a drop ends the session */
        if (!settings->auto_reconnect)
            break;

        /* Announce reconnection and loop to retry */
        guac_serial_write_banner(serial_client->term,
                "connection lost — reconnecting…");

    }

    /* Kill client and wait for input thread to die */
    guac_client_stop(client);
    pthread_join(input_thread, NULL);

    guac_client_log(client, GUAC_LOG_INFO, "Serial connection ended.");
    return NULL;

}
