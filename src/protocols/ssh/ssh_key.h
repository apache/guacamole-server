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


#ifndef _GUAC_SSH_KEY_H
#define _GUAC_SSH_KEY_H

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
typedef enum ssh_key_type {

    /**
     * RSA key.
     */
    SSH_KEY_RSA,

    /**
     * DSA key.
     */
    SSH_KEY_DSA

} ssh_key_type;

/**
 * Abstraction of a key used for SSH authentication.
 */
typedef struct ssh_key {

    /**
     * The type of this key.
     */
    ssh_key_type type;

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

} ssh_key;

/**
 * Allocates a new key containing the given private key data and specified
 * passphrase. If unable to read the key, NULL is returned.
 */
ssh_key* ssh_key_alloc(char* data, int length, char* passphrase);

/**
 * Returns a statically-allocated string describing the most recent SSH key
 * error.
 *
 * @return
 *     A statically-allocated string describing the most recent SSH key error.
 */
const char* ssh_key_error();

/**
 * Frees all memory associated with the given key.
 */
void ssh_key_free(ssh_key* key);

/**
 * Signs the given data using the given key, returning the length of the
 * signature in bytes, or a value less than zero on error.
 */
int ssh_key_sign(ssh_key* key, const char* data, int length, unsigned char* sig);

#endif

