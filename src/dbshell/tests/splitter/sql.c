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

#include "dbshell/splitter.h"

#include <CUnit/CUnit.h>
#include <guacamole/mem.h>
#include <stdlib.h>
#include <string.h>

/**
 * Asserts that the next statement produced by the given splitter equals
 * the given expected text, freeing the produced statement.
 *
 * @param splitter
 *     The splitter to retrieve a statement from.
 *
 * @param expected
 *     The expected statement text.
 */
static void assert_statement(guac_dbshell_splitter* splitter,
        const char* expected) {

    char* statement = guac_dbshell_splitter_next_statement(splitter);
    CU_ASSERT_PTR_NOT_NULL_FATAL(statement);
    CU_ASSERT_STRING_EQUAL(statement, expected);
    guac_mem_free(statement);

}

/**
 * Verifies basic semicolon-terminated splitting, including multiple
 * statements within a single line.
 */
void test_splitter_sql__semicolons(void) {

    guac_dbshell_splitter* splitter =
        guac_dbshell_splitter_alloc(GUAC_DBSHELL_DIALECT_MYSQL);

    /* Incomplete input produces no statement */
    guac_dbshell_splitter_feed(splitter, "SELECT 1");
    CU_ASSERT_PTR_NULL(guac_dbshell_splitter_next_statement(splitter));
    CU_ASSERT_TRUE(guac_dbshell_splitter_pending(splitter));

    /* Completing the statement produces it, trimmed */
    guac_dbshell_splitter_feed(splitter, "  ;");
    assert_statement(splitter, "SELECT 1");
    CU_ASSERT_FALSE(guac_dbshell_splitter_pending(splitter));

    /* Multiple statements on one line are produced in order */
    guac_dbshell_splitter_feed(splitter, "SELECT 2; SELECT 3;");
    assert_statement(splitter, "SELECT 2");
    assert_statement(splitter, "SELECT 3");
    CU_ASSERT_PTR_NULL(guac_dbshell_splitter_next_statement(splitter));

    /* Empty statements are skipped */
    guac_dbshell_splitter_feed(splitter, ";;  ;");
    CU_ASSERT_PTR_NULL(guac_dbshell_splitter_next_statement(splitter));

    guac_dbshell_splitter_free(splitter);

}

/**
 * Verifies that semicolons within string literals, quoted identifiers,
 * and comments do not terminate statements (MySQL dialect).
 */
void test_splitter_sql__mysql_quoting(void) {

    guac_dbshell_splitter* splitter =
        guac_dbshell_splitter_alloc(GUAC_DBSHELL_DIALECT_MYSQL);

    /* Semicolons inside quotes are content */
    guac_dbshell_splitter_feed(splitter,
            "SELECT 'a;b', \"c;d\", `e;f`;");
    assert_statement(splitter, "SELECT 'a;b', \"c;d\", `e;f`");

    /* Backslash-escaped quote does not close the literal */
    guac_dbshell_splitter_feed(splitter, "SELECT 'it\\';s';");
    assert_statement(splitter, "SELECT 'it\\';s'");

    /* Doubled quote does not close the literal */
    guac_dbshell_splitter_feed(splitter, "SELECT 'it''s; fine';");
    assert_statement(splitter, "SELECT 'it''s; fine'");

    /* Comments hide semicolons */
    guac_dbshell_splitter_feed(splitter, "SELECT 1 -- comment;");
    CU_ASSERT_PTR_NULL(guac_dbshell_splitter_next_statement(splitter));
    guac_dbshell_splitter_feed(splitter, "# other; comment");
    CU_ASSERT_PTR_NULL(guac_dbshell_splitter_next_statement(splitter));
    guac_dbshell_splitter_feed(splitter, "/* block; comment */;");

    char* statement = guac_dbshell_splitter_next_statement(splitter);
    CU_ASSERT_PTR_NOT_NULL_FATAL(statement);
    guac_mem_free(statement);

    /* MySQL "--" requires trailing whitespace: "--x" is not a comment */
    guac_dbshell_splitter_reset(splitter);
    guac_dbshell_splitter_feed(splitter, "SELECT 1--1;");
    assert_statement(splitter, "SELECT 1--1");

    guac_dbshell_splitter_free(splitter);

}

/**
 * Verifies PostgreSQL dollar-quoting and nested block comments.
 */
void test_splitter_sql__pgsql(void) {

    guac_dbshell_splitter* splitter =
        guac_dbshell_splitter_alloc(GUAC_DBSHELL_DIALECT_PGSQL);

    /* Semicolons within dollar-quoted strings are content */
    guac_dbshell_splitter_feed(splitter,
            "CREATE FUNCTION f() RETURNS void AS $$");
    guac_dbshell_splitter_feed(splitter, "BEGIN; SELECT 1; END;");
    CU_ASSERT_PTR_NULL(guac_dbshell_splitter_next_statement(splitter));
    guac_dbshell_splitter_feed(splitter, "$$ LANGUAGE plpgsql;");

    char* statement = guac_dbshell_splitter_next_statement(splitter);
    CU_ASSERT_PTR_NOT_NULL_FATAL(statement);
    CU_ASSERT_PTR_NOT_NULL(strstr(statement, "BEGIN; SELECT 1; END;"));
    guac_mem_free(statement);

    /* Tagged dollar quotes must match exactly */
    guac_dbshell_splitter_feed(splitter,
            "SELECT $tag$ ; $notit$ ; $tag$;");
    assert_statement(splitter, "SELECT $tag$ ; $notit$ ; $tag$");

    /* Positional parameters are not dollar quotes */
    guac_dbshell_splitter_feed(splitter, "SELECT $1;");
    assert_statement(splitter, "SELECT $1");

    /* Nested block comments */
    guac_dbshell_splitter_feed(splitter,
            "SELECT 1 /* outer /* inner; */ still; */;");
    assert_statement(splitter, "SELECT 1 /* outer /* inner; */ still; */");

    /* Backslash is NOT an escape in PostgreSQL strings */
    guac_dbshell_splitter_feed(splitter, "SELECT 'path\\';");
    assert_statement(splitter, "SELECT 'path\\'");

    guac_dbshell_splitter_free(splitter);

}

/**
 * Verifies Transact-SQL GO batch separators and bracket quoting.
 */
void test_splitter_sql__tsql(void) {

    guac_dbshell_splitter* splitter =
        guac_dbshell_splitter_alloc(GUAC_DBSHELL_DIALECT_TSQL);

    /* Semicolons terminate as usual */
    guac_dbshell_splitter_feed(splitter, "SELECT 1;");
    assert_statement(splitter, "SELECT 1");

    /* A line consisting solely of GO terminates the batch */
    guac_dbshell_splitter_feed(splitter, "SELECT 2");
    CU_ASSERT_PTR_NULL(guac_dbshell_splitter_next_statement(splitter));
    guac_dbshell_splitter_feed(splitter, "GO");
    assert_statement(splitter, "SELECT 2");

    /* GO is case-insensitive and tolerates surrounding whitespace */
    guac_dbshell_splitter_feed(splitter, "SELECT 3");
    guac_dbshell_splitter_feed(splitter, "  go  ");
    assert_statement(splitter, "SELECT 3");

    /* GO within a line is not a separator */
    guac_dbshell_splitter_feed(splitter, "SELECT category GO FROM t;");
    assert_statement(splitter, "SELECT category GO FROM t");

    /* Semicolons within bracket identifiers are content */
    guac_dbshell_splitter_feed(splitter, "SELECT [a;b] FROM t;");
    assert_statement(splitter, "SELECT [a;b] FROM t");

    /* Doubled closing bracket remains within the identifier */
    guac_dbshell_splitter_feed(splitter, "SELECT [a]];b] FROM t;");
    assert_statement(splitter, "SELECT [a]];b] FROM t");

    guac_dbshell_splitter_free(splitter);

}

/**
 * Verifies Oracle PL/SQL block handling: semicolons terminate plain SQL
 * but not PL/SQL blocks, which end only with a slash line.
 */
void test_splitter_sql__oracle(void) {

    guac_dbshell_splitter* splitter =
        guac_dbshell_splitter_alloc(GUAC_DBSHELL_DIALECT_ORACLE);

    /* Plain SQL is terminated by semicolons */
    guac_dbshell_splitter_feed(splitter, "SELECT 1 FROM dual;");
    assert_statement(splitter, "SELECT 1 FROM dual");

    /* Anonymous blocks retain internal semicolons until the slash line */
    guac_dbshell_splitter_feed(splitter, "BEGIN");
    guac_dbshell_splitter_feed(splitter, "  NULL;");
    guac_dbshell_splitter_feed(splitter, "END;");
    CU_ASSERT_PTR_NULL(guac_dbshell_splitter_next_statement(splitter));
    guac_dbshell_splitter_feed(splitter, "/");

    char* statement = guac_dbshell_splitter_next_statement(splitter);
    CU_ASSERT_PTR_NOT_NULL_FATAL(statement);
    CU_ASSERT_PTR_NOT_NULL(strstr(statement, "NULL;"));
    CU_ASSERT_PTR_NOT_NULL(strstr(statement, "END;"));
    guac_mem_free(statement);

    /* CREATE OR REPLACE of procedural objects behaves as a block */
    guac_dbshell_splitter_feed(splitter,
            "CREATE OR REPLACE PROCEDURE p AS");
    guac_dbshell_splitter_feed(splitter, "BEGIN NULL; END;");
    CU_ASSERT_PTR_NULL(guac_dbshell_splitter_next_statement(splitter));
    guac_dbshell_splitter_feed(splitter, "/");
    statement = guac_dbshell_splitter_next_statement(splitter);
    CU_ASSERT_PTR_NOT_NULL_FATAL(statement);
    guac_mem_free(statement);

    /* A leading comment does not hide the block keyword */
    guac_dbshell_splitter_feed(splitter, "/* setup */ DECLARE x NUMBER;");
    guac_dbshell_splitter_feed(splitter, "BEGIN x := 1; END;");
    CU_ASSERT_PTR_NULL(guac_dbshell_splitter_next_statement(splitter));
    guac_dbshell_splitter_feed(splitter, "/");
    statement = guac_dbshell_splitter_next_statement(splitter);
    CU_ASSERT_PTR_NOT_NULL_FATAL(statement);
    guac_mem_free(statement);

    /* CREATE TABLE is not procedural and ends at the semicolon */
    guac_dbshell_splitter_feed(splitter, "CREATE TABLE t (a NUMBER);");
    assert_statement(splitter, "CREATE TABLE t (a NUMBER)");

    guac_dbshell_splitter_free(splitter);

}

/**
 * Verifies that oversized accumulated input is discarded and reported.
 */
void test_splitter_sql__overflow(void) {

    guac_dbshell_splitter* splitter =
        guac_dbshell_splitter_alloc(GUAC_DBSHELL_DIALECT_MYSQL);

    /* Build a line of 64KB, fed 17 times: exceeds the 1MB limit */
    char* big = guac_mem_alloc(65537);
    memset(big, 'x', 65536);
    big[0] = '\'';
    big[65536] = '\0';

    CU_ASSERT_FALSE(guac_dbshell_splitter_overflowed(splitter));

    for (int i = 0; i < 17; i++)
        guac_dbshell_splitter_feed(splitter, big);

    CU_ASSERT_TRUE(guac_dbshell_splitter_overflowed(splitter));

    /* The overflow indication clears once read */
    CU_ASSERT_FALSE(guac_dbshell_splitter_overflowed(splitter));

    guac_mem_free(big);
    guac_dbshell_splitter_free(splitter);

}
