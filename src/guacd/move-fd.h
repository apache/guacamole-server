/*
 * Copyright (C) 2014 Glyptodon LLC
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

#ifndef GUACD_MOVE_FD_H
#define GUACD_MOVE_FD_H

#include "config.h"

/**
 * Sends the given file descriptor along the given socket, allowing the
 * receiving process to use that file descriptor normally. Returns non-zero on
 * success, zero on error, just as a normal call to sendmsg() would. If an
 * error does occur, errno will be set appropriately.
 *
 * @param sock
 *     The file descriptor of an open UNIX domain socket along which the file
 *     descriptor specified by fd should be sent.
 *
 * @param fd
 *     The file descriptor to send along the given UNIX domain socket.
 *
 * @return
 *     Non-zero if the send operation succeeded, zero on error.
 */
int guacd_send_fd(int sock, int fd);

/**
 * Waits for a file descriptor on the given socket, returning the received file
 * descriptor. The file descriptor must have been sent via guacd_send_fd. If an
 * error occurs, -1 is returned, and errno will be set appropriately.
 *
 * @param sock
 *     The file descriptor of an open UNIX domain socket along which the file
 *     descriptor will be sent (by guacd_send_fd()).
 *
 * @return
 *     The received file descriptor, or -1 if an error occurs preventing
 *     receipt of the file descriptor.
 */
int guacd_recv_fd(int sock);

#endif

