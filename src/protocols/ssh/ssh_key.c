
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
        buffer_write_string(&pos, "ssh-dsa", sizeof("ssh-dsa")-1);
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

        case SSH_KEY_DSA:
            if (DSA_sign(NID_sha1, digest, dlen, sig, &len, key->dsa) == 1)
                return len;

    }

    return -1;

}

