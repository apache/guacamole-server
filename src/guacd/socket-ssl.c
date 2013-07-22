
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
 * The Original Code is guacd.
 *
 * The Initial Developer of the Original Code is
 * Michael Jumper.
 * Portions created by the Initial Developer are Copyright (C) 2010
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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
#include <sys/select.h>

#include <openssl/ssl.h>

#include <guacamole/socket.h>
#include <guacamole/error.h>

#include "socket-ssl.h"


static ssize_t __guac_socket_ssl_read_handler(guac_socket* socket,
        void* buf, size_t count) {

    /* Read from socket */
    guac_socket_ssl_data* data = (guac_socket_ssl_data*) socket->data;
    int retval;

    retval = SSL_read(data->ssl, buf, count);

    /* Record errors in guac_error */
    if (retval <= 0) {
        guac_error = GUAC_STATUS_SEE_ERRNO;
        guac_error_message = "Error reading data from secure socket";
    }

    return retval;

}

static ssize_t __guac_socket_ssl_write_handler(guac_socket* socket,
        const void* buf, size_t count) {

    /* Write data to socket */
    guac_socket_ssl_data* data = (guac_socket_ssl_data*) socket->data;
    int retval;

    retval = SSL_write(data->ssl, buf, count);

    /* Record errors in guac_error */
    if (retval <= 0) {
        guac_error = GUAC_STATUS_SEE_ERRNO;
        guac_error_message = "Error writing data to secure socket";
    }

    return retval;

}

static int __guac_socket_ssl_select_handler(guac_socket* socket, int usec_timeout) {

    guac_socket_ssl_data* data = (guac_socket_ssl_data*) socket->data;

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
        guac_error_message = "Error while waiting for data on secure socket";
    }

    if (retval == 0) {
        guac_error = GUAC_STATUS_INPUT_TIMEOUT;
        guac_error_message = "Timeout while waiting for data on secure socket";
    }

    return retval;

}

static int __guac_socket_ssl_free_handler(guac_socket* socket) {

    /* Shutdown SSL */
    guac_socket_ssl_data* data = (guac_socket_ssl_data*) socket->data;
    SSL_shutdown(data->ssl);

    free(data);
    return 0;
}

guac_socket* guac_socket_open_secure(SSL_CTX* context, int fd) {

    /* Allocate socket and associated data */
    guac_socket* socket = guac_socket_alloc();
    guac_socket_ssl_data* data = malloc(sizeof(guac_socket_ssl_data));

    /* Init SSL */
    data->context = context;
    data->ssl = SSL_new(context);
    SSL_set_fd(data->ssl, fd);

    /* Accept SSL connection, handle errors */
    if (SSL_accept(data->ssl) <= 0) {

        guac_error = GUAC_STATUS_BAD_STATE;
        guac_error_message = "SSL accept failed";

        free(data);
        guac_socket_free(socket);
        return NULL;
    }

    /* Store file descriptor as socket data */
    data->fd = fd;
    socket->data = data;

    /* Set read/write handlers */
    socket->read_handler   = __guac_socket_ssl_read_handler;
    socket->write_handler  = __guac_socket_ssl_write_handler;
    socket->select_handler = __guac_socket_ssl_select_handler;
    socket->free_handler   = __guac_socket_ssl_free_handler;

    return socket;

}

