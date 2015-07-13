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

#include "guac_ssh_key.h"
#include "guac_ssh_user.h"

#include <stdlib.h>
#include <string.h>

guac_common_ssh_user* guac_common_ssh_create_user(const char* username) {

    guac_common_ssh_user* user = malloc(sizeof(guac_common_ssh_user));

    /* Init user */
    user->username = strdup(username);
    user->password = NULL;
    user->private_key = NULL;

    return user;

}

void guac_common_ssh_destroy_user(guac_common_ssh_user* user) {

    /* Free private key, if present */
    if (user->private_key != NULL)
        guac_common_ssh_key_free(user->private_key);

    /* Free all other data */
    free(user->password);
    free(user->username);
    free(user);

}

void guac_common_ssh_user_set_password(guac_common_ssh_user* user,
        const char* password) {

    /* Replace current password with given value */
    free(user->password);
    user->password = strdup(password);

}

int guac_common_ssh_user_import_key(guac_common_ssh_user* user,
        char* private_key, char* passphrase) {

    /* Free existing private key, if present */
    if (user->private_key != NULL)
        guac_common_ssh_key_free(user->private_key);

    /* Attempt to read key without passphrase if none given */
    if (passphrase == NULL)
        user->private_key = guac_common_ssh_key_alloc(private_key,
                strlen(private_key), "");

    /* Otherwise, use provided passphrase */
    else
        user->private_key = guac_common_ssh_key_alloc(private_key,
                strlen(private_key), passphrase);

    /* Fail if key could not be read */
    return user->private_key == NULL;

}

