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
#include "log.h"
#include "state.h"

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

guaclog_state* guaclog_state_alloc(const char* path) {

    /* Open output file */
    int fd = open(path, O_CREAT | O_EXCL | O_WRONLY, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        guaclog_log(GUAC_LOG_ERROR, "Failed to open output file \"%s\": %s",
                path, strerror(errno));
        goto fail_output_fd;
    }

    /* Create stream for output file */
    FILE* output = fdopen(fd, "wb");
    if (output == NULL) {
        guaclog_log(GUAC_LOG_ERROR, "Failed to allocate stream for output "
                "file \"%s\": %s", path, strerror(errno));
        goto fail_output_file;
    }

    /* Allocate state */
    guaclog_state* state = (guaclog_state*) calloc(1, sizeof(guaclog_state));
    if (state == NULL) {
        goto fail_state;
    }

    /* Associate state with output file */
    state->output = output;

    return state;

    /* Free all allocated data in case of failure */
fail_state:
    fclose(output);

fail_output_file:
    close(fd);

fail_output_fd:
    return NULL;

}

int guaclog_state_free(guaclog_state* state) {

    /* Ignore NULL state */
    if (state == NULL)
        return 0;

    /* Close output file */
    fclose(state->output);

    free(state);
    return 0;

}

int guaclog_state_update_key(guaclog_state* state, int keysym, bool pressed) {

    /* STUB */
    fprintf(state->output, "STUB: keysym=0x%X, pressed=%s\n",
            keysym, pressed ? "true" : "false");

    return 0;

}

