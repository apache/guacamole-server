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

#include "rfc2217.h"
#include "settings.h"
#include "stream.h"
#include "tcp.h"

#include <guacamole/client.h>
#include <guacamole/protocol.h>

#ifdef ENABLE_SERIAL_RFC2217

#include "serial.h"
#include "terminal/terminal.h"

#include <guacamole/error.h>
#include <guacamole/tcp.h>
#include <libtelnet.h>

#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

/**
 * The telnet option number of the RFC2217 COM-PORT-OPTION. libtelnet does not
 * define this option, so it is defined here.
 */
#define GUAC_SERIAL_TELOPT_COMPORT 44

/**
 * The client-to-server command codes of the RFC2217 COM-PORT-OPTION, as sent
 * in the first byte of each COM-PORT-OPTION subnegotiation. The values are
 * defined by RFC 2217 and must not be changed.
 */
typedef enum guac_serial_rfc2217_command {

    /**
     * Set the serial line speed. The data is a 4-byte big-endian unsigned
     * integer giving the speed in bits per second.
     */
    GUAC_SERIAL_RFC2217_SET_BAUDRATE = 1,

    /**
     * Set the number of data bits. The data is a single byte (5 through 8).
     */
    GUAC_SERIAL_RFC2217_SET_DATASIZE = 2,

    /**
     * Set the parity. The data is a single guac_serial_rfc2217_parity
     * value.
     */
    GUAC_SERIAL_RFC2217_SET_PARITY = 3,

    /**
     * Set the number of stop bits. The data is a single byte (1=1 stop bit,
     * 2=2 stop bits).
     */
    GUAC_SERIAL_RFC2217_SET_STOPSIZE = 4,

    /**
     * Set flow control, the break state, or the DTR/RTS modem control lines.
     * The data is a single guac_serial_rfc2217_control value.
     */
    GUAC_SERIAL_RFC2217_SET_CONTROL = 5

} guac_serial_rfc2217_command;

/**
 * The values of the RFC2217 COM-PORT-OPTION SET-PARITY command. The values are
 * defined by RFC 2217 and must not be changed.
 */
typedef enum guac_serial_rfc2217_parity {

    /**
     * No parity bit.
     */
    GUAC_SERIAL_RFC2217_PARITY_NONE = 1,

    /**
     * Odd parity.
     */
    GUAC_SERIAL_RFC2217_PARITY_ODD = 2,

    /**
     * Even parity.
     */
    GUAC_SERIAL_RFC2217_PARITY_EVEN = 3,

    /**
     * The parity bit is always set (mark).
     */
    GUAC_SERIAL_RFC2217_PARITY_MARK = 4,

    /**
     * The parity bit is always clear (space).
     */
    GUAC_SERIAL_RFC2217_PARITY_SPACE = 5

} guac_serial_rfc2217_parity;

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
 * Sends a COM-PORT-OPTION subnegotiation consisting of the given command byte
 * followed by a single data byte, as used by all RFC2217 client-to-server
 * commands other than SET-BAUDRATE.
 *
 * @param telnet
 *     The libtelnet session over which the subnegotiation should be sent.
 *
 * @param command
 *     The COM-PORT-OPTION command code, such as
 *     GUAC_SERIAL_RFC2217_SET_DATASIZE.
 *
 * @param value
 *     The single byte of data associated with the command.
 */
static void guac_serial_rfc2217_send_byte(telnet_t* telnet,
        guac_serial_rfc2217_command command, unsigned char value) {
    unsigned char code = (unsigned char) command;
    telnet_begin_sb(telnet, GUAC_SERIAL_TELOPT_COMPORT);
    telnet_send(telnet, (char*) &code, 1);
    telnet_send(telnet, (char*) &value, 1);
    telnet_finish_sb(telnet);
}

/**
 * Sends the serial line parameters described by the given settings (baud rate,
 * data bits, parity, stop bits and flow control) to the remote RFC2217 server
 * using client-to-server COM-PORT-OPTION subnegotiations.
 *
 * @param telnet
 *     The libtelnet session over which the subnegotiations should be sent.
 *
 * @param settings
 *     The parsed connection settings containing the serial line parameters to
 *     send.
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
    telnet_begin_sb(telnet, GUAC_SERIAL_TELOPT_COMPORT);
    telnet_send(telnet, (char*) rate, sizeof(rate));
    telnet_finish_sb(telnet);

    /* SET-DATASIZE */
    guac_serial_rfc2217_send_byte(telnet, GUAC_SERIAL_RFC2217_SET_DATASIZE,
            (unsigned char) settings->data_bits);

    /* SET-PARITY */
    guac_serial_rfc2217_parity parity;
    switch (settings->parity) {

        case GUAC_SERIAL_PARITY_ODD:
            parity = GUAC_SERIAL_RFC2217_PARITY_ODD;
            break;

        case GUAC_SERIAL_PARITY_EVEN:
            parity = GUAC_SERIAL_RFC2217_PARITY_EVEN;
            break;

        case GUAC_SERIAL_PARITY_MARK:
            parity = GUAC_SERIAL_RFC2217_PARITY_MARK;
            break;

        case GUAC_SERIAL_PARITY_SPACE:
            parity = GUAC_SERIAL_RFC2217_PARITY_SPACE;
            break;

        default:
            parity = GUAC_SERIAL_RFC2217_PARITY_NONE;
            break;

    }
    guac_serial_rfc2217_send_byte(telnet, GUAC_SERIAL_RFC2217_SET_PARITY,
            (unsigned char) parity);

    /* SET-STOPSIZE (1=1 stop bit, 2=2 stop bits) */
    guac_serial_rfc2217_send_byte(telnet, GUAC_SERIAL_RFC2217_SET_STOPSIZE,
            (unsigned char) settings->stop_bits);

    /* SET-CONTROL flow control */
    guac_serial_rfc2217_control flow;
    switch (settings->flow_control) {

        case GUAC_SERIAL_FLOW_XON_XOFF:
            flow = GUAC_SERIAL_RFC2217_CONTROL_FLOW_XON_XOFF;
            break;

        case GUAC_SERIAL_FLOW_RTS_CTS:
            flow = GUAC_SERIAL_RFC2217_CONTROL_FLOW_RTS_CTS;
            break;

        default:
            flow = GUAC_SERIAL_RFC2217_CONTROL_FLOW_NONE;
            break;

    }
    guac_serial_rfc2217_send_byte(telnet, GUAC_SERIAL_RFC2217_SET_CONTROL,
            (unsigned char) flow);

}

/**
 * Event handler, as defined by libtelnet. Invoked for every event fired by the
 * libtelnet session of either the RFC2217 or the plain Telnet backend.
 * Received data is written to the terminal, outbound data is written to the
 * stream's socket, warnings are logged and errors abort the connection.
 *
 * @param telnet
 *     The libtelnet session which fired the event.
 *
 * @param event
 *     The event which was fired.
 *
 * @param data
 *     The user data associated with the libtelnet session, which is the
 *     guac_serial_stream that owns the session.
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
        { GUAC_SERIAL_TELOPT_COMPORT, TELNET_WILL, TELNET_DONT },
        { TELNET_TELOPT_BINARY,  TELNET_WILL, TELNET_DO   },
        { TELNET_TELOPT_SGA,     TELNET_WILL, TELNET_DO   },
        { TELNET_TELOPT_ECHO,    TELNET_WONT, TELNET_DO   },
        { -1, 0, 0 }
    };

    /* Acquire the network socket (dialing out, or accepting an inbound
     * connection in reverse mode). On failure open_status is already set. */
    int fd = guac_serial_net_open_fd(stream, client, settings);
    if (fd < 0)
        return -1;

    /* Store the socket before initializing libtelnet, as the event handler
     * writes outbound data to this descriptor */
    stream->backend = GUAC_SERIAL_BACKEND_RFC2217;
    stream->fd = fd;

    /* Initialize the libtelnet session, passing the stream as user data */
    telnet_t* telnet = telnet_init(rfc2217_options,
            guac_serial_rfc2217_event_handler, 0, stream);
    if (telnet == NULL) {
        guac_client_log(client, GUAC_LOG_ERROR,
                "RFC2217 client allocation failed.");
        stream->open_status = GUAC_PROTOCOL_STATUS_SERVER_ERROR;
        close(fd);
        stream->fd = -1;
        return -1;
    }

    stream->telnet = telnet;

    /* Advertise our willingness to use COM-PORT-OPTION and binary mode */
    telnet_negotiate(telnet, TELNET_WILL, GUAC_SERIAL_TELOPT_COMPORT);
    telnet_negotiate(telnet, TELNET_WILL, TELNET_TELOPT_BINARY);
    telnet_negotiate(telnet, TELNET_DO, TELNET_TELOPT_BINARY);

    /* Push the configured serial line parameters to the remote end */
    guac_serial_rfc2217_set_line(telnet, settings);

    return 0;

}

int guac_serial_telnet_open(guac_serial_stream* stream, guac_client* client,
        guac_serial_settings* settings) {

    /* Support levels for the telnet options used by the base Telnet session.
     * Unlike RFC2217, no COM-PORT-OPTION is negotiated. */
    static const telnet_telopt_t telnet_options[] = {
        { TELNET_TELOPT_BINARY, TELNET_WILL, TELNET_DO   },
        { TELNET_TELOPT_SGA,    TELNET_WILL, TELNET_DO   },
        { TELNET_TELOPT_ECHO,   TELNET_WONT, TELNET_DO   },
        { -1, 0, 0 }
    };

    /* Acquire the network socket (dialing out, or accepting an inbound
     * connection in reverse mode). On failure open_status is already set. */
    int fd = guac_serial_net_open_fd(stream, client, settings);
    if (fd < 0)
        return -1;

    /* Store the socket before initializing libtelnet, as the event handler
     * writes outbound data to this descriptor */
    stream->backend = GUAC_SERIAL_BACKEND_TELNET;
    stream->fd = fd;

    /* Initialize the libtelnet session, reusing the shared event handler and
     * passing the stream as user data */
    telnet_t* telnet = telnet_init(telnet_options,
            guac_serial_rfc2217_event_handler, 0, stream);
    if (telnet == NULL) {
        guac_client_log(client, GUAC_LOG_ERROR,
                "Telnet client allocation failed.");
        stream->open_status = GUAC_PROTOCOL_STATUS_SERVER_ERROR;
        close(fd);
        stream->fd = -1;
        return -1;
    }

    stream->telnet = telnet;

    /* Advertise our willingness to use binary mode and suppress go-ahead */
    telnet_negotiate(telnet, TELNET_WILL, TELNET_TELOPT_BINARY);
    telnet_negotiate(telnet, TELNET_DO, TELNET_TELOPT_BINARY);
    telnet_negotiate(telnet, TELNET_WILL, TELNET_TELOPT_SGA);

    /* Plain telnet cannot negotiate serial line parameters; they remain
     * informational, so no COM-PORT-OPTION line push is performed. */

    return 0;

}

int guac_serial_telnet_send_break(guac_serial_stream* stream) {

    /* Plain telnet has no COM-PORT-OPTION break, so the NVT BREAK function is
     * used instead. Its effect depends entirely on the remote end. telnet_iac
     * sends "IAC <cmd>", i.e. "IAC BRK". */
    telnet_iac((telnet_t*) stream->telnet, TELNET_BREAK);
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
    guac_serial_rfc2217_send_byte(telnet, GUAC_SERIAL_RFC2217_SET_CONTROL,
            GUAC_SERIAL_RFC2217_CONTROL_BREAK_ON);

    /* Hold the line low for the configured duration */
    if (stream->break_duration > 0)
        usleep((useconds_t) stream->break_duration * 1000);

    /* SET-CONTROL: break off */
    guac_serial_rfc2217_send_byte(telnet, GUAC_SERIAL_RFC2217_SET_CONTROL,
            GUAC_SERIAL_RFC2217_CONTROL_BREAK_OFF);

    return 0;

}

void guac_serial_rfc2217_set_control(guac_serial_stream* stream,
        guac_serial_rfc2217_control value) {
    guac_serial_rfc2217_send_byte((telnet_t*) stream->telnet,
            GUAC_SERIAL_RFC2217_SET_CONTROL, (unsigned char) value);
}

void guac_serial_rfc2217_free(guac_serial_stream* stream) {
    if (stream->telnet != NULL) {
        telnet_free((telnet_t*) stream->telnet);
        stream->telnet = NULL;
    }
}

#else /* !ENABLE_SERIAL_RFC2217 */

/*
 * libtelnet is not available, so neither the RFC2217 nor the Telnet transport
 * is compiled in. The functions below fail (open) or do nothing (all others),
 * allowing the rest of the plugin to link and run with those transports
 * unavailable.
 */

int guac_serial_rfc2217_open(guac_serial_stream* stream, guac_client* client,
        guac_serial_settings* settings) {

    (void) settings;

    guac_client_log(client, GUAC_LOG_ERROR, "RFC2217 support was not compiled "
            "in (rebuild guacamole-server with libtelnet).");
    stream->open_status = GUAC_PROTOCOL_STATUS_SERVER_ERROR;
    return -1;

}

int guac_serial_telnet_open(guac_serial_stream* stream, guac_client* client,
        guac_serial_settings* settings) {

    (void) settings;

    guac_client_log(client, GUAC_LOG_ERROR, "Telnet transport support was not "
            "compiled in (rebuild guacamole-server with libtelnet).");
    stream->open_status = GUAC_PROTOCOL_STATUS_SERVER_ERROR;
    return -1;

}

int guac_serial_telnet_send_break(guac_serial_stream* stream) {
    /* Telnet unavailable; a stream with this backend is never created */
    (void) stream;
    return 0;
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

void guac_serial_rfc2217_set_control(guac_serial_stream* stream,
        guac_serial_rfc2217_control value) {
    /* RFC2217 unavailable; a stream with this backend is never created */
    (void) stream;
    (void) value;
}

void guac_serial_rfc2217_free(guac_serial_stream* stream) {
    /* No libtelnet session can exist when RFC2217 is not compiled in */
    (void) stream;
}

#endif /* ENABLE_SERIAL_RFC2217 */
