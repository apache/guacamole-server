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

#ifndef GUACENC_PARSE_H
#define GUACENC_PARSE_H

#include "config.h"

/**
 * Parses a string into a single integer. Only positive integers are accepted.
 * The input string may be modified during parsing. A value will be stored in
 * the provided int pointer only if valid.
 *
 * @param arg
 *     The string to parse.
 *
 * @param i
 *     A pointer to the integer in which the parsed value of the given string
 *     should be stored.
 *
 * @return
 *     Zero if parsing was successful, non-zero if the provided string was
 *     invalid.
 */
int guacenc_parse_int(char* arg, int* i);

/**
 * Parses a string of the form WIDTHxHEIGHT into individual width and height
 * integers. The input string may be modified during parsing. Values will be
 * stored in the provided width and height pointers only if the given
 * dimensions are valid.
 *
 * @param arg
 *     The string to parse.
 *
 * @param width
 *     A pointer to the integer in which the parsed width component of the
 *     given string should be stored.
 *
 * @param height
 *     A pointer to the integer in which the parsed height component of the
 *     given string should be stored.
 *
 * @return
 *     Zero if parsing was successful, non-zero if the provided string was
 *     invalid.
 */
int guacenc_parse_dimensions(char* arg, int* width, int* height);

#endif


