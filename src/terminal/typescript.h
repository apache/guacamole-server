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


#ifndef GUAC_TERMINAL_TYPESCRIPT_H
#define GUAC_TERMINAL_TYPESCRIPT_H

#include "config.h"

/**
 * A NULL-terminated string of raw bytes which should be written at the
 * beginning of any typescript.
 */
#define GUAC_TERMINAL_TYPESCRIPT_HEADER "[BEGIN TYPESCRIPT]\n"

/**
 * A NULL-terminated string of raw bytes which should be written at the
 * end of any typescript.
 */
#define GUAC_TERMINAL_TYPESCRIPT_FOOTER "\n[END TYPESCRIPT]\n"

/**
 * An active typescript, consisting of a data file (raw terminal output) and
 * timing file (related timestamps and byte counts).
 */
typedef struct guac_terminal_typescript {

    /**
     * Buffer of raw terminal output which has not yet been written to the
     * data file.
     */
    char buffer[4096];

    /**
     * The number of bytes currently stored in the buffer.
     */
    int length;

    /**
     * The file descriptor of the file into which raw terminal output should be
     * written.
     */
    int data_fd;

    /**
     * The file descriptor of the file into which timing information
     * (timestamps and byte counts) related to the raw terminal output in the
     * data file should be written.
     */
    int timing_fd;

} guac_terminal_typescript;

/**
 * Creates a new pair of typescript files within the given path and using the
 * given base name, returning an abstraction which represents those files.
 * Terminal output will be written to these new files, along with timing
 * information. If the create_path flag is non-zero, the given path will be
 * created if it does not yet exist.
 *
 * @param path
 *     The full absolute path to a directory in which the typescript files
 *     should be created.
 *
 * @param name
 *     The base name to use for the typescript files created within the
 *     specified path.
 *
 * @param create_path
 *     Zero if the specified path MUST exist for typescript files to be
 *     written, or non-zero if the path should be created if it does not yet
 *     exist.
 *
 * @return
 *     A new guac_terminal_typescript representing the typescript files
 *     requested, or NULL if creation of the typescript files failed.
 */
guac_terminal_typescript* guac_terminal_typescript_alloc(const char* path,
        const char* name, int create_path);

/**
 * Writes a single byte of terminal data to the typescript, flushing and
 * writing a new timestamp if necessary.
 *
 * @param typescript
 *     The typescript that the given byte of raw terminal data should be
 *     written to.
 *
 * @param c
 *     The single byte of raw terminal data to write to the typescript.
 */
void guac_terminal_typescript_write(guac_terminal_typescript* typescript,
        char c);

/**
 * Flushes any pending data to the typescript, writing a new timestamp to the
 * timing file if any data was flushed.
 *
 * @param typescript
 *     The typescript which should be flushed.
 */
void guac_terminal_typescript_flush(guac_terminal_typescript* typescript);

/**
 * Frees all resources associated with the given typescript, flushing and
 * closing the data and timing files and freeing all related memory. If the
 * provided typescript is NULL, this function has no effect.
 *
 * @param typescript
 *     The typescript to free.
 */
void guac_terminal_typescript_free(guac_terminal_typescript* typescript);

#endif

