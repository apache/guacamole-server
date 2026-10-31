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

#include "channels/file.h"

#include <CUnit/CUnit.h>

#include <string.h>

/**
 * Normalizes the given virtual path via guac_spice_folder_normalize_path() and
 * asserts that normalization succeeds and yields exactly the expected result.
 *
 * @param path
 *     The input virtual path to normalize.
 *
 * @param expected
 *     The exact normalized path expected on success.
 */
static void assert_normalized(const char* path, const char* expected) {
    char out[GUAC_SPICE_FOLDER_MAX_PATH];
    int result = guac_spice_folder_normalize_path(path, out);
    CU_ASSERT_EQUAL_FATAL(result, 0);
    CU_ASSERT_STRING_EQUAL(out, expected);

    /* A normalized path must NEVER retain traversal or backslash artifacts */
    CU_ASSERT_PTR_NULL(strstr(out, ".."));
    CU_ASSERT_PTR_NULL(strchr(out, '\\'));
}

/**
 * Asserts that guac_spice_folder_normalize_path() rejects the given path
 * (returns non-zero).
 *
 * @param path
 *     The input virtual path which must be rejected.
 */
static void assert_rejected(const char* path) {
    char out[GUAC_SPICE_FOLDER_MAX_PATH];
    CU_ASSERT_NOT_EQUAL(guac_spice_folder_normalize_path(path, out), 0);
}

/**
 * Verifies that ordinary, legitimate paths normalize unchanged (or to their
 * obvious canonical form) so that the traversal hardening did not break normal
 * file-transfer behavior.
 */
void test_path__legitimate(void) {
    assert_normalized("/", "/");
    assert_normalized("/a/b/c", "/a/b/c");
    assert_normalized("/Download/report.pdf", "/Download/report.pdf");
    assert_normalized("/a/./b", "/a/b");           /* "." components dropped */
    assert_normalized("/a/b/", "/a/b");            /* trailing slash dropped */
    assert_normalized("/a//b", "/a/b");            /* repeated separators */
    assert_normalized("/a/b/../c", "/a/c");        /* in-range ".." collapses */
}

/**
 * Verifies that ".." can never escape the virtual root: root-relative traversal
 * is clamped at "/" rather than producing a path that climbs above it. This is
 * the core of the F1 path-traversal fix (forward-slash form).
 */
void test_path__forward_traversal_clamped(void) {
    assert_normalized("/../../etc/passwd", "/etc/passwd");
    assert_normalized("/a/../../b", "/b");
    assert_normalized("/../..", "/");
}

/**
 * Verifies that backslashes are treated as path separators BEFORE ".."
 * evaluation, so a backslash-escaped traversal cannot slip through
 * normalization to be rewritten into real "../" later. This is the specific
 * regression the F1 fix closed (normalization previously split on "/" only).
 */
void test_path__backslash_traversal_clamped(void) {

    /* Backslash form must normalize identically to the forward-slash form */
    assert_normalized("/a\\b\\c", "/a/b/c");
    assert_normalized("/..\\..\\etc\\passwd", "/etc/passwd");
    assert_normalized("/a\\..\\b", "/b");

    /* Mixed separators in a single path must still collapse ".." correctly */
    assert_normalized("/a/..\\..\\b", "/b");
    assert_normalized("/a\\../b", "/b");
}

/**
 * Verifies that paths which the shared-folder API must never accept are
 * rejected: relative paths (no leading separator) and NTFS-style named-stream
 * paths containing ':'.
 */
void test_path__rejected(void) {
    assert_rejected("a/b");            /* relative path */
    assert_rejected("relative");       /* relative, single component */
    assert_rejected("/a/b:stream");    /* named data stream */
}
