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

#include "config.h"
#include "sha256.h"

#include <CUnit/CUnit.h>

#include <stdint.h>
#include <string.h>

/**
 * Converts the given raw SHA-256 digest into a null-terminated lowercase
 * hexadecimal string.
 *
 * @param digest
 *     The raw 32-byte digest to convert.
 *
 * @param out
 *     A buffer of at least GUACCLIP_SHA256_HEX_LENGTH bytes which will receive
 *     the null-terminated hexadecimal representation.
 */
static void hexlify(const uint8_t digest[GUACCLIP_SHA256_DIGEST_LENGTH],
        char out[GUACCLIP_SHA256_HEX_LENGTH]) {

    static const char hex[] = "0123456789abcdef";
    int i;

    for (i = 0; i < GUACCLIP_SHA256_DIGEST_LENGTH; i++) {
        out[i * 2]     = hex[(digest[i] >> 4) & 0xF];
        out[i * 2 + 1] = hex[digest[i] & 0xF];
    }
    out[GUACCLIP_SHA256_DIGEST_LENGTH * 2] = '\0';

}

/**
 * Test which verifies that the SHA-256 of the empty string matches the known
 * FIPS 180-4 digest.
 */
void test_sha256__empty(void) {

    char out[GUACCLIP_SHA256_HEX_LENGTH];
    guacclip_sha256_hex("", 0, out);

    CU_ASSERT_STRING_EQUAL(out,
            "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");

}

/**
 * Test which verifies that the SHA-256 of "abc" matches the known FIPS 180-4
 * digest.
 */
void test_sha256__abc(void) {

    char out[GUACCLIP_SHA256_HEX_LENGTH];
    guacclip_sha256_hex("abc", 3, out);

    CU_ASSERT_STRING_EQUAL(out,
            "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");

}

/**
 * Test which verifies that incrementally hashing "abc" across several
 * guacclip_sha256_update() calls yields the same digest as hashing it in one
 * pass.
 */
void test_sha256__multi_update(void) {

    guacclip_sha256_context context;
    uint8_t digest[GUACCLIP_SHA256_DIGEST_LENGTH];
    char out[GUACCLIP_SHA256_HEX_LENGTH];

    guacclip_sha256_init(&context);
    guacclip_sha256_update(&context, "a", 1);
    guacclip_sha256_update(&context, "b", 1);
    guacclip_sha256_update(&context, "c", 1);
    guacclip_sha256_final(&context, digest);

    hexlify(digest, out);

    CU_ASSERT_STRING_EQUAL(out,
            "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");

}
