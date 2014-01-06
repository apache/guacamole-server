/*
 * Copyright (C) 2013 Glyptodon LLC
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

#ifndef __GUAC_COMMON_IO_H
#define __GUAC_COMMON_IO_H

#include "config.h"

/**
 * Writes absolutely all bytes from within the given buffer, returning an error
 * only if the required writes fail.
 *
 * @param fd The file descriptor to write to.
 * @param buffer The buffer containing the data to write.
 * @param length The number of bytes to write.
 * @return The number of bytes written, or a value less than zero if an error
 *         occurs.
 */
int guac_common_write(int fd, void* buffer, int length);

/**
 * Reads enough bytes to fill the given buffer, returning an error only if the
 * required reads fail.
 *
 * @param fd The file descriptor to read from.
 * @param buffer The buffer to read data into.
 * @param length The number of bytes to read.
 * @return The number of bytes read, or a value less than zero if an error
 *         occurs.
 */
int guac_common_read(int fd, void* buffer, int length);

#endif

