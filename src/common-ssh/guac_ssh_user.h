/*
 * Copyright (C) 2015 Glyptodon LLC
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

#ifndef GUAC_COMMON_SSH_USER_H
#define GUAC_COMMON_SSH_USER_H

#include "guac_ssh_key.h"

/**
 * Data describing an SSH user, including their credentials.
 */
typedef struct guac_common_ssh_user {

    /**
     * The username of this user.
     */
    char* username;

    /**
     * The password which should be used to authenticate this user, if any, or
     * NULL if a private key will be used instead.
     */
    char* password;

    /**
     * The private key which should be used to authenticate this user, if any,
     * or NULL if a password will be used instead.
     */
    guac_common_ssh_key* private_key;

} guac_common_ssh_user;

/**
 * Creates a new SSH user with the given username. When additionally populated
 * with a password or private key, this user can then be used for
 * authentication.
 *
 * @param username
 *     The username of the user being created.
 *
 * @return
 *     A new SSH user having the given username, but no associated password
 *     or private key.
 */
guac_common_ssh_user* guac_common_ssh_create_user(const char* username);

/**
 * Destroys the given user object, releasing all associated resources.
 *
 * @param user
 *     The user to destroy.
 */
void guac_common_ssh_destroy_user(guac_common_ssh_user* user);

/**
 * Associates the given user with the given password, such that that password
 * is used for future authentication attempts.
 *
 * @param user
 *     The user to associate with the given password.
 *
 * @param password
 *     The password to associate with the given user.
 */
void guac_common_ssh_user_set_password(guac_common_ssh_user* user,
        const char* password);

/**
 * Imports the given private key, associating that key with the given user. If
 * necessary to decrypt the key, a passphrase may be specified. The private key
 * must be provided in base64 form. If the private key is imported
 * successfully, it will be used for future authentication attempts.
 *
 * @param user
 *     The user to associate with the given private key.
 *
 * @param private_key
 *     The base64-encoded private key to import.
 *
 * @param passphrase
 *     The passphrase to use to decrypt the given private key, or NULL if no
 *     passphrase should be used.
 *
 * @return
 *     Zero if the private key is successfully imported, or non-zero if the
 *     private key could not be imported due to an error.
 */
int guac_common_ssh_user_import_key(guac_common_ssh_user* user,
        char* private_key, char* passphrase);

#endif

