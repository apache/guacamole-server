
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is libguac-client-ssh.
 *
 * The Initial Developer of the Original Code is
 * Michael Jumper.
 * Portions created by the Initial Developer are Copyright (C) 2011
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include <string.h>

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>

#include "ssh_buffer.h"
#include "ssh_key.h"

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
    else
        return NULL;

    /* Copy private key to structure */
    key->private_key_length = length;
    key->private_key = malloc(length);
    memcpy(key->private_key, data, length);

    return key;

}

void ssh_key_free(ssh_key* key) {
    free(key->public_key);
    free(key);
}

int ssh_key_sign(ssh_key* key, const char* data, int length, u_char* sig) {

    const EVP_MD* md;
    EVP_MD_CTX md_ctx;

    u_char digest[EVP_MAX_MD_SIZE];
    u_int dlen, len;

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

