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


#ifndef _GUAC_TEST_PROTOCOL_SUITE_H
#define _GUAC_TEST_PROTOCOL_SUITE_H

#include "config.h"

/* Unicode (UTF-8) strings */

#define UTF8_1 "\xe7\x8a\xac"            /* One character    */
#define UTF8_2 UTF8_1 "\xf0\x90\xac\x80" /* Two characters   */
#define UTF8_3 UTF8_2 "z"                /* Three characters */
#define UTF8_4 UTF8_3 "\xc3\xa1"         /* Four characters  */
#define UTF8_8 UTF8_4 UTF8_4             /* Eight characters */

int register_protocol_suite();

void test_base64_decode();
void test_instruction_parse();
void test_instruction_read();
void test_instruction_write();
void test_nest_write();

#endif

