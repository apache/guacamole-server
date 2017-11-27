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

    /* No keys are initially tracked */
    state->active_keys = 0;

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

/**
 * Adds the given key state to the array of tracked keys. If the key is already
 * being tracked, its corresponding entry within the array of tracked keys is
 * updated, and the number of tracked keys remains the same. If the key is not
 * already being tracked, it is added to the end of the array of tracked keys
 * providing there is space available, and the number of tracked keys is
 * updated. Failures to add keys will be automatically logged.
 *
 * @param state
 *     The Guacamole input log interpreter state being updated.
 *
 * @param keysym
 *     The X11 keysym of the key being pressed or released.
 *
 * @param pressed
 *     true if the key is being pressed, false if the key is being released.
 *
 * @return
 *     Zero if the key state was successfully added, non-zero otherwise.
 */
static int guaclog_state_add_key(guaclog_state* state, int keysym, bool pressed) {

    int i;

    /* Update existing key, if already tracked */
    for (i = 0; i < state->active_keys; i++) {
        guaclog_key_state* key = &state->key_states[i];
        if (key->keysym == keysym) {
            key->pressed = pressed;
            return 0;
        }
    }

    /* If not already tracked, we need space to add it */
    if (state->active_keys == GUACLOG_MAX_KEYS) {
        guaclog_log(GUAC_LOG_WARNING, "Unable to log key 0x%X: Too many "
                "active keys.", keysym);
        return 1;
    }

    /* Add key to state */
    guaclog_key_state* key = &state->key_states[state->active_keys++];
    key->keysym = keysym;
    key->pressed = pressed;
    return 0;

}

/**
 * Removes released keys from the end of the array of tracked keys, such that
 * the last key in the array is a pressed key. This function should be invoked
 * after changes have been made to the interpreter state, to ensure that the
 * array of tracked keys does not grow longer than necessary.
 *
 * @param state
 *     The Guacamole input log interpreter state to trim.
 */
static void guaclog_state_trim_keys(guaclog_state* state) {

    int i;

    /* Reset active_keys to contain only up to the last pressed key */
    for (i = state->active_keys - 1; i >= 0; i--) {
        guaclog_key_state* key = &state->key_states[i];
        if (key->pressed) {
            state->active_keys = i + 1;
            return;
        }
    }

    /* No keys are active */
    state->active_keys = 0;

}

int guaclog_state_update_key(guaclog_state* state, int keysym, bool pressed) {

    int i;

    /* Update tracked keysysm state */
    guaclog_state_add_key(state, keysym, pressed);
    guaclog_state_trim_keys(state);

    /* Output new log entries only when keys are pressed */
    if (pressed) {   

        /* STUB: Output raw hex log entry */
        for (i = 0; i < state->active_keys; i++) {

            if (i != 0)
                fprintf(state->output, " ");

            guaclog_key_state* key = &state->key_states[i];
            fprintf(state->output, "0x%X:%s", key->keysym,
                    key->pressed ? "*" : " ");

        }

        /* Terminate log entry with newline */
        fprintf(state->output, "\n");

    }

    return 0;

}

