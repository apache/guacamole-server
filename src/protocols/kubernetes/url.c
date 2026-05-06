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

#include "url.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Returns whether the given character is a character that need not be
 * escaped when included as part of a component of a URL.
 *
 * @param c
 *     The character to test.
 *
 * @return
 *     Zero if the character does not need to be escaped when included as
 *     part of a component of a URL, non-zero otherwise.
 */
static int guac_kubernetes_is_url_safe(char c) {
    return (c >= 'A' && c <= 'Z')
        || (c >= 'a' && c <= 'z')
        || (c >= '0' && c <= '9')
        || strchr("-_.!~*'()", c) != NULL;
}

int guac_kubernetes_escape_url_component(char* output, int length,
        const char* str) {

    char* current = output;
    while (*str != '\0') {

        char c = *str;

        /* Store alphanumeric characters verbatim */
        if (guac_kubernetes_is_url_safe(c)) {

            /* Verify space exists for single character */
            if (length < 1)
                return 1;

            *(current++) = c;
            length--;

        }

        /* Escape EVERYTHING else as hex */
        else {

            /* Verify space exists for hex-encoded character */
            if (length < 4)
                return 1;

            snprintf(current, 4, "%%%02X", (int) c);

            current += 3;
            length -= 3;
        }

        /* Next character */
        str++;

    }

    /* Verify space exists for null terminator */
    if (length < 1)
        return 1;

    /* Append null terminator */
    *current = '\0';
    return 0;

}

/**
 * Splits exec_command on whitespace and appends each argument as a separate
 * Kubernetes "command=..." query parameter.
 *
 * Quoting is not supported. "$TERM" is replaced with terminal_type unless it
 * is part of a longer shell variable name, such as "$TERMINAL".
 *
 * @param buffer
 *     The buffer that should receive the appended query parameters.
 *
 * @param length
 *     The number of bytes available in the given buffer.
 *
 * @param exec_command
 *     The command string to split into arguments.
 *
 * @param terminal_type
 *     The replacement value for "$TERM", or NULL if "$TERM" should be
 *     replaced with an empty string.
 *
 * @return
 *     Zero on success, non-zero if the buffer is too small.
 */
static int guac_kubernetes_append_exec_command(char* buffer, int length,
        const char* exec_command, const char* terminal_type) {

    const char* replacement = terminal_type != NULL ? terminal_type : "";
    size_t replacement_length = strlen(replacement);

    char arg[GUAC_KUBERNETES_MAX_ENDPOINT_LENGTH];
    size_t arg_length = 0;

    /* Cast before ctype.h calls to avoid undefined behavior. */
    while (*exec_command != '\0') {

        while (isspace((unsigned char)*exec_command))
            exec_command++;

        arg_length = 0;

        while (*exec_command != '\0' && !isspace((unsigned char)*exec_command)) {

            if (strncmp(exec_command, "$TERM", 5) == 0) {

                /* Match "$TERM" only when it is not part of a longer shell
                 * variable name, like "$TERMINAL". */
                char next = exec_command[5];

                if (!(isalnum((unsigned char)next) || next == '_')) {

                    /* Leave room for the final null terminator. */
                    if (arg_length + replacement_length >= sizeof(arg))
                        return 1;

                    memcpy(arg + arg_length, replacement, replacement_length);
                    arg_length += replacement_length;
                    exec_command += 5;
                    continue;
                }
            }

            if (arg_length + 1 >= sizeof(arg))
                return 1;

            arg[arg_length++] = *exec_command++;
        }

        /* Avoid adding "command=" for trailing whitespace. */
        if (arg_length > 0) {
            arg[arg_length] = '\0';

            if (guac_kubernetes_append_endpoint_param(buffer, length,
                        "command", arg))
                return 1;
        }
    }

    return 0;
}

int guac_kubernetes_append_endpoint_param(char* buffer, int length,
        const char* param_name, const char* param_value) {

    char escaped_param_value[GUAC_KUBERNETES_MAX_ENDPOINT_LENGTH];

    /* Escape value */
    if (guac_kubernetes_escape_url_component(escaped_param_value,
                    sizeof(escaped_param_value), param_value))
            return 1;

    char* str = buffer;

    int str_len = 0;
    int qmark = 0;

    while (*str != '\0') {

        /* Look for a question mark */
        if (*str=='?') qmark = 1;

        /* Compute the buffer string length */
        str_len++;

        /* Verify the buffer null terminated */
        if (str_len >= length) return 1;

        /* Next character */
        str++;
    }

    /* Determine the parameter delimiter */
    char delimiter = '?';
    if (qmark) delimiter = '&';

    /* Advance to end of buffer, where the new parameter and delimiter need to
     * be appended */
    buffer += str_len;
    length -= str_len;

    /* Write the parameter and delimiter to the buffer */
    int written = snprintf(buffer, length, "%c%s=%s", delimiter,
            param_name, escaped_param_value);

    /* The parameter was successfully added if it was written to the given
     * buffer without truncation */
    return (written < 0 || written >= length);
}

int guac_kubernetes_endpoint_uri(char* buffer, int length,
        const char* kubernetes_namespace, const char* kubernetes_pod,
        const char* kubernetes_container, const char* exec_command,
        const char* terminal_type) {

    char escaped_namespace[GUAC_KUBERNETES_MAX_ENDPOINT_LENGTH];
    char escaped_pod[GUAC_KUBERNETES_MAX_ENDPOINT_LENGTH];

    /* Escape Kubernetes namespace */
    if (guac_kubernetes_escape_url_component(escaped_namespace,
                sizeof(escaped_namespace), kubernetes_namespace))
        return 1;

    /* Escape name of Kubernetes pod */
    if (guac_kubernetes_escape_url_component(escaped_pod,
                sizeof(escaped_pod), kubernetes_pod))
        return 1;

    /* Determine the call type */
    char* call = "attach";
    if (exec_command != NULL)
        call = "exec";

    int written;

    /* Generate the endpoint path and write to the buffer */
    written = snprintf(buffer, length,
        "/api/v1/namespaces/%s/pods/%s/%s", escaped_namespace, escaped_pod, call);

    /* Operation successful if the endpoint path was written to the given
     * buffer without truncation */
    if (written < 0 || written >= length)
        return 1;

    /* Append exec command, splitting on whitespace into separate "command"
     * query parameters and substituting any $TERM token with the configured
     * terminal type so that callers can choose whether and how to propagate
     * the terminal type into the executed command. */
    if (exec_command != NULL) {
        if (guac_kubernetes_append_exec_command(buffer, length,
                    exec_command, terminal_type))
            return 1;
    }

    /* Append kubernetes container parameter */
    if (kubernetes_container != NULL) {
        if (guac_kubernetes_append_endpoint_param(buffer,
                    length, "container", kubernetes_container))
            return 1;
    }

    /* Append stdin, stdout and tty parameters */
    return (guac_kubernetes_append_endpoint_param(buffer, length, "stdin", "true"))
        || (guac_kubernetes_append_endpoint_param(buffer, length, "stdout", "true"))
        || (guac_kubernetes_append_endpoint_param(buffer, length, "tty", "true"));
}
