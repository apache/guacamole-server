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


#ifndef _GUAC_PARSER_TYPES_H
#define _GUAC_PARSER_TYPES_H

/**
 * Type definitions related to parsing the Guacamole protocol.
 *
 * @file parser-types.h
 */

/**
 * All possible states of the instruction parser.
 */
typedef enum guac_parse_state {

    /**
     * The parser is currently waiting for data to complete the length prefix
     * of the current element of the instruction.
     */
    GUAC_PARSE_LENGTH,

    /**
     * The parser has finished reading the length prefix and is currently
     * waiting for data to complete the content of the instruction.
     */
    GUAC_PARSE_CONTENT,

    /**
     * The instruction has been fully parsed.
     */
    GUAC_PARSE_COMPLETE,

    /**
     * The instruction cannot be parsed because of a protocol error.
     */
    GUAC_PARSE_ERROR

} guac_parse_state;

/**
 * A Guacamole protocol parser, which reads individual instructions, filling
 * its own internal structure with the most recently read instruction data.
 */
typedef struct guac_parser guac_parser;

#endif

