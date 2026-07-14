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

#include "dbshell/line-editor.h"

#include <CUnit/CUnit.h>
#include <string.h>

/**
 * Feeds each byte of the given null-terminated string to the given parser,
 * returning the last non-NONE key produced, or GUAC_DBSHELL_KEY_NONE if no
 * key was produced.
 *
 * @param parser
 *     The parser to feed.
 *
 * @param bytes
 *     The null-terminated bytes to feed.
 *
 * @return
 *     The last key produced other than GUAC_DBSHELL_KEY_NONE, or
 *     GUAC_DBSHELL_KEY_NONE if no such key was produced.
 */
static guac_dbshell_key feed_all(guac_dbshell_parser* parser,
        const char* bytes) {

    guac_dbshell_key last = GUAC_DBSHELL_KEY_NONE;

    for (const char* c = bytes; *c != '\0'; c++) {
        guac_dbshell_key key = guac_dbshell_parser_feed(parser, *c);
        if (key != GUAC_DBSHELL_KEY_NONE)
            last = key;
    }

    return last;

}

/**
 * Verifies that printable ASCII characters are produced as
 * GUAC_DBSHELL_KEY_CHAR with the correct content.
 */
void test_parser__printable(void) {

    guac_dbshell_parser parser;
    guac_dbshell_parser_init(&parser);

    CU_ASSERT_EQUAL(guac_dbshell_parser_feed(&parser, 'a'),
            GUAC_DBSHELL_KEY_CHAR);
    CU_ASSERT_EQUAL(parser.char_length, 1);
    CU_ASSERT_EQUAL(parser.char_buffer[0], 'a');

}

/**
 * Verifies that multi-byte UTF-8 characters are accumulated and produced
 * as a single GUAC_DBSHELL_KEY_CHAR.
 */
void test_parser__utf8(void) {

    guac_dbshell_parser parser;
    guac_dbshell_parser_init(&parser);

    /* U+00E9 (2 bytes) */
    CU_ASSERT_EQUAL(guac_dbshell_parser_feed(&parser, '\xC3'),
            GUAC_DBSHELL_KEY_NONE);
    CU_ASSERT_EQUAL(guac_dbshell_parser_feed(&parser, '\xA9'),
            GUAC_DBSHELL_KEY_CHAR);
    CU_ASSERT_EQUAL(parser.char_length, 2);
    CU_ASSERT_NSTRING_EQUAL(parser.char_buffer, "\xC3\xA9", 2);

    /* U+4E2D (3 bytes) */
    CU_ASSERT_EQUAL(guac_dbshell_parser_feed(&parser, '\xE4'),
            GUAC_DBSHELL_KEY_NONE);
    CU_ASSERT_EQUAL(guac_dbshell_parser_feed(&parser, '\xB8'),
            GUAC_DBSHELL_KEY_NONE);
    CU_ASSERT_EQUAL(guac_dbshell_parser_feed(&parser, '\xAD'),
            GUAC_DBSHELL_KEY_CHAR);
    CU_ASSERT_EQUAL(parser.char_length, 3);

    /* Truncated character followed by a printable character: the invalid
     * byte is ignored, the printable character survives */
    CU_ASSERT_EQUAL(guac_dbshell_parser_feed(&parser, '\xE4'),
            GUAC_DBSHELL_KEY_NONE);
    CU_ASSERT_EQUAL(guac_dbshell_parser_feed(&parser, 'x'),
            GUAC_DBSHELL_KEY_IGNORED);
    CU_ASSERT_EQUAL(guac_dbshell_parser_feed(&parser, 'x'),
            GUAC_DBSHELL_KEY_CHAR);

}

/**
 * Verifies that CSI cursor sequences produce the corresponding editing
 * keys.
 */
void test_parser__csi(void) {

    guac_dbshell_parser parser;
    guac_dbshell_parser_init(&parser);

    CU_ASSERT_EQUAL(feed_all(&parser, "\x1B[A"), GUAC_DBSHELL_KEY_UP);
    CU_ASSERT_EQUAL(feed_all(&parser, "\x1B[B"), GUAC_DBSHELL_KEY_DOWN);
    CU_ASSERT_EQUAL(feed_all(&parser, "\x1B[C"), GUAC_DBSHELL_KEY_RIGHT);
    CU_ASSERT_EQUAL(feed_all(&parser, "\x1B[D"), GUAC_DBSHELL_KEY_LEFT);
    CU_ASSERT_EQUAL(feed_all(&parser, "\x1B[H"), GUAC_DBSHELL_KEY_HOME);
    CU_ASSERT_EQUAL(feed_all(&parser, "\x1B[F"), GUAC_DBSHELL_KEY_END);

    /* Editing keypad variants */
    CU_ASSERT_EQUAL(feed_all(&parser, "\x1B[1~"), GUAC_DBSHELL_KEY_HOME);
    CU_ASSERT_EQUAL(feed_all(&parser, "\x1B[4~"), GUAC_DBSHELL_KEY_END);
    CU_ASSERT_EQUAL(feed_all(&parser, "\x1B[7~"), GUAC_DBSHELL_KEY_HOME);
    CU_ASSERT_EQUAL(feed_all(&parser, "\x1B[8~"), GUAC_DBSHELL_KEY_END);
    CU_ASSERT_EQUAL(feed_all(&parser, "\x1B[3~"), GUAC_DBSHELL_KEY_DELETE);

    /* Unknown sequences are consumed and ignored */
    CU_ASSERT_EQUAL(feed_all(&parser, "\x1B[5~"),
            GUAC_DBSHELL_KEY_IGNORED);
    CU_ASSERT_EQUAL(feed_all(&parser, "\x1B[38;5;100m"),
            GUAC_DBSHELL_KEY_IGNORED);

    /* Parser returns to ground state afterwards */
    CU_ASSERT_EQUAL(guac_dbshell_parser_feed(&parser, 'z'),
            GUAC_DBSHELL_KEY_CHAR);

}

/**
 * Verifies that SS3 sequences produce the corresponding editing keys.
 */
void test_parser__ss3(void) {

    guac_dbshell_parser parser;
    guac_dbshell_parser_init(&parser);

    CU_ASSERT_EQUAL(feed_all(&parser, "\x1BOA"), GUAC_DBSHELL_KEY_UP);
    CU_ASSERT_EQUAL(feed_all(&parser, "\x1BOB"), GUAC_DBSHELL_KEY_DOWN);
    CU_ASSERT_EQUAL(feed_all(&parser, "\x1BOH"), GUAC_DBSHELL_KEY_HOME);
    CU_ASSERT_EQUAL(feed_all(&parser, "\x1BOF"), GUAC_DBSHELL_KEY_END);

}

/**
 * Verifies that CR produces ENTER and that the LF of a CRLF pair is
 * absorbed, including when the pair spans separate feeds.
 */
void test_parser__crlf(void) {

    guac_dbshell_parser parser;
    guac_dbshell_parser_init(&parser);

    /* CR alone is ENTER */
    CU_ASSERT_EQUAL(guac_dbshell_parser_feed(&parser, '\r'),
            GUAC_DBSHELL_KEY_ENTER);

    /* LF following CR is absorbed */
    CU_ASSERT_EQUAL(guac_dbshell_parser_feed(&parser, '\n'),
            GUAC_DBSHELL_KEY_NONE);

    /* LF alone is ENTER */
    CU_ASSERT_EQUAL(guac_dbshell_parser_feed(&parser, '\n'),
            GUAC_DBSHELL_KEY_ENTER);

    /* Non-CR input clears the CR flag */
    CU_ASSERT_EQUAL(guac_dbshell_parser_feed(&parser, '\r'),
            GUAC_DBSHELL_KEY_ENTER);
    CU_ASSERT_EQUAL(guac_dbshell_parser_feed(&parser, 'a'),
            GUAC_DBSHELL_KEY_CHAR);
    CU_ASSERT_EQUAL(guac_dbshell_parser_feed(&parser, '\n'),
            GUAC_DBSHELL_KEY_ENTER);

}

/**
 * Verifies that control characters map to the expected editing keys.
 */
void test_parser__controls(void) {

    guac_dbshell_parser parser;
    guac_dbshell_parser_init(&parser);

    CU_ASSERT_EQUAL(guac_dbshell_parser_feed(&parser, '\x7F'),
            GUAC_DBSHELL_KEY_BACKSPACE);
    CU_ASSERT_EQUAL(guac_dbshell_parser_feed(&parser, '\x08'),
            GUAC_DBSHELL_KEY_BACKSPACE);
    CU_ASSERT_EQUAL(guac_dbshell_parser_feed(&parser, '\x01'),
            GUAC_DBSHELL_KEY_HOME);
    CU_ASSERT_EQUAL(guac_dbshell_parser_feed(&parser, '\x05'),
            GUAC_DBSHELL_KEY_END);
    CU_ASSERT_EQUAL(guac_dbshell_parser_feed(&parser, '\x03'),
            GUAC_DBSHELL_KEY_INTERRUPT);
    CU_ASSERT_EQUAL(guac_dbshell_parser_feed(&parser, '\x04'),
            GUAC_DBSHELL_KEY_EOF);
    CU_ASSERT_EQUAL(guac_dbshell_parser_feed(&parser, '\x0C'),
            GUAC_DBSHELL_KEY_CLEAR);
    CU_ASSERT_EQUAL(guac_dbshell_parser_feed(&parser, '\x15'),
            GUAC_DBSHELL_KEY_KILL_LINE);
    CU_ASSERT_EQUAL(guac_dbshell_parser_feed(&parser, '\x0B'),
            GUAC_DBSHELL_KEY_KILL_TO_END);
    CU_ASSERT_EQUAL(guac_dbshell_parser_feed(&parser, '\x17'),
            GUAC_DBSHELL_KEY_KILL_WORD);
    CU_ASSERT_EQUAL(guac_dbshell_parser_feed(&parser, '\x09'),
            GUAC_DBSHELL_KEY_TAB);

}

/**
 * Verifies that Alt+key combinations (ESC followed by a printable
 * character) are consumed without inserting the character.
 */
void test_parser__alt_ignored(void) {

    guac_dbshell_parser parser;
    guac_dbshell_parser_init(&parser);

    CU_ASSERT_EQUAL(guac_dbshell_parser_feed(&parser, '\x1B'),
            GUAC_DBSHELL_KEY_NONE);
    CU_ASSERT_EQUAL(guac_dbshell_parser_feed(&parser, 'f'),
            GUAC_DBSHELL_KEY_IGNORED);

    /* Subsequent input is processed normally */
    CU_ASSERT_EQUAL(guac_dbshell_parser_feed(&parser, 'f'),
            GUAC_DBSHELL_KEY_CHAR);

}
