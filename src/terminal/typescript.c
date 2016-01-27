/*
 * Copyright (C) 2016 Glyptodon LLC
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
#include "guac_io.h"
#include "typescript.h"

#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

guac_terminal_typescript* guac_terminal_typescript_alloc(const char* path,
        const char* name, int create_path) {

    guac_terminal_typescript* typescript;
    int data_fd, timing_fd;

    /* TODO: Determing data and timing filenames prior to open() calls.
     * Be sure not to use open() itself to test for existence, as that could
     * result in tons of typescript data files being unnecessarily created.
     */

    /* Attempt to open typescript data file */
    data_fd = open("/tmp/typescript-data",
            O_CREAT | O_EXCL | O_WRONLY,
            S_IRUSR | S_IWUSR);
    if (data_fd == -1)
        return NULL;

    /* Attempt to open typescript timing file */
    timing_fd = open("/tmp/typescript-timing",
            O_CREAT | O_EXCL | O_WRONLY,
            S_IRUSR | S_IWUSR);
    if (timing_fd == -1) {
        close(data_fd);
        return NULL;
    }

    /* Init newly-created typescript */
    typescript = malloc(sizeof(guac_terminal_typescript));
    typescript->data_fd = data_fd;
    typescript->timing_fd = timing_fd;
    typescript->length = 0;

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

    /* Empty buffer into data file */
    guac_common_write(typescript->data_fd,
            typescript->buffer, typescript->length);
    typescript->length = 0;

    /* TODO: Write timestamp */

}

void guac_terminal_typescript_free(guac_terminal_typescript* typescript) {

    /* Do nothing if no typescript provided */
    if (typescript == NULL)
        return;

    /* Flush any pending data */
    guac_terminal_typescript_flush(typescript);

    /* Close file descriptors */
    close(typescript->data_fd);
    close(typescript->timing_fd);

    /* Free allocated typescript data */
    free(typescript);

}

