/*
 * Copyright (C) 2016 Glyptodon, Inc.
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
#include "log.h"

#include <guacamole/client.h>
#include <guacamole/error.h>
#include <guacamole/parser.h>
#include <guacamole/socket.h>

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>

/**
 * Reads and handles all Guacamole instructions from the given guac_socket
 * until end-of-stream is reached.
 *
 * @param path
 *     The name of the file being parsed (for logging purposes). This file
 *     must already be open and available through the given socket.
 *
 * @param socket
 *     The guac_socket through which instructions should be read.
 *
 * @return
 *     Zero on success, non-zero if parsing of Guacamole protocol data through
 *     the given socket fails.
 */
static int guacenc_read_instructions(const char* path, guac_socket* socket) {

    /* Obtain Guacamole protocol parser */
    guac_parser* parser = guac_parser_alloc();
    if (parser == NULL)
        return 1;

    /* Continuously read instructions */
    while (!guac_parser_read(parser, socket, -1)) {

        /* STUB: Handle instruction */
        guacenc_log(GUAC_LOG_DEBUG, "STUB: \"%s\" ...", parser->opcode);

    }

    /* Fail on read/parse error */
    if (guac_error != GUAC_STATUS_CLOSED) {
        guacenc_log(GUAC_LOG_ERROR, "%s: %s",
                path, guac_status_string(guac_error));
        guac_parser_free(parser);
        return 1;
    }

    /* Parse complete */
    guac_parser_free(parser);
    return 0;

}

int guacenc_encode(const char* path) {

    /* Open input file */
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        guacenc_log(GUAC_LOG_ERROR, "%s: %s", path, strerror(errno));
        return 1;
    }

    /* Obtain guac_socket wrapping file descriptor */
    guac_socket* socket = guac_socket_open(fd);
    if (socket == NULL) {
        guacenc_log(GUAC_LOG_ERROR, "%s: %s", path,
                guac_status_string(guac_error));
        close(fd);
        return 1;
    }

    guacenc_log(GUAC_LOG_INFO, "Encoding \"%s\" ...", path);

    /* Attempt to read all instructions in the file */
    if (guacenc_read_instructions(path, socket)) {
        guac_socket_free(socket);
        return 1;
    }

    /* Parsing/encoding was successful */
    guac_socket_free(socket);
    return 0;

}

