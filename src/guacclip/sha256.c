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

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/**
 * The SHA-256 round constants, as defined by FIPS 180-4. These are the first
 * 32 bits of the fractional parts of the cube roots of the first 64 primes.
 */
static const uint32_t GUACCLIP_SHA256_K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

/**
 * Rotates the given 32-bit value right by the given number of bits.
 *
 * @param value
 *     The 32-bit value to rotate.
 *
 * @param bits
 *     The number of bits by which to rotate the value.
 *
 * @return
 *     The rotated 32-bit value.
 */
static uint32_t guacclip_sha256_rotr(uint32_t value, unsigned int bits) {
    return (value >> bits) | (value << (32 - bits));
}

/**
 * Processes a single 64-byte block of message data, updating the working hash
 * values within the given context.
 *
 * @param context
 *     The SHA-256 context whose state should be updated.
 *
 * @param block
 *     A pointer to exactly 64 bytes of message data to process.
 */
static void guacclip_sha256_process(guacclip_sha256_context* context,
        const uint8_t block[64]) {

    uint32_t w[64];
    int i;

    /* Prepare the first 16 words directly from the big-endian block */
    for (i = 0; i < 16; i++) {
        w[i] = ((uint32_t) block[i * 4]     << 24)
             | ((uint32_t) block[i * 4 + 1] << 16)
             | ((uint32_t) block[i * 4 + 2] <<  8)
             | ((uint32_t) block[i * 4 + 3]);
    }

    /* Extend the first 16 words into the remaining 48 words */
    for (i = 16; i < 64; i++) {
        uint32_t s0 = guacclip_sha256_rotr(w[i - 15], 7)
                    ^ guacclip_sha256_rotr(w[i - 15], 18)
                    ^ (w[i - 15] >> 3);
        uint32_t s1 = guacclip_sha256_rotr(w[i - 2], 17)
                    ^ guacclip_sha256_rotr(w[i - 2], 19)
                    ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }

    /* Initialize working variables from the current hash state */
    uint32_t a = context->state[0];
    uint32_t b = context->state[1];
    uint32_t c = context->state[2];
    uint32_t d = context->state[3];
    uint32_t e = context->state[4];
    uint32_t f = context->state[5];
    uint32_t g = context->state[6];
    uint32_t h = context->state[7];

    /* Perform the main compression loop */
    for (i = 0; i < 64; i++) {

        uint32_t s1 = guacclip_sha256_rotr(e, 6)
                    ^ guacclip_sha256_rotr(e, 11)
                    ^ guacclip_sha256_rotr(e, 25);
        uint32_t ch = (e & f) ^ ((~e) & g);
        uint32_t temp1 = h + s1 + ch + GUACCLIP_SHA256_K[i] + w[i];
        uint32_t s0 = guacclip_sha256_rotr(a, 2)
                    ^ guacclip_sha256_rotr(a, 13)
                    ^ guacclip_sha256_rotr(a, 22);
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        uint32_t temp2 = s0 + maj;

        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;

    }

    /* Add the compressed chunk to the current hash value */
    context->state[0] += a;
    context->state[1] += b;
    context->state[2] += c;
    context->state[3] += d;
    context->state[4] += e;
    context->state[5] += f;
    context->state[6] += g;
    context->state[7] += h;

}

void guacclip_sha256_init(guacclip_sha256_context* context) {

    /* Initial hash values: first 32 bits of the fractional parts of the
     * square roots of the first 8 primes */
    context->state[0] = 0x6a09e667;
    context->state[1] = 0xbb67ae85;
    context->state[2] = 0x3c6ef372;
    context->state[3] = 0xa54ff53a;
    context->state[4] = 0x510e527f;
    context->state[5] = 0x9b05688c;
    context->state[6] = 0x1f83d9ab;
    context->state[7] = 0x5be0cd19;

    context->bitcount = 0;
    context->buffer_length = 0;

}

void guacclip_sha256_update(guacclip_sha256_context* context,
        const void* data, size_t length) {

    const uint8_t* input = (const uint8_t*) data;
    size_t i;

    for (i = 0; i < length; i++) {

        context->buffer[context->buffer_length++] = input[i];

        /* Process each complete 64-byte block */
        if (context->buffer_length == 64) {
            guacclip_sha256_process(context, context->buffer);
            context->bitcount += 512;
            context->buffer_length = 0;
        }

    }

}

void guacclip_sha256_final(guacclip_sha256_context* context,
        uint8_t digest[GUACCLIP_SHA256_DIGEST_LENGTH]) {

    int i;

    /* Total message length in bits, including the bytes still buffered */
    uint64_t total_bits = context->bitcount + (uint64_t) context->buffer_length * 8;

    /* Append the mandatory 0x80 padding byte */
    context->buffer[context->buffer_length++] = 0x80;

    /* If there is not enough room for the 8-byte length, pad and flush */
    if (context->buffer_length > 56) {
        while (context->buffer_length < 64)
            context->buffer[context->buffer_length++] = 0x00;
        guacclip_sha256_process(context, context->buffer);
        context->buffer_length = 0;
    }

    /* Pad with zeroes up to the length field */
    while (context->buffer_length < 56)
        context->buffer[context->buffer_length++] = 0x00;

    /* Append the message length as a big-endian 64-bit integer */
    for (i = 7; i >= 0; i--)
        context->buffer[context->buffer_length++] =
            (uint8_t) (total_bits >> (i * 8));

    guacclip_sha256_process(context, context->buffer);

    /* Produce the final big-endian digest */
    for (i = 0; i < 8; i++) {
        digest[i * 4]     = (uint8_t) (context->state[i] >> 24);
        digest[i * 4 + 1] = (uint8_t) (context->state[i] >> 16);
        digest[i * 4 + 2] = (uint8_t) (context->state[i] >>  8);
        digest[i * 4 + 3] = (uint8_t) (context->state[i]);
    }

}

void guacclip_sha256_hex(const void* data, size_t length,
        char output[GUACCLIP_SHA256_HEX_LENGTH]) {

    static const char hex[] = "0123456789abcdef";
    uint8_t digest[GUACCLIP_SHA256_DIGEST_LENGTH];
    guacclip_sha256_context context;
    int i;

    guacclip_sha256_init(&context);
    guacclip_sha256_update(&context, data, length);
    guacclip_sha256_final(&context, digest);

    for (i = 0; i < GUACCLIP_SHA256_DIGEST_LENGTH; i++) {
        output[i * 2]     = hex[(digest[i] >> 4) & 0xF];
        output[i * 2 + 1] = hex[digest[i] & 0xF];
    }
    output[GUACCLIP_SHA256_DIGEST_LENGTH * 2] = '\0';

}
