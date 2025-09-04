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

#include "common/io.h"
#include "terminal/typescript.h"

#include <guacamole/file.h>
#include <guacamole/mem.h>
#include <guacamole/timestamp.h>

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

guac_terminal_typescript* guac_terminal_typescript_alloc(const char* path,
        const char* name, int create_path, int allow_write_existing) {

    /* Allocate space for new typescript */
    guac_terminal_typescript* typescript =
        guac_mem_alloc(sizeof(guac_terminal_typescript));

    guac_open_how data_how = {
        .oflags = O_CREAT | O_WRONLY,
        .mode = S_IRUSR | S_IWUSR | S_IRGRP,
        .filename = typescript->data_filename,
        .filename_size = sizeof(typescript->data_filename)
    };

    if (create_path)
        data_how.flags |= GUAC_O_CREATE_PATH;

    if (!allow_write_existing)
        data_how.flags |= GUAC_O_UNIQUE_SUFFIX;

    /* Attempt to open typescript data file */
    typescript->data_fd = guac_openat(path, name, &data_how);
    if (typescript->data_fd == -1) {
        guac_mem_free(typescript);
        return NULL;
    }

    /* Append suffix to basename */
    if (snprintf(typescript->timing_filename, sizeof(typescript->timing_filename),
                "%s.%s", typescript->data_filename, GUAC_TERMINAL_TYPESCRIPT_TIMING_SUFFIX)
            >= sizeof(typescript->timing_filename)) {
        close(typescript->data_fd);
        guac_mem_free(typescript);
        return NULL;
    }

    guac_open_how timing_how = {
        .oflags = O_CREAT | O_WRONLY,
        .mode = S_IRUSR | S_IWUSR | S_IRGRP
    };

    /* Attempt to open typescript timing file */
    typescript->timing_fd = guac_openat(path, typescript->timing_filename, &timing_how);
    if (typescript->timing_fd == -1) {
        close(typescript->data_fd);
        guac_mem_free(typescript);
        return NULL;
    }

    /* Typescript starts out flushed */
    typescript->length = 0;
    typescript->last_flush = guac_timestamp_current();

    /* Write header */
    guac_common_write(typescript->data_fd, GUAC_TERMINAL_TYPESCRIPT_HEADER,
            sizeof(GUAC_TERMINAL_TYPESCRIPT_HEADER) - 1);

    return typescript;

}

void guac_terminal_typescript_write(guac_terminal_typescript* typescript,
        char c) {

    /* Flush buffer if no space is available */
    if (typescript->length == sizeof(typescript->buffer))
        guac_terminal_typescript_flush(typescript);

    /* Append single byte to buffer */
    typescript->buffer[typescript->length++] = c;

}

void guac_terminal_typescript_flush(guac_terminal_typescript* typescript) {

    /* Do nothing if nothing to flush */
    if (typescript->length == 0)
        return;

    /* Get timestamps of previous and current flush */
    guac_timestamp this_flush = guac_timestamp_current();
    guac_timestamp last_flush = typescript->last_flush;

    /* Calculate time since last flush */
    int elapsed_time = this_flush - last_flush;
    if (elapsed_time > GUAC_TERMINAL_TYPESCRIPT_MAX_DELAY)
        elapsed_time = GUAC_TERMINAL_TYPESCRIPT_MAX_DELAY;

    /* Produce single line of timestamp output */
    char timestamp_buffer[32];
    int timestamp_length = snprintf(timestamp_buffer, sizeof(timestamp_buffer),
            "%0.6f %i\n", elapsed_time / 1000.0, typescript->length);

    /* Calculate actual length of timestamp line */
    if (timestamp_length > sizeof(timestamp_buffer))
        timestamp_length = sizeof(timestamp_buffer);

    /* Write timestamp to timing file */
    guac_common_write(typescript->timing_fd,
            timestamp_buffer, timestamp_length);

    /* Empty buffer into data file */
    guac_common_write(typescript->data_fd,
            typescript->buffer, typescript->length);

    /* Buffer is now flushed */
    typescript->length = 0;
    typescript->last_flush = this_flush;

}

void guac_terminal_typescript_free(guac_terminal_typescript* typescript) {

    /* Do nothing if no typescript provided */
    if (typescript == NULL)
        return;

    /* Flush any pending data */
    guac_terminal_typescript_flush(typescript);

    /* Write footer */
    guac_common_write(typescript->data_fd, GUAC_TERMINAL_TYPESCRIPT_FOOTER,
            sizeof(GUAC_TERMINAL_TYPESCRIPT_FOOTER) - 1);

    /* Close file descriptors */
    close(typescript->data_fd);
    close(typescript->timing_fd);

    /* Free allocated typescript data */
    guac_mem_free(typescript);

}

