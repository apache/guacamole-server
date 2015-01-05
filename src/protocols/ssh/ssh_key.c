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

#include "config.h"

#include "ssh_buffer.h"
#include "ssh_key.h"

#include <openssl/bio.h>
#include <openssl/bn.h>
#include <openssl/dsa.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/obj_mac.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>

#include <stdlib.h>
#include <string.h>

ssh_key* ssh_key_alloc(char* data, int length, char* passphrase) {

    ssh_key* key;
    BIO* key_bio;

    char* public_key;
    char* pos;

    /* Create BIO for reading key from memory */
    key_bio = BIO_new_mem_buf(data, length);

    /* If RSA key, load RSA */
    if (length > sizeof(SSH_RSA_KEY_HEADER)-1
            && memcmp(SSH_RSA_KEY_HEADER, data,
                      sizeof(SSH_RSA_KEY_HEADER)-1) == 0) {

        RSA* rsa_key;

        /* Read key */
        rsa_key = PEM_read_bio_RSAPrivateKey(key_bio, NULL, NULL, passphrase);
        if (rsa_key == NULL)
            return NULL;

        /* Allocate key */
        key = malloc(sizeof(ssh_key));
        key->rsa = rsa_key;

        /* Set type */
        key->type = SSH_KEY_RSA;

        /* Allocate space for public key */
        public_key = malloc(4096);
        pos = public_key;

        /* Derive public key */
        buffer_write_string(&pos, "ssh-rsa", sizeof("ssh-rsa")-1);
        buffer_write_bignum(&pos, rsa_key->e);
        buffer_write_bignum(&pos, rsa_key->n);

        /* Save public key to structure */
        key->public_key = public_key;
        key->public_key_length = pos - public_key;

    }

    /* If DSA key, load DSA */
    else if (length > sizeof(SSH_DSA_KEY_HEADER)-1
            && memcmp(SSH_DSA_KEY_HEADER, data,
                      sizeof(SSH_DSA_KEY_HEADER)-1) == 0) {

        DSA* dsa_key;

        /* Read key */
        dsa_key = PEM_read_bio_DSAPrivateKey(key_bio, NULL, NULL, passphrase);
        if (dsa_key == NULL)
            return NULL;

        /* Allocate key */
        key = malloc(sizeof(ssh_key));
        key->dsa = dsa_key;

        /* Set type */
        key->type = SSH_KEY_DSA;

        /* Allocate space for public key */
        public_key = malloc(4096);
        pos = public_key;

        /* Derive public key */
        buffer_write_string(&pos, "ssh-dss", sizeof("ssh-dss")-1);
        buffer_write_bignum(&pos, dsa_key->p);
        buffer_write_bignum(&pos, dsa_key->q);
        buffer_write_bignum(&pos, dsa_key->g);
        buffer_write_bignum(&pos, dsa_key->pub_key);

        /* Save public key to structure */
        key->public_key = public_key;
        key->public_key_length = pos - public_key;

    }

    /* Otherwise, unsupported type */
    else {
        BIO_free(key_bio);
        return NULL;
    }

    /* Copy private key to structure */
    key->private_key_length = length;
    key->private_key = malloc(length);
    memcpy(key->private_key, data, length);

    BIO_free(key_bio);
    return key;

}

const char* ssh_key_error() {

    /* Return static error string */
    return ERR_reason_error_string(ERR_get_error());

}

void ssh_key_free(ssh_key* key) {

    /* Free key-specific data */
    if (key->type == SSH_KEY_RSA)
        RSA_free(key->rsa);
    else if (key->type == SSH_KEY_DSA)
        DSA_free(key->dsa);

    free(key->public_key);
    free(key);
}

int ssh_key_sign(ssh_key* key, const char* data, int length, unsigned char* sig) {

    const EVP_MD* md;
    EVP_MD_CTX md_ctx;

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int dlen, len;

    /* Get SHA1 digest */
    if ((md = EVP_get_digestbynid(NID_sha1)) == NULL)
        return -1;

    /* Digest data */
    EVP_DigestInit(&md_ctx, md);
    EVP_DigestUpdate(&md_ctx, data, length);
    EVP_DigestFinal(&md_ctx, digest, &dlen);

    /* Sign with key */
    switch (key->type) {

        case SSH_KEY_RSA:
            if (RSA_sign(NID_sha1, digest, dlen, sig, &len, key->rsa) == 1)
                return len;

        case SSH_KEY_DSA: {

            DSA_SIG* dsa_sig = DSA_do_sign(digest, dlen, key->dsa);
            if (dsa_sig != NULL) {

                /* Compute size of each half of signature */
                int rlen = BN_num_bytes(dsa_sig->r);
                int slen = BN_num_bytes(dsa_sig->s);

                /* Ensure each number is within the required size */
                if (rlen > DSA_SIG_NUMBER_SIZE || slen > DSA_SIG_NUMBER_SIZE)
                    return -1;

                /* Init to all zeroes */
                memset(sig, 0, DSA_SIG_SIZE);

                /* Add R at the end of the first block of the signature */
                BN_bn2bin(dsa_sig->r, sig + DSA_SIG_SIZE
                                          - DSA_SIG_NUMBER_SIZE - rlen);

                /* Add S at the end of the second block of the signature */
                BN_bn2bin(dsa_sig->s, sig + DSA_SIG_SIZE - slen);

                /* Done */
                DSA_SIG_free(dsa_sig);
                return DSA_SIG_SIZE;

            }

        }

    }

    return -1;

}

