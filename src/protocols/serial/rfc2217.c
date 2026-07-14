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

#include "rfc2217.h"
#include "settings.h"
#include "stream.h"

#include <guacamole/client.h>
#include <guacamole/protocol.h>

#ifdef ENABLE_SERIAL_RFC2217

#include "serial.h"
#include "terminal/terminal.h"

#include <guacamole/error.h>
#include <guacamole/tcp.h>
#include <libtelnet.h>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdint.h>
#include <unistd.h>

/**
 * The telnet option number of the RFC2217 COM-PORT-OPTION. libtelnet does not
 * define this option, so it is defined here.
 */
#ifndef TELNET_TELOPT_COMPORT
#define TELNET_TELOPT_COMPORT 44
#endif

/**
 * COM-PORT-OPTION client-to-server command: set the serial line speed. The
 * data is a 4-byte big-endian unsigned integer giving the speed in bits per
 * second.
 */
#define GUAC_SERIAL_RFC2217_SET_BAUDRATE 1

/**
 * COM-PORT-OPTION client-to-server command: set the number of data bits. The
 * data is a single byte (5 through 8).
 */
#define GUAC_SERIAL_RFC2217_SET_DATASIZE 2

/**
 * COM-PORT-OPTION client-to-server command: set the parity. The data is a
 * single byte (1=none, 2=odd, 3=even, 4=mark, 5=space).
 */
#define GUAC_SERIAL_RFC2217_SET_PARITY 3

/**
 * COM-PORT-OPTION client-to-server command: set the number of stop bits. The
 * data is a single byte (1=1 stop bit, 2=2 stop bits).
 */
#define GUAC_SERIAL_RFC2217_SET_STOPSIZE 4

/**
 * COM-PORT-OPTION client-to-server command: set line/flow control and break
 * state. The data is a single byte. For flow control: 1=none, 2=xon/xoff,
 * 3=rts/cts. For the break signal: 5=break on, 6=break off.
 */
#define GUAC_SERIAL_RFC2217_SET_CONTROL 5

/**
 * Writes the entire buffer given to the specified file descriptor, retrying
 * the write automatically if necessary.
 *
 * @param fd
 *     The file descriptor to write to.
 *
 * @param buffer
 *     The buffer to write.
 *
 * @param size
 *     The number of bytes from the buffer to write.
 *
 * @return
 *     The number of bytes written, or a value not equal to size if an error
 *     prevents all future writes.
 */
static int guac_serial_rfc2217_write_all(int fd, const char* buffer, int size) {

    int remaining = size;
    while (remaining > 0) {

        int ret_val;
        GUAC_RETRY_EINTR(ret_val, write(fd, buffer, remaining));
        if (ret_val <= 0)
            return -1;

        remaining -= ret_val;
        buffer += ret_val;

    }

    return size;

}

/**
 * Sends a single-byte COM-PORT-OPTION subnegotiation.
 */
static void guac_serial_rfc2217_send_byte(telnet_t* telnet,
        unsigned char command, unsigned char value) {
    telnet_begin_sb(telnet, TELNET_TELOPT_COMPORT);
    telnet_send(telnet, (char*) &command, 1);
    telnet_send(telnet, (char*) &value, 1);
    telnet_finish_sb(telnet);
}

/**
 * Sends the serial line parameters described by the given settings using
 * client-to-server COM-PORT-OPTION subnegotiations.
 */
static void guac_serial_rfc2217_set_line(telnet_t* telnet,
        guac_serial_settings* settings) {

    /* SET-BAUDRATE: 4-byte big-endian speed in bits per second */
    uint32_t baud = (uint32_t) settings->baud_rate;
    unsigned char rate[5] = {
        GUAC_SERIAL_RFC2217_SET_BAUDRATE,
        (unsigned char) ((baud >> 24) & 0xFF),
        (unsigned char) ((baud >> 16) & 0xFF),
        (unsigned char) ((baud >>  8) & 0xFF),
        (unsigned char) ( baud        & 0xFF)
    };
    telnet_begin_sb(telnet, TELNET_TELOPT_COMPORT);
    telnet_send(telnet, (char*) rate, sizeof(rate));
    telnet_finish_sb(telnet);

    /* SET-DATASIZE */
    guac_serial_rfc2217_send_byte(telnet, GUAC_SERIAL_RFC2217_SET_DATASIZE,
            (unsigned char) settings->data_bits);

    /* SET-PARITY (1=none, 2=odd, 3=even, 4=mark, 5=space) */
    unsigned char parity;
    switch (settings->parity) {
        case GUAC_SERIAL_PARITY_ODD:   parity = 2; break;
        case GUAC_SERIAL_PARITY_EVEN:  parity = 3; break;
        case GUAC_SERIAL_PARITY_MARK:  parity = 4; break;
        case GUAC_SERIAL_PARITY_SPACE: parity = 5; break;
        default:                       parity = 1; break;
    }
    guac_serial_rfc2217_send_byte(telnet, GUAC_SERIAL_RFC2217_SET_PARITY,
            parity);

    /* SET-STOPSIZE (1=1 stop bit, 2=2 stop bits) */
    guac_serial_rfc2217_send_byte(telnet, GUAC_SERIAL_RFC2217_SET_STOPSIZE,
            (unsigned char) settings->stop_bits);

    /* SET-CONTROL flow control (1=none, 2=xon/xoff, 3=rts/cts) */
    unsigned char flow;
    switch (settings->flow_control) {
        case GUAC_SERIAL_FLOW_XON_XOFF: flow = 2; break;
        case GUAC_SERIAL_FLOW_RTS_CTS:  flow = 3; break;
        default:                        flow = 1; break;
    }
    guac_serial_rfc2217_send_byte(telnet, GUAC_SERIAL_RFC2217_SET_CONTROL,
            flow);

}

/**
 * Event handler, as defined by libtelnet. Invoked for every event fired by the
 * RFC2217 libtelnet session. The user data is the guac_serial_stream.
 */
static void guac_serial_rfc2217_event_handler(telnet_t* telnet,
        telnet_event_t* event, void* data) {

    guac_serial_stream* stream = (guac_serial_stream*) data;
    guac_client* client = stream->client;
    guac_serial_client* serial_client = (guac_serial_client*) client->data;

    switch (event->type) {

        /* Terminal output received */
        case TELNET_EV_DATA:
            guac_terminal_write(serial_client->term, event->data.buffer,
                    event->data.size);
            break;

        /* Data destined for remote end */
        case TELNET_EV_SEND:
            if (guac_serial_rfc2217_write_all(stream->fd, event->data.buffer,
                    event->data.size) != (int) event->data.size)
                guac_client_stop(client);
            break;

        /* Connection warnings */
        case TELNET_EV_WARNING:
            guac_client_log(client, GUAC_LOG_WARNING, "%s", event->error.msg);
            break;

        /* Connection errors */
        case TELNET_EV_ERROR:
            guac_client_abort(client, GUAC_PROTOCOL_STATUS_UPSTREAM_ERROR,
                    "RFC2217 connection closing with error: %s",
                    event->error.msg);
            break;

        /* Ignore other events */
        default:
            break;

    }

}

int guac_serial_rfc2217_open(guac_serial_stream* stream, guac_client* client,
        guac_serial_settings* settings) {

    /* Support levels for the telnet options used by the RFC2217 session */
    static const telnet_telopt_t rfc2217_options[] = {
        { TELNET_TELOPT_COMPORT, TELNET_WILL, TELNET_DONT },
        { TELNET_TELOPT_BINARY,  TELNET_WILL, TELNET_DO   },
        { TELNET_TELOPT_SGA,     TELNET_WILL, TELNET_DO   },
        { TELNET_TELOPT_ECHO,    TELNET_WONT, TELNET_DO   },
        { -1, 0, 0 }
    };

    /* Connect to the remote serial server */
    int fd = guac_tcp_connect(settings->hostname, settings->port,
            settings->timeout);
    if (fd < 0) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_UPSTREAM_NOT_FOUND,
                "Unable to connect to serial server \"%s\" port \"%s\".",
                settings->hostname, settings->port);
        return -1;
    }

    /* Disable Nagle's algorithm so keystrokes are sent without delay */
    int flag = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

    /* Store the socket before initializing libtelnet, as the event handler
     * writes outbound data to this descriptor */
    stream->backend = GUAC_SERIAL_BACKEND_RFC2217;
    stream->fd = fd;

    /* Initialize the libtelnet session, passing the stream as user data */
    telnet_t* telnet = telnet_init(rfc2217_options,
            guac_serial_rfc2217_event_handler, 0, stream);
    if (telnet == NULL) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR,
                "RFC2217 client allocation failed.");
        close(fd);
        stream->fd = -1;
        return -1;
    }

    stream->telnet = telnet;

    /* Advertise our willingness to use COM-PORT-OPTION and binary mode */
    telnet_negotiate(telnet, TELNET_WILL, TELNET_TELOPT_COMPORT);
    telnet_negotiate(telnet, TELNET_WILL, TELNET_TELOPT_BINARY);
    telnet_negotiate(telnet, TELNET_DO, TELNET_TELOPT_BINARY);

    /* Push the configured serial line parameters to the remote end */
    guac_serial_rfc2217_set_line(telnet, settings);

    return 0;

}

void guac_serial_rfc2217_send(guac_serial_stream* stream, const char* buffer,
        int length) {
    telnet_send((telnet_t*) stream->telnet, buffer, length);
}

void guac_serial_rfc2217_recv(guac_serial_stream* stream, const char* buffer,
        int length) {
    telnet_recv((telnet_t*) stream->telnet, buffer, length);
}

int guac_serial_rfc2217_send_break(guac_serial_stream* stream) {

    telnet_t* telnet = (telnet_t*) stream->telnet;

    /* SET-CONTROL: break on */
    guac_serial_rfc2217_send_byte(telnet, GUAC_SERIAL_RFC2217_SET_CONTROL, 5);

    /* Hold the line low for the configured duration */
    if (stream->break_duration > 0)
        usleep((useconds_t) stream->break_duration * 1000);

    /* SET-CONTROL: break off */
    guac_serial_rfc2217_send_byte(telnet, GUAC_SERIAL_RFC2217_SET_CONTROL, 6);

    return 0;

}

void guac_serial_rfc2217_free(guac_serial_stream* stream) {
    if (stream->telnet != NULL) {
        telnet_free((telnet_t*) stream->telnet);
        stream->telnet = NULL;
    }
}

#else /* ENABLE_SERIAL_RFC2217 */

int guac_serial_rfc2217_open(guac_serial_stream* stream, guac_client* client,
        guac_serial_settings* settings) {

    (void) stream;
    (void) settings;

    guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR,
            "RFC2217 support was not compiled in "
            "(rebuild guacamole-server with libtelnet).");
    return -1;

}

void guac_serial_rfc2217_send(guac_serial_stream* stream, const char* buffer,
        int length) {
    /* RFC2217 unavailable; a stream with this backend is never created */
    (void) stream;
    (void) buffer;
    (void) length;
}

void guac_serial_rfc2217_recv(guac_serial_stream* stream, const char* buffer,
        int length) {
    /* RFC2217 unavailable; a stream with this backend is never created */
    (void) stream;
    (void) buffer;
    (void) length;
}

int guac_serial_rfc2217_send_break(guac_serial_stream* stream) {
    /* RFC2217 unavailable; a stream with this backend is never created */
    (void) stream;
    return 0;
}

void guac_serial_rfc2217_free(guac_serial_stream* stream) {
    /* No libtelnet session can exist when RFC2217 is not compiled in */
    (void) stream;
}

#endif /* ENABLE_SERIAL_RFC2217 */
