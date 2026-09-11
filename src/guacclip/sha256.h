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

#ifndef GUACCLIP_SHA256_H
#define GUACCLIP_SHA256_H

#include <stddef.h>
#include <stdint.h>

/**
 * The length, in bytes, of a raw (binary) SHA-256 digest.
 */
#define GUACCLIP_SHA256_DIGEST_LENGTH 32

/**
 * The length, in bytes, of the buffer required to hold the null-terminated
 * hexadecimal (lowercase) representation of a SHA-256 digest.
 */
#define GUACCLIP_SHA256_HEX_LENGTH 65

/**
 * The internal state of an incremental SHA-256 computation. This structure
 * should be initialized with guacclip_sha256_init(), updated incrementally
 * with any number of guacclip_sha256_update() calls, and finalized with a
 * single call to guacclip_sha256_final(). This is a self-contained
 * implementation of the FIPS 180-4 SHA-256 algorithm requiring no external
 * cryptographic libraries.
 */
typedef struct guacclip_sha256_context {

    /**
     * The eight 32-bit working hash values (H0 through H7).
     */
    uint32_t state[8];

    /**
     * The total number of message bytes processed so far.
     */
    uint64_t bitcount;

    /**
     * Buffer holding message bytes which have not yet been processed as part
     * of a complete 64-byte block.
     */
    uint8_t buffer[64];

    /**
     * The number of bytes currently pending within the buffer.
     */
    size_t buffer_length;

} guacclip_sha256_context;

/**
 * Initializes the given SHA-256 context, preparing it to receive message data
 * via guacclip_sha256_update().
 *
 * @param context
 *     The SHA-256 context to initialize.
 */
void guacclip_sha256_init(guacclip_sha256_context* context);

/**
 * Updates the given SHA-256 context with the given block of message data. This
 * function may be called any number of times to incrementally hash a message
 * of arbitrary length. The provided data is binary-safe and may contain null
 * bytes.
 *
 * @param context
 *     The SHA-256 context to update.
 *
 * @param data
 *     A pointer to the message bytes to incorporate into the hash.
 *
 * @param length
 *     The number of bytes to read from the given data pointer.
 */
void guacclip_sha256_update(guacclip_sha256_context* context,
        const void* data, size_t length);

/**
 * Finalizes the given SHA-256 context, producing the raw 32-byte digest of all
 * message data provided via guacclip_sha256_update(). After this call, the
 * context should no longer be used without being re-initialized.
 *
 * @param context
 *     The SHA-256 context to finalize.
 *
 * @param digest
 *     A buffer of at least GUACCLIP_SHA256_DIGEST_LENGTH bytes which will
 *     receive the raw binary digest.
 */
void guacclip_sha256_final(guacclip_sha256_context* context,
        uint8_t digest[GUACCLIP_SHA256_DIGEST_LENGTH]);

/**
 * Computes the SHA-256 digest of the given data in a single call, writing the
 * result as a null-terminated lowercase hexadecimal string.
 *
 * @param data
 *     A pointer to the message bytes to hash. May be NULL only if length is
 *     zero.
 *
 * @param length
 *     The number of bytes to read from the given data pointer.
 *
 * @param output
 *     A buffer of at least GUACCLIP_SHA256_HEX_LENGTH bytes which will receive
 *     the null-terminated hexadecimal digest.
 */
void guacclip_sha256_hex(const void* data, size_t length,
        char output[GUACCLIP_SHA256_HEX_LENGTH]);

#endif
