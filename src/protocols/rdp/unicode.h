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

#ifndef GUAC_RDP_UNICODE_H
#define GUAC_RDP_UNICODE_H

/**
 * Convert the given number of UTF-16 characters to UTF-8 characters.
 *
 * @param utf16 Arbitrary UTF-16 data.
 * @param length The length of the UTF-16 data, in characters.
 * @param utf8 Buffer to which the converted UTF-8 data will be written.
 * @param size The maximum number of bytes available in the UTF-8 buffer.
 */
void guac_rdp_utf16_to_utf8(const unsigned char* utf16, int length,
        char* utf8, int size);

/**
 * Convert the given number of UTF-8 characters to UTF-16 characters.
 *
 * @param utf8 Arbitrary UTF-8 data.
 * @param length The length of the UTF-8 data, in characters.
 * @param utf16 Buffer to which the converted UTF-16 data will be written.
 * @param size The maximum number of bytes available in the UTF-16 buffer.
 */
void guac_rdp_utf8_to_utf16(const unsigned char* utf8, int length,
        char* utf16, int size);

#endif

