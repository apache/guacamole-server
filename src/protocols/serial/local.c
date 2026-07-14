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

#include "local.h"
#include "settings.h"
#include "stream.h"

#include <guacamole/client.h>
#include <guacamole/protocol.h>

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

int guac_serial_local_open(guac_serial_stream* stream, guac_client* client,
        guac_serial_settings* settings) {

    /* Open device non-blocking so open() does not wait for carrier detect */
    int fd = open(settings->device, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        int status = (errno == ENOENT)
                ? GUAC_PROTOCOL_STATUS_UPSTREAM_NOT_FOUND
                : GUAC_PROTOCOL_STATUS_SERVER_ERROR;
        guac_client_abort(client, status,
                "Unable to open serial device \"%s\": %s",
                settings->device, strerror(errno));
        return -1;
    }

    /* Request exclusive access; warn but continue if unsupported */
    if (ioctl(fd, TIOCEXCL) != 0)
        guac_client_log(client, GUAC_LOG_WARNING, "Unable to obtain exclusive "
                "access to serial device \"%s\": %s", settings->device,
                strerror(errno));

    /* Read current line settings */
    struct termios tio;
    if (tcgetattr(fd, &tio) != 0) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR,
                "Unable to read attributes of serial device \"%s\": %s",
                settings->device, strerror(errno));
        close(fd);
        return -1;
    }

    /* Start from a fully raw configuration */
    cfmakeraw(&tio);

    /* Apply line speed */
    speed_t speed = guac_serial_baud_to_speed(settings->baud_rate);
    cfsetispeed(&tio, speed);
    cfsetospeed(&tio, speed);

    /* Ignore modem control lines and enable the receiver */
    tio.c_cflag |= (CLOCAL | CREAD);

    /* Apply data bits */
    tio.c_cflag &= ~CSIZE;
    switch (settings->data_bits) {
        case 5: tio.c_cflag |= CS5; break;
        case 6: tio.c_cflag |= CS6; break;
        case 7: tio.c_cflag |= CS7; break;
        default: tio.c_cflag |= CS8; break;
    }

    /* Apply parity */
    switch (settings->parity) {

        case GUAC_SERIAL_PARITY_NONE:
            tio.c_cflag &= ~PARENB;
            break;

        case GUAC_SERIAL_PARITY_EVEN:
            tio.c_cflag |= PARENB;
            tio.c_cflag &= ~PARODD;
            break;

        case GUAC_SERIAL_PARITY_ODD:
            tio.c_cflag |= (PARENB | PARODD);
            break;

        case GUAC_SERIAL_PARITY_MARK:
#ifdef CMSPAR
            tio.c_cflag |= (PARENB | CMSPAR | PARODD);
#else
            guac_client_log(client, GUAC_LOG_WARNING, "Mark parity is not "
                    "supported on this platform; using no parity.");
            tio.c_cflag &= ~PARENB;
#endif
            break;

        case GUAC_SERIAL_PARITY_SPACE:
#ifdef CMSPAR
            tio.c_cflag |= (PARENB | CMSPAR);
            tio.c_cflag &= ~PARODD;
#else
            guac_client_log(client, GUAC_LOG_WARNING, "Space parity is not "
                    "supported on this platform; using no parity.");
            tio.c_cflag &= ~PARENB;
#endif
            break;

    }

    /* Apply stop bits */
    if (settings->stop_bits == 2)
        tio.c_cflag |= CSTOPB;
    else
        tio.c_cflag &= ~CSTOPB;

    /* Apply flow control */
    switch (settings->flow_control) {

        case GUAC_SERIAL_FLOW_NONE:
            tio.c_iflag &= ~(IXON | IXOFF | IXANY);
#ifdef CRTSCTS
            tio.c_cflag &= ~CRTSCTS;
#endif
            break;

        case GUAC_SERIAL_FLOW_RTS_CTS:
            tio.c_iflag &= ~(IXON | IXOFF | IXANY);
#ifdef CRTSCTS
            tio.c_cflag |= CRTSCTS;
#else
            guac_client_log(client, GUAC_LOG_WARNING, "Hardware (RTS/CTS) flow "
                    "control is not supported on this platform; using no flow "
                    "control.");
#endif
            break;

        case GUAC_SERIAL_FLOW_XON_XOFF:
#ifdef CRTSCTS
            tio.c_cflag &= ~CRTSCTS;
#endif
            tio.c_iflag |= (IXON | IXOFF);
            break;

    }

    /* Block for at least one byte per read, with no inter-byte timeout */
    tio.c_cc[VMIN] = 1;
    tio.c_cc[VTIME] = 0;

    /* Apply configuration immediately and discard any pending I/O */
    if (tcsetattr(fd, TCSANOW, &tio) != 0) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR,
                "Unable to configure serial device \"%s\": %s",
                settings->device, strerror(errno));
        close(fd);
        return -1;
    }
    tcflush(fd, TCIOFLUSH);

    stream->backend = GUAC_SERIAL_BACKEND_LOCAL;
    stream->fd = fd;
    return 0;

}
