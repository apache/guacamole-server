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

#include "common-ssh/buffer.h"
#include "common-ssh/dsa-compat.h"
#include "common-ssh/key.h"
#include "common-ssh/rsa-compat.h"

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

guac_common_ssh_key* guac_common_ssh_key_alloc(char* data, int length,
        char* passphrase) {

    guac_common_ssh_key* key;
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

        const BIGNUM* key_e;
        const BIGNUM* key_n;

        /* Read key */
        rsa_key = PEM_read_bio_RSAPrivateKey(key_bio, NULL, NULL, passphrase);
        if (rsa_key == NULL)
            return NULL;

        /* Allocate key */
        key = malloc(sizeof(guac_common_ssh_key));
        key->rsa = rsa_key;

        /* Set type */
        key->type = SSH_KEY_RSA;

        /* Allocate space for public key */
        public_key = malloc(4096);
        pos = public_key;

        /* Retrieve public key */
        RSA_get0_key(rsa_key, &key_n, &key_e, NULL);

        /* Send public key formatted for SSH */
        guac_common_ssh_buffer_write_string(&pos, "ssh-rsa", sizeof("ssh-rsa")-1);
        guac_common_ssh_buffer_write_bignum(&pos, key_e);
        guac_common_ssh_buffer_write_bignum(&pos, key_n);

        /* Save public key to structure */
        key->public_key = public_key;
        key->public_key_length = pos - public_key;

    }

    /* If DSA key, load DSA */
    else if (length > sizeof(SSH_DSA_KEY_HEADER)-1
            && memcmp(SSH_DSA_KEY_HEADER, data,
                      sizeof(SSH_DSA_KEY_HEADER)-1) == 0) {

        DSA* dsa_key;

        const BIGNUM* key_p;
        const BIGNUM* key_q;
        const BIGNUM* key_g;
        const BIGNUM* pub_key;

        /* Read key */
        dsa_key = PEM_read_bio_DSAPrivateKey(key_bio, NULL, NULL, passphrase);
        if (dsa_key == NULL)
            return NULL;

        /* Allocate key */
        key = malloc(sizeof(guac_common_ssh_key));
        key->dsa = dsa_key;

        /* Set type */
        key->type = SSH_KEY_DSA;

        /* Allocate space for public key */
        public_key = malloc(4096);
        pos = public_key;

        /* Retrieve public key */
        DSA_get0_pqg(dsa_key, &key_p, &key_q, &key_g);
        DSA_get0_key(dsa_key, &pub_key, NULL);

        /* Send public key formatted for SSH */
        guac_common_ssh_buffer_write_string(&pos, "ssh-dss", sizeof("ssh-dss")-1);
        guac_common_ssh_buffer_write_bignum(&pos, key_p);
        guac_common_ssh_buffer_write_bignum(&pos, key_q);
        guac_common_ssh_buffer_write_bignum(&pos, key_g);
        guac_common_ssh_buffer_write_bignum(&pos, pub_key);

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

const char* guac_common_ssh_key_error() {

    /* Return static error string */
    return ERR_reason_error_string(ERR_get_error());

}

void guac_common_ssh_key_free(guac_common_ssh_key* key) {

    /* Free key-specific data */
    if (key->type == SSH_KEY_RSA)
        RSA_free(key->rsa);
    else if (key->type == SSH_KEY_DSA)
        DSA_free(key->dsa);

    free(key->private_key);
    free(key->public_key);
    free(key);
}

int guac_common_ssh_key_sign(guac_common_ssh_key* key, const char* data,
        int length, unsigned char* sig) {

    const EVP_MD* md;

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int dlen, len;

    /* Get SHA1 digest */
    if ((md = EVP_get_digestbynid(NID_sha1)) == NULL)
        return -1;

    /* Allocate digest context */
    EVP_MD_CTX* md_ctx = EVP_MD_CTX_create();
    if (md_ctx == NULL)
        return -1;

    /* Digest data */
    EVP_DigestInit(md_ctx, md);
    EVP_DigestUpdate(md_ctx, data, length);
    EVP_DigestFinal(md_ctx, digest, &dlen);

    /* Digest context no longer needed */
    EVP_MD_CTX_destroy(md_ctx);

    /* Sign with key */
    switch (key->type) {

        case SSH_KEY_RSA:
            if (RSA_sign(NID_sha1, digest, dlen, sig, &len, key->rsa) == 1)
                return len;
            break;

        case SSH_KEY_DSA: {

            DSA_SIG* dsa_sig = DSA_do_sign(digest, dlen, key->dsa);
            if (dsa_sig != NULL) {

                const BIGNUM* sig_r;
                const BIGNUM* sig_s;

                /* Retrieve DSA signature values */
                DSA_SIG_get0(dsa_sig, &sig_r, &sig_s);

                /* Compute size of each half of signature */
                int rlen = BN_num_bytes(sig_r);
                int slen = BN_num_bytes(sig_s);

                /* Ensure each number is within the required size */
                if (rlen > DSA_SIG_NUMBER_SIZE || slen > DSA_SIG_NUMBER_SIZE)
                    return -1;

                /* Init to all zeroes */
                memset(sig, 0, DSA_SIG_SIZE);

                /* Add R at the end of the first block of the signature */
                BN_bn2bin(sig_r, sig + DSA_SIG_SIZE
                                     - DSA_SIG_NUMBER_SIZE - rlen);

                /* Add S at the end of the second block of the signature */
                BN_bn2bin(sig_s, sig + DSA_SIG_SIZE - slen);

                /* Done */
                DSA_SIG_free(dsa_sig);
                return DSA_SIG_SIZE;

            }

        }

    }

    return -1;

}

