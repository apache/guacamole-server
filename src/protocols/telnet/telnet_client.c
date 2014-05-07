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
#include "guac_handlers.h"

#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <libtelnet.h>
#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>

static const telnet_telopt_t __telnet_options[] = {
    { TELNET_TELOPT_ECHO,      TELNET_WONT, TELNET_DONT },
    { TELNET_TELOPT_TTYPE,     TELNET_WILL, TELNET_DONT },
    { TELNET_TELOPT_COMPRESS2, TELNET_WONT, TELNET_DO   },
    { TELNET_TELOPT_MSSP,      TELNET_WONT, TELNET_DO   },
    { -1, 0, 0 }
};

static int __write_all(int fd, const char* buffer, int size) {

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

static void __guac_telnet_event_handler(telnet_t* telnet, telnet_event_t* event, void* data) {

    guac_client* client = (guac_client*) data;
    telnet_client_data* client_data = (telnet_client_data*) client->data;

    switch (event->type) {

        /* User input received */
        case TELNET_EV_DATA:
            guac_terminal_write_stdout(client_data->term, event->data.buffer, event->data.size);
            break;

        /* Data destined for remote end */
        case TELNET_EV_SEND:
            if (__write_all(client_data->socket_fd, event->data.buffer, event->data.size) != event->data.size)
                guac_client_stop(client);
            break;

        /* Terminal type request */
        case TELNET_EV_TTYPE:
            if (event->ttype.cmd == TELNET_TTYPE_SEND)
                telnet_ttype_is(client_data->telnet, "linux");
            break;

        /* Connection warnings */
        case TELNET_EV_WARNING:
            guac_client_log_info(client, "%s", event->error.msg);
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

void* telnet_input_thread(void* data) {

    guac_client* client = (guac_client*) data;
    telnet_client_data* client_data = (telnet_client_data*) client->data;

    char buffer[8192];
    int bytes_read;

    /* Write all data read */
    while ((bytes_read = guac_terminal_read_stdin(client_data->term, buffer, sizeof(buffer))) > 0)
        telnet_send(client_data->telnet, buffer, bytes_read);

    return NULL;

}

static telnet_t* __guac_telnet_create_session(guac_client* client) {

    int retval;

    int fd;
    struct addrinfo* addresses;
    struct addrinfo* current_address;

    char connected_address[1024];
    char connected_port[64];

    telnet_client_data* client_data = (telnet_client_data*) client->data;

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
            guac_client_log_info(client, "Unable to resolve host: %s", gai_strerror(retval));

        /* Connect */
        if (connect(fd, current_address->ai_addr,
                        current_address->ai_addrlen) == 0) {

            guac_client_log_info(client, "Successfully connected to "
                    "host %s, port %s", connected_address, connected_port);

            /* Done if successful connect */
            break;

        }

        /* Otherwise log information regarding bind failure */
        else
            guac_client_log_info(client, "Unable to connect to "
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

void* telnet_client_thread(void* data) {

    guac_client* client = (guac_client*) data;
    telnet_client_data* client_data = (telnet_client_data*) client->data;

    pthread_t input_thread;
    char buffer[8192];
    int bytes_read;

    /* Open telnet session */
    client_data->telnet = __guac_telnet_create_session(client);
    if (client_data->telnet == NULL) {
        /* Already aborted within __guac_telnet_create_session() */
        return NULL;
    }

    /* Logged in */
    guac_client_log_info(client, "Telnet connection successful.");

    /* Start input thread */
    if (pthread_create(&(input_thread), NULL, telnet_input_thread, (void*) client)) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR, "Unable to start input thread");
        return NULL;
    }

    /* While data available, write to terminal */
    while ((bytes_read = read(client_data->socket_fd, buffer, sizeof(buffer))) > 0)
        telnet_recv(client_data->telnet, buffer, bytes_read);

    /* Kill client and Wait for input thread to die */
    guac_client_stop(client);
    pthread_join(input_thread, NULL);

    guac_client_log_info(client, "Telnet connection ended.");
    return NULL;

}

