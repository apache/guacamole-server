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
#include "telnet_client.h"
#include "terminal.h"

#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <libtelnet.h>

#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

/**
 * Support levels for various telnet options, required for connection
 * negotiation by telnet_init(), part of libtelnet.
 */
static const telnet_telopt_t __telnet_options[] = {
    { TELNET_TELOPT_ECHO,        TELNET_WONT, TELNET_DO   },
    { TELNET_TELOPT_TTYPE,       TELNET_WILL, TELNET_DONT },
    { TELNET_TELOPT_COMPRESS2,   TELNET_WONT, TELNET_DO   },
    { TELNET_TELOPT_MSSP,        TELNET_WONT, TELNET_DO   },
    { TELNET_TELOPT_NAWS,        TELNET_WILL, TELNET_DONT },
    { TELNET_TELOPT_NEW_ENVIRON, TELNET_WILL, TELNET_DONT },
    { -1, 0, 0 }
};

/**
 * Write the entire buffer given to the specified file descriptor, retrying
 * the write automatically if necessary. This function will return a value
 * not equal to the buffer's size iff an error occurs which prevents all
 * future writes.
 *
 * @param fd The file descriptor to write to.
 * @param buffer The buffer to write.
 * @param size The number of bytes from the buffer to write.
 */
static int __guac_telnet_write_all(int fd, const char* buffer, int size) {

    int remaining = size;
    while (remaining > 0) {

        /* Attempt to write data */
        int ret_val = write(fd, buffer, remaining);
        if (ret_val <= 0)
            return -1;

        /* If successful, contine with what data remains (if any) */
        remaining -= ret_val;
        buffer += ret_val;

    }

    return size;

}

/**
 * Searches for a line matching the stored password regex, appending the given
 * buffer to the internal pattern matching buffer. The internal pattern match
 * buffer is cleared whenever a newline is read. Returns TRUE if a match is found and the
 * value is sent.
 */
static bool __guac_telnet_regex_search(guac_client* client, regex_t* regex, char* value, const char* buffer, int size) {

    static char line_buffer[1024] = {0};
    static int length = 0;

    guac_telnet_client_data* client_data = (guac_telnet_client_data*) client->data;

    int i;
    const char* current;

    /* Ensure line buffer contains only the most recent line */
    current = buffer;
    for (i = 0; i < size; i++) {

        /* Reset line buffer and shift input buffer for each newline */
        if (*(current++) == '\n') {
            length = 0;
            buffer += i;
            size -= i;
            i = 0;
        }
    }

    /* Truncate if necessary */
    if (size + length + 1 > sizeof(line_buffer))
        size = sizeof(line_buffer) - length - 1;

    /* Append to line */
    memcpy(&(line_buffer[length]), buffer, size);
    length += size;
    line_buffer[length] = '\0';

    /* Send value upon match */
    if (regexec(regex, line_buffer, 0, NULL, 0) == 0) {

        /* Send value */
        guac_terminal_send_string(client_data->term, value);
        guac_terminal_send_key(client_data->term, 0xFF0D, 1);
        guac_terminal_send_key(client_data->term, 0xFF0D, 0);

        /* Stop searching for prompt */
        return TRUE;

    }

    return FALSE;
}

/**
 * Event handler, as defined by libtelnet. This function is passed to
 * telnet_init() and will be called for every event fired by libtelnet,
 * including feature enable/disable and receipt/transmission of data.
 */
static void __guac_telnet_event_handler(telnet_t* telnet, telnet_event_t* event, void* data) {

    guac_client* client = (guac_client*) data;
    guac_telnet_client_data* client_data = (guac_telnet_client_data*) client->data;

    switch (event->type) {

        /* Terminal output received */
        case TELNET_EV_DATA:
            guac_terminal_write_stdout(client_data->term, event->data.buffer, event->data.size);

            /* Continue search for username prompt */
            if (client_data->username_regex != NULL) {
                if (__guac_telnet_regex_search(client, client_data->username_regex, client_data->username,
                                           event->data.buffer, event->data.size)) {
                    regfree(client_data->username_regex);
                    free(client_data->username_regex);
                    client_data->username_regex = NULL;
                }
            }

            /* Continue search for password prompt */
            if (client_data->password_regex != NULL) {
                if (__guac_telnet_regex_search(client, client_data->password_regex, client_data->password,
                                           event->data.buffer, event->data.size)) {

                    /* Do not continue searching for username once password is sent */
                    if (client_data->username_regex != NULL) {
                        regfree(client_data->username_regex);
                        free(client_data->username_regex);
                        client_data->username_regex = NULL;
                    }

                    regfree(client_data->password_regex);
                    free(client_data->password_regex);
                    client_data->password_regex = NULL;
                }
            }
            break;

        /* Data destined for remote end */
        case TELNET_EV_SEND:
            if (__guac_telnet_write_all(client_data->socket_fd, event->data.buffer, event->data.size)
                    != event->data.size)
                guac_client_stop(client);
            break;

        /* Remote feature enabled */
        case TELNET_EV_WILL:
            if (event->neg.telopt == TELNET_TELOPT_ECHO)
                client_data->echo_enabled = 0; /* Disable local echo, as remote will echo */
            break;

        /* Remote feature disabled */
        case TELNET_EV_WONT:
            if (event->neg.telopt == TELNET_TELOPT_ECHO)
                client_data->echo_enabled = 1; /* Enable local echo, as remote won't echo */
            break;

        /* Local feature enable */
        case TELNET_EV_DO:
            if (event->neg.telopt == TELNET_TELOPT_NAWS) {
                client_data->naws_enabled = 1;
                guac_telnet_send_naws(telnet, client_data->term->term_width, client_data->term->term_height);
            }
            break;

        /* Terminal type request */
        case TELNET_EV_TTYPE:
            if (event->ttype.cmd == TELNET_TTYPE_SEND)
                telnet_ttype_is(client_data->telnet, "linux");
            break;

        /* Environment request */
        case TELNET_EV_ENVIRON:

            /* Only send USER if entire environment was requested */
            if (event->environ.size == 0)
                guac_telnet_send_user(telnet, client_data->username);

            break;

        /* Connection warnings */
        case TELNET_EV_WARNING:
            guac_client_log(client, GUAC_LOG_INFO, "%s", event->error.msg);
            break;

        /* Connection errors */
        case TELNET_EV_ERROR:
            guac_client_abort(client, GUAC_PROTOCOL_STATUS_UPSTREAM_ERROR,
                    "Telnet connection closing with error: %s", event->error.msg);
            break;

        /* Ignore other events */
        default:
            break;

    }

}

/**
 * Input thread, started by the main telnet client thread. This thread
 * continuously reads from the terminal's STDIN and transfers all read
 * data to the telnet connection.
 *
 * @param data The current guac_client instance.
 * @return Always NULL.
 */
static void* __guac_telnet_input_thread(void* data) {

    guac_client* client = (guac_client*) data;
    guac_telnet_client_data* client_data = (guac_telnet_client_data*) client->data;

    char buffer[8192];
    int bytes_read;

    /* Write all data read */
    while ((bytes_read = guac_terminal_read_stdin(client_data->term, buffer, sizeof(buffer))) > 0) {
        telnet_send(client_data->telnet, buffer, bytes_read);
        if (client_data->echo_enabled)
            guac_terminal_write_stdout(client_data->term, buffer, bytes_read);
    }

    return NULL;

}

/**
 * Connects to the telnet server specified within the data associated
 * with the given guac_client, which will have been populated by
 * guac_client_init.
 *
 * @return The connected telnet instance, if successful, or NULL if the
 *         connection fails for any reason.
 */
static telnet_t* __guac_telnet_create_session(guac_client* client) {

    int retval;

    int fd;
    struct addrinfo* addresses;
    struct addrinfo* current_address;

    char connected_address[1024];
    char connected_port[64];

    guac_telnet_client_data* client_data = (guac_telnet_client_data*) client->data;

    struct addrinfo hints = {
        .ai_family   = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM,
        .ai_protocol = IPPROTO_TCP
    };

    /* Get socket */
    fd = socket(AF_INET, SOCK_STREAM, 0);

    /* Get addresses connection */
    if ((retval = getaddrinfo(client_data->hostname, client_data->port,
                    &hints, &addresses))) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR, "Error parsing given address or port: %s",
                gai_strerror(retval));
        return NULL;

    }

    /* Attempt connection to each address until success */
    current_address = addresses;
    while (current_address != NULL) {

        int retval;

        /* Resolve hostname */
        if ((retval = getnameinfo(current_address->ai_addr,
                current_address->ai_addrlen,
                connected_address, sizeof(connected_address),
                connected_port, sizeof(connected_port),
                NI_NUMERICHOST | NI_NUMERICSERV)))
            guac_client_log(client, GUAC_LOG_INFO, "Unable to resolve host: %s", gai_strerror(retval));

        /* Connect */
        if (connect(fd, current_address->ai_addr,
                        current_address->ai_addrlen) == 0) {

            guac_client_log(client, GUAC_LOG_INFO, "Successfully connected to "
                    "host %s, port %s", connected_address, connected_port);

            /* Done if successful connect */
            break;

        }

        /* Otherwise log information regarding bind failure */
        else
            guac_client_log(client, GUAC_LOG_INFO, "Unable to connect to "
                    "host %s, port %s: %s",
                    connected_address, connected_port, strerror(errno));

        current_address = current_address->ai_next;

    }

    /* If unable to connect to anything, fail */
    if (current_address == NULL) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_UPSTREAM_ERROR, "Unable to connect to any addresses.");
        return NULL;
    }

    /* Free addrinfo */
    freeaddrinfo(addresses);

    /* Open telnet session */
    telnet_t* telnet = telnet_init(__telnet_options, __guac_telnet_event_handler, 0, client);
    if (telnet == NULL) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR, "Telnet client allocation failed.");
        return NULL;
    }

    /* Save file descriptor */
    client_data->socket_fd = fd;

    return telnet;

}

/**
 * Sends a 16-bit value over the given telnet connection with the byte order
 * required by the telnet protocol.
 *
 * @param telnet The telnet connection to use.
 * @param value The value to send.
 */
static void __guac_telnet_send_uint16(telnet_t* telnet, uint16_t value) {

    unsigned char buffer[2];
    buffer[0] = (value >> 8) & 0xFF;
    buffer[1] =  value       & 0xFF;

    telnet_send(telnet, (char*) buffer, 2);

}

/**
 * Sends an 8-bit value over the given telnet connection.
 *
 * @param telnet The telnet connection to use.
 * @param value The value to send.
 */
static void __guac_telnet_send_uint8(telnet_t* telnet, uint8_t value) {
    telnet_send(telnet, (char*) (&value), 1);
}

void guac_telnet_send_naws(telnet_t* telnet, uint16_t width, uint16_t height) {
    telnet_begin_sb(telnet, TELNET_TELOPT_NAWS);
    __guac_telnet_send_uint16(telnet, width);
    __guac_telnet_send_uint16(telnet, height);
    telnet_finish_sb(telnet);
}

void guac_telnet_send_user(telnet_t* telnet, const char* username) {

    /* IAC SB NEW-ENVIRON IS */
    telnet_begin_sb(telnet, TELNET_TELOPT_NEW_ENVIRON);
    __guac_telnet_send_uint8(telnet, TELNET_ENVIRON_IS);

    /* VAR "USER" */
    __guac_telnet_send_uint8(telnet, TELNET_ENVIRON_VAR);
    telnet_send(telnet, "USER", 4);

    /* VALUE username */
    __guac_telnet_send_uint8(telnet, TELNET_ENVIRON_VALUE);
    telnet_send(telnet, username, strlen(username));

    /* IAC SE */
    telnet_finish_sb(telnet);

}

/**
 * Waits for data on the given file descriptor for up to one second. The
 * return value is identical to that of select(): 0 on timeout, < 0 on
 * error, and > 0 on success.
 *
 * @param socket_fd The file descriptor to wait for.
 * @return A value greater than zero on success, zero on timeout, and
 *         less than zero on error.
 */
static int __guac_telnet_wait(int socket_fd) {

    fd_set fds;
    struct timeval timeout;

    FD_ZERO(&fds);
    FD_SET(socket_fd, &fds);

    /* Wait for one second */
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    return select(socket_fd+1, &fds, NULL, NULL, &timeout);

}

void* guac_telnet_client_thread(void* data) {

    guac_client* client = (guac_client*) data;
    guac_telnet_client_data* client_data = (guac_telnet_client_data*) client->data;

    pthread_t input_thread;
    char buffer[8192];
    int wait_result;

    /* Open telnet session */
    client_data->telnet = __guac_telnet_create_session(client);
    if (client_data->telnet == NULL) {
        /* Already aborted within __guac_telnet_create_session() */
        return NULL;
    }

    /* Logged in */
    guac_client_log(client, GUAC_LOG_INFO, "Telnet connection successful.");

    /* Start input thread */
    if (pthread_create(&(input_thread), NULL, __guac_telnet_input_thread, (void*) client)) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR, "Unable to start input thread");
        return NULL;
    }

    /* While data available, write to terminal */
    while ((wait_result = __guac_telnet_wait(client_data->socket_fd)) >= 0) {

        /* Resume waiting of no data available */
        if (wait_result == 0)
            continue;

        int bytes_read = read(client_data->socket_fd, buffer, sizeof(buffer));
        if (bytes_read <= 0)
            break;

        telnet_recv(client_data->telnet, buffer, bytes_read);

    }

    /* Kill client and Wait for input thread to die */
    guac_client_stop(client);
    pthread_join(input_thread, NULL);

    guac_client_log(client, GUAC_LOG_INFO, "Telnet connection ended.");
    return NULL;

}

