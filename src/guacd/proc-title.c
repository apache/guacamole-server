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

#include "proc-title.h"

#include <guacamole/client.h>
#include <guacamole/string.h>

#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/**
 * The number of characters to preserve at the beginning and end of a username
 * that is sufficiently long.
 */
#define GUACD_PROC_TITLE_USERNAME_UNMASKED_LENGTH 2

/**
 * The fixed string substituted for the masked (middle) portion of a
 * username by guacd_proc_title_mask_username().
 *
 * @note
 *     The size of the mask is intentionally constant, rather than matching the
 *     number of characters removed, so that information regarding the length of
 *     the masked username is relatively limited to the casual observer.
 */
#define GUACD_PROC_TITLE_USERNAME_MASK "****"

/**
 * The minimum required username length for guacd_proc_title_mask_username() to
 * permit revealing characters at the leading and trailing edges of the
 * username. Shorter usernames must be masked in their entirety. This value is
 * in bytes.
 *
 * @note
 *     The "+ 1" here ensures that at least one character is always masked,
 *     rather than permitting 4-character usernames to be fully revealed.
 */
#define GUACD_PROC_TITLE_USERNAME_MIN_REVEAL \
    (2 * GUACD_PROC_TITLE_USERNAME_UNMASKED_LENGTH + 1)

/**
 * The minimum buffer size required to contain any string produced by
 * guacd_proc_title_mask_username(), including null terminator. This value is in
 * bytes.
 */
#define GUACD_PROC_TITLE_USERNAME_MASKED_BUFSIZE \
    (2 * GUACD_PROC_TITLE_USERNAME_UNMASKED_LENGTH + sizeof(GUACD_PROC_TITLE_USERNAME_MASK))

/**
 * The start of the writable area used for the current process title. This value
 * must not be modified without holding guacd_proc_title_lock.
 */
static char* guacd_proc_title_buffer = NULL;

/**
 * The number of bytes available within guacd_proc_title_buffer. This value must
 * not be modified without holding guacd_proc_title_lock.
 */
static size_t guacd_proc_title_buffer_size = 0;

/**
 * Lock which guards changes to guacd_proc_title_buffer and
 * guacd_proc_title_buffer_size.
 */
static pthread_mutex_t guacd_proc_title_lock = PTHREAD_MUTEX_INITIALIZER;

/**
 * Returns whether the given character is within the ASCII printable range.
 * Printable ASCII characters are all characters between space (0x20) and tilde
 * (0x7E), inclusive.
 *
 * @param c
 *     The character to test.
 *
 * @return
 *     Non-zero if the character is within the ASCII printable range, zero
 *     otherwise.
 */
static int guacd_proc_title_is_printable(unsigned char c) {
    return c >= 0x20 && c <= 0x7E;
}

/**
 * Replaces the title of the current process with the given printf-style
 * formatted string. The formatted title is truncated to fit the available space
 * of guacd_proc_title_buffer.
 *
 * @param format
 *     A printf-style format string producing the new title.
 *
 * @param ...
 *     Any arguments required by the format string.
 */
static void guacd_proc_title_set(const char* format, ...) {

    pthread_mutex_lock(&guacd_proc_title_lock);

    if (guacd_proc_title_buffer == NULL || guacd_proc_title_buffer_size == 0) {
        pthread_mutex_unlock(&guacd_proc_title_lock);
        return;
    }

    va_list args;
    va_start(args, format);
    int length = vsnprintf(guacd_proc_title_buffer,
            guacd_proc_title_buffer_size, format, args);
    va_end(args);

    /* Leave title empty if formatting fails */
    if (length < 0)
        memset(guacd_proc_title_buffer, '\0', guacd_proc_title_buffer_size);

    /* Clear whatever remains of any previous, longer title (the process
     * command line would otherwise be reported in full based on the state of
     * the process at start, independently of any null terminators) */
    else if ((size_t) length < guacd_proc_title_buffer_size - 1) {
        char* title_end = guacd_proc_title_buffer + length;
        size_t remaining = guacd_proc_title_buffer_size - length;
        memset(title_end, '\0', remaining);
    }

    pthread_mutex_unlock(&guacd_proc_title_lock);

}

/**
 * Writes a partially obfuscated form of the given username into the provided
 * buffer for inclusion in a process title. The username is intended to still be
 * at least somewhat readable, such that an administrator can recognize a
 * session at a glance, yet still be relatively obscure to casual observers.
 *
 * @param user
 *     The username to obfuscate, which may be NULL.
 *
 * @param buffer
 *     The buffer to receive the obfuscated username.
 *
 * @param buffer_size
 *     The size of the buffer, in bytes.
 */
static void guacd_proc_title_mask_username(const char* user,
        char* buffer, size_t buffer_size) {

    if (!guac_is_nonempty(user)) {
        buffer[0] = '\0';
        return;
    }

    size_t username_length = strlen(user);

    /* Reveal the edges of a username only if sufficiently long and consisting
     * purely of printable characters */
    int reveal = (username_length >= GUACD_PROC_TITLE_USERNAME_MIN_REVEAL);
    for (size_t i = 0; reveal && i < username_length; i++) {
        if (!guacd_proc_title_is_printable((unsigned char) user[i]))
            reveal = 0;
    }

    /* Use the fixed mask as the entire content for all usernames that cannot be
     * partially revealed (including usernames that may contain non-printable
     * characters) */
    if (!reveal) {
        guac_strlcpy(buffer, GUACD_PROC_TITLE_USERNAME_MASK, buffer_size);
        return;
    }

    /* Preserve the first and last GUACD_PROC_TITLE_USERNAME_UNMASKED_LENGTH
     * characters, replacing all characters between those edges with the fixed
     * mask */

    const char* suffix = user + username_length
            - GUACD_PROC_TITLE_USERNAME_UNMASKED_LENGTH;

    snprintf(buffer, buffer_size, "%.*s" GUACD_PROC_TITLE_USERNAME_MASK "%s",
            GUACD_PROC_TITLE_USERNAME_UNMASKED_LENGTH, user, suffix);

}

void guacd_proc_title_init(char* process_name) {

    if (process_name == NULL)
        return;

    pthread_mutex_lock(&guacd_proc_title_lock);

    guacd_proc_title_buffer = process_name;
    guacd_proc_title_buffer_size = strlen(process_name) + 1;

    pthread_mutex_unlock(&guacd_proc_title_lock);

}

void guacd_proc_title_set_connection(const char* protocol,
        const guac_client_info* info) {

    static const char* established_protocol = "UNKNOWN";
    if (protocol != NULL)
        established_protocol = protocol;

    /* Identify only the protocol if nothing is yet known of the connection */
    if (info == NULL) {
        guacd_proc_title_set(GUACD_PROC_TITLE_PREFIX "%s" GUACD_PROC_TITLE_SUFFIX,
            established_protocol);
        return;
    }

    /* Determine what components of guac_client_info were provided */
    int has_user = guac_is_nonempty(info->username);
    int has_host = guac_is_nonempty(info->hostname);
    int has_port = guac_is_nonempty(info->port);
    int has_resource = guac_is_nonempty(info->resource_name);

    /* Include any endpoint information */
    if (has_user || has_host || has_port) {

        char endpoint[GUACD_PROC_TITLE_BUFSIZE];
        const char* hostname = has_host ? info->hostname : "(NONE)";

        char masked_user[GUACD_PROC_TITLE_USERNAME_MASKED_BUFSIZE];
        guacd_proc_title_mask_username(info->username, masked_user, sizeof(masked_user));

        /* Build string representing destination endpoint */
        if (has_user && has_port)
            snprintf(endpoint, sizeof(endpoint), "%s@%s:%s", masked_user, hostname, info->port);
        else if (has_user)
            snprintf(endpoint, sizeof(endpoint), "%s@%s", masked_user, hostname);
        else if (has_port)
            snprintf(endpoint, sizeof(endpoint), "%s:%s", hostname, info->port);
        else
            snprintf(endpoint, sizeof(endpoint), "%s", hostname);

        /* Include resource in resulting title, if present */
        if (has_resource) {
            guacd_proc_title_set(GUACD_PROC_TITLE_PREFIX "%s" GUACD_PROC_TITLE_SUFFIX " %s %s",
                    established_protocol, endpoint, info->resource_name);
        }
        else {
            guacd_proc_title_set(GUACD_PROC_TITLE_PREFIX "%s" GUACD_PROC_TITLE_SUFFIX " %s",
                    established_protocol, endpoint);
        }

        return;

    }

    /* Lacking an endpoint, use only the protocol and arbitrary resource name in
     * the title */
    if (has_resource) {
        guacd_proc_title_set(GUACD_PROC_TITLE_PREFIX "%s" GUACD_PROC_TITLE_SUFFIX " %s",
                established_protocol, info->resource_name);
    }

    /* Lacking any additional information at all, use just the protocol */
    else {
        guacd_proc_title_set(GUACD_PROC_TITLE_PREFIX "%s" GUACD_PROC_TITLE_SUFFIX,
                established_protocol);
    }

}
