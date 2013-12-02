
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

