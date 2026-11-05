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
#include "instructions.h"
#include "interpret.h"
#include "log.h"
#include "state.h"

#include <guacamole/error.h>
#include <guacamole/parser.h>
#include <guacamole/socket.h>

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/**
 * Reads and handles all Guacamole instructions from the given guac_socket
 * until end-of-stream is reached.
 *
 * @param state
 *     The current state of the guacclip interpreter.
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
static int guacclip_read_instructions(guacclip_state* state,
        const char* path, guac_socket* socket) {

    /* Obtain Guacamole protocol parser */
    guac_parser* parser = guac_parser_alloc();
    if (parser == NULL)
        return 1;

    /* Continuously read and handle all instructions */
    while (!guac_parser_read(parser, socket, -1)) {
        guacclip_handle_instruction(state, parser->opcode,
                parser->argc, parser->argv);
    }

    /* Fail on read/parse error */
    if (guac_error != GUAC_STATUS_CLOSED) {
        guacclip_log(GUAC_LOG_ERROR, "%s: %s",
                path, guac_status_string(guac_error));
        guac_parser_free(parser);
        return 1;
    }

    /* Parse complete */
    guac_parser_free(parser);
    return 0;

}

int guacclip_interpret(const char* path, const guacclip_options* options,
        bool force) {

    /* Open input file */
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        guacclip_log(GUAC_LOG_ERROR, "%s: %s", path, strerror(errno));
        return 1;
    }

    /* Lock entire input file for reading by the current process */
    struct flock file_lock = {
        .l_type   = F_RDLCK,
        .l_whence = SEEK_SET,
        .l_start  = 0,
        .l_len    = 0,
        .l_pid    = getpid()
    };

    /* Abort if file cannot be locked for reading */
    if (!force && fcntl(fd, F_SETLK, &file_lock) == -1) {

        /* Warn if lock cannot be acquired */
        if (errno == EACCES || errno == EAGAIN)
            guacclip_log(GUAC_LOG_WARNING, "Refusing to interpret recording of "
                    "in-progress session \"%s\" (specify the -f option to "
                    "override this behavior).", path);

        /* Log an error if locking fails in an unexpected way */
        else
            guacclip_log(GUAC_LOG_ERROR, "Cannot lock \"%s\" for reading: %s",
                    path, strerror(errno));

        close(fd);
        return 1;
    }

    /* Allocate interpreter state (creates output directory) */
    guacclip_state* state = guacclip_state_alloc(path, options);
    if (state == NULL) {
        close(fd);
        return 1;
    }

    /* Obtain guac_socket wrapping file descriptor */
    guac_socket* socket = guac_socket_open(fd);
    if (socket == NULL) {
        guacclip_log(GUAC_LOG_ERROR, "%s: %s", path,
                guac_status_string(guac_error));
        close(fd);
        guacclip_state_free(state);
        return 1;
    }

    guacclip_log(GUAC_LOG_INFO, "Extracting clipboard artifacts from \"%s\" "
            "into \"%s\" ...", path, options->outdir);

    /* Attempt to read all instructions in the file */
    if (guacclip_read_instructions(state, path, socket)) {
        guac_socket_free(socket);
        guacclip_state_free(state);
        return 1;
    }

    /* Close input and finish interpreting process (writes manifest) */
    guac_socket_free(socket);
    return guacclip_state_free(state);

}
