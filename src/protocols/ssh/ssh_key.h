
#ifndef _GUAC_SSH_KEY_H
#define _GUAC_SSH_KEY_H

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/dsa.h>
#include <openssl/rsa.h>

/**
 * The expected header of RSA private keys.
 */
#define SSH_RSA_KEY_HEADER "-----BEGIN RSA PRIVATE KEY-----"

/**
 * The expected header of DSA private keys.
 */
#define SSH_DSA_KEY_HEADER "-----BEGIN DSA PRIVATE KEY-----"

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
 * Frees all memory associated with the given key.
 */
void ssh_key_free(ssh_key* key);

/**
 * Signs the given data using the given key, returning the length of the
 * signature in bytes, or a value less than zero on error.
 */
int ssh_key_sign(ssh_key* key, const char* data, int length, u_char* sig);

#endif

