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

#ifndef GUACD_PROC_H
#define GUACD_PROC_H

#include "config.h"

#include <guacamole/client.h>
#include <guacamole/parser.h>

#include <unistd.h>

/**
 * Process information of the internal remote desktop client.
 */
typedef struct guacd_proc {

    /**
     * The process ID of the client. This will only be available to the
     * parent process. The child process will see this as 0.
     */
    pid_t pid;

    /**
     * The file descriptor of the UNIX domain socket to use for sending and
     * receiving file descriptors of new users. This parent will see this
     * as the file descriptor for communicating with the child and vice
     * versa.
     */
    int fd_socket;

    /**
     * The actual client instance. This will be visible to both child and
     * parent process, but only the child will have a full guac_client
     * instance, containing handlers from the plugin, etc.
     *
     * The parent process will receive a skeleton guac_client, containing only
     * a proper connection_id and logging handlers. The actual
     * protocol-specific handling will be absent.
     */
    guac_client* client;

} guacd_proc;

/**
 * Creates a new process for handling the given protocol, returning the process
 * created. The created process runs in the background relative to the calling
 * process. Within the child process, this function does not return - the
 * entire child process simply terminates instead.
 */
guacd_proc* guacd_create_proc(guac_parser* parser, const char* protocol);

/**
 * Signals the given process to stop accepting new users and clean up. This
 * will eventually cause the child process to exit.
 */
void guacd_proc_stop(guacd_proc* proc);

#endif

