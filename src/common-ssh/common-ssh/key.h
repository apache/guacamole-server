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

#ifndef GUAC_COMMON_SSH_KEY_H
#define GUAC_COMMON_SSH_KEY_H

#include "config.h"

#include <openssl/ossl_typ.h>

/**
 * The expected header of RSA private keys.
 */
#define SSH_RSA_KEY_HEADER "-----BEGIN RSA PRIVATE KEY-----"

/**
 * The expected header of DSA private keys.
 */
#define SSH_DSA_KEY_HEADER "-----BEGIN DSA PRIVATE KEY-----"

/**
 * The size of single number within a DSA signature, in bytes.
 */
#define DSA_SIG_NUMBER_SIZE 20

/**
 * The size of a DSA signature, in bytes.
 */
#define DSA_SIG_SIZE DSA_SIG_NUMBER_SIZE*2 

/**
 * The type of an SSH key.
 */
typedef enum guac_common_ssh_key_type {

    /**
     * RSA key.
     */
    SSH_KEY_RSA,

    /**
     * DSA key.
     */
    SSH_KEY_DSA

} guac_common_ssh_key_type;

/**
 * Abstraction of a key used for SSH authentication.
 */
typedef struct guac_common_ssh_key {

    /**
     * The type of this key.
     */
    guac_common_ssh_key_type type;

    /**
     * Underlying RSA private key, if any.
     */
    RSA* rsa;

    /**
     * Underlying DSA private key, if any.
     */
    DSA* dsa;

    /**
     * The associated public key, encoded as necessary for SSH.
     */
    char* public_key;

    /**
     * The length of the public key, in bytes.
     */
    int public_key_length;

    /**
     * The private key, encoded as necessary for SSH.
     */
    char* private_key;

    /**
     * The length of the private key, in bytes.
     */
    int private_key_length;

} guac_common_ssh_key;

/**
 * Allocates a new key containing the given private key data and specified
 * passphrase. If unable to read the key, NULL is returned.
 *
 * @param data
 *     The base64-encoded data to decode when reading the key.
 *
 * @param length
 *     The length of the provided data, in bytes.
 *
 * @param passphrase
 *     The passphrase to use when decrypting the key, if any, or an empty
 *     string or NULL if no passphrase is needed.
 *
 * @return
 *     The decoded, decrypted private key, or NULL if the key could not be
 *     decoded.
 */
guac_common_ssh_key* guac_common_ssh_key_alloc(char* data, int length,
        char* passphrase);

/**
 * Returns a statically-allocated string describing the most recent SSH key
 * error.
 *
 * @return
 *     A statically-allocated string describing the most recent SSH key error.
 */
const char* guac_common_ssh_key_error();

/**
 * Frees all memory associated with the given key.
 *
 * @param key
 *     The key to free.
 */
void guac_common_ssh_key_free(guac_common_ssh_key* key);

/**
 * Signs the given data using the given key, returning the length of the
 * signature in bytes, or a value less than zero on error.
 *
 * @param key
 *     The key to use when signing the given data.
 *
 * @param data
 *     The arbitrary data to sign.
 *
 * @param length
 *     The length of the arbitrary data being signed, in bytes.
 *
 * @param sig
 *     The buffer into which the signature should be written. The buffer must
 *     be at least DSA_SIG_SIZE for DSA keys. For RSA keys, the signature size
 *     is dependent only on key size, and is equal to the length of the
 *     modulus, in bytes.
 *
 * @return
 *     The number of bytes in the resulting signature.
 */
int guac_common_ssh_key_sign(guac_common_ssh_key* key, const char* data,
        int length, unsigned char* sig);

#endif

