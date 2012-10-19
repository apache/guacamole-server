
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
 * David PHAM-VAN <d.pham-van@ulteo.com> Ulteo SAS - http://www.ulteo.com
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
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#ifdef __MINGW32__
#include <winsock2.h>
#else
#include <sys/select.h>
#endif

#include <time.h>
#include <sys/time.h>

#include "socket.h"
#include "error.h"

ssize_t __guac_socket_fd_read_handler(guac_socket* socket,
        void* buf, size_t count) {

    guac_socket_fd_data* data = (guac_socket_fd_data*) socket->data;

    /* Read from socket */
    int retval = read(data->fd, buf, count);

    /* Record errors in guac_error */
    if (retval < 0) {
        guac_error = GUAC_STATUS_SEE_ERRNO;
        guac_error_message = "Error reading data from socket";
    }

    return retval;

}

ssize_t __guac_socket_fd_write_handler(guac_socket* socket,
        void* buf, size_t count) {

    guac_socket_fd_data* data = (guac_socket_fd_data*) socket->data;
    int retval;

#ifdef __MINGW32__
    /* MINGW32 WINSOCK only works with send() */
    retval = send(data->fd, buf, count, 0);
#else
    /* Use write() for all other platforms */
    retval = write(data->fd, buf, count);
#endif

    /* Record errors in guac_error */
    if (retval < 0) {
        guac_error = GUAC_STATUS_SEE_ERRNO;
        guac_error_message = "Error writing data to socket";
    }

    return retval;
}


int __guac_socket_fd_select_handler(guac_socket* socket, int usec_timeout) {

    guac_socket_fd_data* data = (guac_socket_fd_data*) socket->data;

    fd_set fds;
    struct timeval timeout;
    int retval;

    /* No timeout if usec_timeout is negative */
    if (usec_timeout < 0)
        retval = select(data->fd + 1, &fds, NULL, NULL, NULL); 

    /* Handle timeout if specified */
    else {
        timeout.tv_sec = usec_timeout/1000000;
        timeout.tv_usec = usec_timeout%1000000;

        FD_ZERO(&fds);
        FD_SET(data->fd, &fds);

        retval = select(data->fd + 1, &fds, NULL, NULL, &timeout);
    }

    /* Properly set guac_error */
    if (retval <  0) {
        guac_error = GUAC_STATUS_SEE_ERRNO;
        guac_error_message = "Error while waiting for data on socket";
    }

    if (retval == 0) {
        guac_error = GUAC_STATUS_INPUT_TIMEOUT;
        guac_error_message = "Timeout while waiting for data on socket";
    }

    return retval;

}

guac_socket* guac_socket_open(int fd) {

    /* Allocate socket and associated data */
    guac_socket* socket = guac_socket_alloc();
    guac_socket_fd_data* data = malloc(sizeof(guac_socket_fd_data));

    /* Store file descriptor as socket data */
    data->fd = fd;
    socket->data = data;

    /* Set read/write handlers */
    socket->read_handler   = __guac_socket_fd_read_handler;
    socket->write_handler  = __guac_socket_fd_write_handler;
    socket->select_handler = __guac_socket_fd_select_handler;

    return socket;

}

