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
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "guacamole/proctitle.h"

#include <guacamole/mem.h>
#include <guacamole/string.h>

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef HAVE_PRCTL
#include <sys/prctl.h>
#endif

extern char** environ;

/**
 * The size, in bytes, of the buffer used to hold a thread "comm" name. The
 * Linux kernel limits a thread's comm to TASK_COMM_LEN bytes (16), i.e. 15
 * characters plus a null terminator, and silently truncates anything longer,
 * so a larger buffer would serve no purpose.
 */
#define GUAC_PROCTITLE_COMM_LENGTH 16

/**
 * The size, in bytes, of the buffer used to hold the /proc path addressing a
 * thread's comm pseudo-file. This must be large enough for
 * GUAC_PROCTITLE_COMM_PATH expanded with the widest possible PID. A 64-bit
 * PID is at most 20 decimal digits, and the surrounding literal text is 21
 * characters, so 64 bytes leaves comfortable headroom.
 */
#define GUAC_PROCTITLE_COMM_PATH_LENGTH 64

/**
 * Format string for the /proc path of a thread's comm pseudo-file. The single
 * argument is the target thread's TID; for the thread-group leader (the
 * process "main" thread) this equals the process PID returned by getpid().
 */
#define GUAC_PROCTITLE_COMM_PATH "/proc/self/task/%ld/comm"

/**
 * The number of leading and trailing characters of a username preserved by
 * mask_username(); the characters in between are replaced with a fixed run of
 * asterisks.
 */
#define GUAC_PROCTITLE_USER_REVEAL 2

/**
 * The fixed asterisk run substituted for the masked (middle) portion of a
 * username by mask_username(). Its width is intentionally constant, rather
 * than matching the number of characters removed, so the obfuscated form does
 * not leak the original username's length.
 */
#define GUAC_PROCTITLE_USER_MASK "****"

/**
 * The minimum username length, in bytes, for which mask_username() reveals the
 * leading and trailing characters. Names shorter than twice
 * GUAC_PROCTITLE_USER_REVEAL characters would have all (or overlapping)
 * characters exposed, so they are masked in their entirety instead. The "+ 1"
 * guarantees at least one character is always masked.
 */
#define GUAC_PROCTITLE_USER_MIN_REVEAL (2 * GUAC_PROCTITLE_USER_REVEAL + 1)

/**
 * The size, in bytes, of a buffer guaranteed to hold any string produced by
 * mask_username(): GUAC_PROCTITLE_USER_REVEAL characters at each end plus the
 * GUAC_PROCTITLE_USER_MASK run (whose sizeof() includes its NUL terminator).
 */
#define GUAC_PROCTITLE_USER_MASKED_BUFSIZE \
    (2 * GUAC_PROCTITLE_USER_REVEAL + sizeof(GUAC_PROCTITLE_USER_MASK))

/**
 * The inclusive bounds of the printable ASCII range (space through tilde).
 * mask_username() treats any username byte outside this range as
 * non-printable and masks the username in full.
 */
#define ASCII_PRINTABLE_MIN 0x20
#define ASCII_PRINTABLE_MAX 0x7E

/**
 * The start of the writable argv/environ area used for process titles.
 */
static char* guac_proctitle_buffer = NULL;

/**
 * The number of bytes available within guac_proctitle_buffer.
 */
static size_t guac_proctitle_buffer_size = 0;

/**
 * Serializes access to the process-global proctitle state: the argv overlay
 * buffer and the /proc/<pid>/task/<tgid>/comm write. Static-initialized so
 * the first caller does not race against a missing pthread_mutex_init().
 */
static pthread_mutex_t guac_proctitle_lock = PTHREAD_MUTEX_INITIALIZER;

/**
 * Updates the main thread's short comm name from any thread in the same
 * process, without changing the calling thread's own comm. Writes through
 * /proc/<pid>/task/<tid>/comm; the main thread is identified by
 * TID == TGID == getpid(). This relies on the Linux procfs layout and is a
 * no-op on platforms that lack it.
 */
static void guac_main_thread_name_set(const char* name) {

#ifdef __linux__
    char comm_path[GUAC_PROCTITLE_COMM_PATH_LENGTH];
    char short_name[GUAC_PROCTITLE_COMM_LENGTH];
    FILE* comm_file;

    if (name == NULL)
        return;

    memset(short_name, '\0', sizeof(short_name));
    guac_strlcpy(short_name, name, sizeof(short_name));

    snprintf(comm_path, sizeof(comm_path), GUAC_PROCTITLE_COMM_PATH,
            (long) getpid());

    /* Write the new name to the main thread's comm pseudo-file. The kernel
     * accepts a short string (no trailing newline required) and truncates
     * to TASK_COMM_LEN. fopen() may fail in restricted environments where
     * /proc is unavailable or write access is denied; treat that as a
     * silent no-op since process naming is best-effort observability. */
    comm_file = fopen(comm_path, "w");
    if (comm_file == NULL)
        return;

    fputs(short_name, comm_file);
    fclose(comm_file);
#else
    (void) name;
#endif

}

void guac_process_title_init(int argc, char** argv) {

    char* buffer_end;
    char** copied_environ;
    int envc = 0;
    int i;

    if (argc <= 0 || argv == NULL || argv[0] == NULL)
        return;

    pthread_mutex_lock(&guac_proctitle_lock);

    /* Idempotent: a second init after the buffer is claimed is a no-op. */
    if (guac_proctitle_buffer != NULL) {
        pthread_mutex_unlock(&guac_proctitle_lock);
        return;
    }

    buffer_end = argv[argc - 1] + strlen(argv[argc - 1]) + 1;

    for (i = 0; environ != NULL && environ[i] != NULL; i++) {

        if (buffer_end == environ[i])
            buffer_end = environ[i] + strlen(environ[i]) + 1;

        envc++;

    }

    /* Copy environ to heap so it survives later overwrites of the argv area.
     * If any allocation fails, leave the original environ in place and bail
     * out without committing partial state. */
    copied_environ = guac_mem_alloc(sizeof(char*), envc + 1);
    if (copied_environ == NULL) {
        pthread_mutex_unlock(&guac_proctitle_lock);
        return;
    }

    for (i = 0; i < envc; i++) {

        copied_environ[i] = guac_strdup(environ[i]);
        if (copied_environ[i] == NULL) {
            while (i > 0)
                guac_mem_free(copied_environ[--i]);
            guac_mem_free(copied_environ);
            pthread_mutex_unlock(&guac_proctitle_lock);
            return;
        }

    }
    copied_environ[envc] = NULL;

    environ = copied_environ;
    guac_proctitle_buffer = argv[0];
    guac_proctitle_buffer_size = buffer_end - argv[0];

    pthread_mutex_unlock(&guac_proctitle_lock);

}

void guac_process_title_set(const char* title) {

    size_t title_length;

    if (title == NULL)
        return;

    pthread_mutex_lock(&guac_proctitle_lock);

    guac_main_thread_name_set(title);

    if (guac_proctitle_buffer == NULL || guac_proctitle_buffer_size == 0) {
        pthread_mutex_unlock(&guac_proctitle_lock);
        return;
    }

    title_length = strlen(title);
    if (title_length >= guac_proctitle_buffer_size)
        title_length = guac_proctitle_buffer_size - 1;

    memcpy(guac_proctitle_buffer, title, title_length);
    guac_proctitle_buffer[title_length] = '\0';

    if (title_length + 1 < guac_proctitle_buffer_size) {
        memset(guac_proctitle_buffer + title_length + 1, '\0',
                guac_proctitle_buffer_size - title_length - 1);
    }

    pthread_mutex_unlock(&guac_proctitle_lock);

}

/**
 * Writes a partially obfuscated form of the given username into the provided
 * buffer for inclusion in a process title. The goal is to retain enough of the
 * username for an operator to recognize a session at a glance while keeping the
 * full target account name out of world-readable process listings
 * (/proc/<pid>/cmdline).
 *
 * The algorithm is:
 *
 *   1. A NULL or empty username yields an empty string, so the caller omits
 *      the "user@" portion of the title entirely.
 *
 *   2. A username containing any byte outside the printable ASCII range
 *      (0x20-0x7E) is masked in full (GUAC_PROCTITLE_USER_MASK). This avoids
 *      splitting a multi-byte UTF-8 codepoint and emitting garbage.
 *
 *   3. A username shorter than GUAC_PROCTITLE_USER_MIN_REVEAL bytes is masked
 *      in full, since revealing GUAC_PROCTITLE_USER_REVEAL characters at each
 *      end would otherwise expose every (or overlapping) character (e.g.
 *      "root").
 *
 *   4. Otherwise the first and last GUAC_PROCTITLE_USER_REVEAL characters are
 *      preserved and the middle is replaced with the fixed-width run
 *      GUAC_PROCTITLE_USER_MASK, e.g. "bbennett" -> "bb****tt". The run width
 *      is constant regardless of username length so the result does not leak
 *      the original length.
 *
 * @param user
 *     The username to obfuscate, which may be NULL.
 *
 * @param out
 *     The buffer to receive the NUL-terminated, obfuscated username. Should be
 *     at least GUAC_PROCTITLE_USER_MASKED_BUFSIZE bytes.
 *
 * @param out_size
 *     The size of the out buffer, in bytes.
 */
static void mask_username(const char* user, char* out, size_t out_size) {

    if (user == NULL || *user == '\0') {
        out[0] = '\0';
        return;
    }

    size_t length = strlen(user);

    /* Reveal the prefix & suffix edges only for sufficiently long, plain
     * printable-ASCII usernames; otherwise mask the entire value. */
    int reveal = (length >= GUAC_PROCTITLE_USER_MIN_REVEAL);
    for (size_t i = 0; reveal && i < length; i++) {
        unsigned char c = (unsigned char) user[i];
        if (c < ASCII_PRINTABLE_MIN || c > ASCII_PRINTABLE_MAX)
            reveal = 0;
    }

    if (!reveal) {
        guac_strlcpy(out, GUAC_PROCTITLE_USER_MASK, out_size);
        return;
    }

    /* Preserve the first and last GUAC_PROCTITLE_USER_REVEAL characters,
     * replacing everything between them with the fixed mask. */
    char prefix[GUAC_PROCTITLE_USER_REVEAL + 1];
    memcpy(prefix, user, GUAC_PROCTITLE_USER_REVEAL);
    prefix[GUAC_PROCTITLE_USER_REVEAL] = '\0';

    const char* suffix = user + length - GUAC_PROCTITLE_USER_REVEAL;

    snprintf(out, out_size, "%s%s%s", prefix, GUAC_PROCTITLE_USER_MASK, suffix);

}

void guac_process_title_set_endpoint(const char* protocol, const char* user,
        const char* host, const char* port) {

    char title[GUAC_PROCESS_TITLE_BUFSIZE];
    char masked_user[GUAC_PROCTITLE_USER_MASKED_BUFSIZE];

    if (protocol == NULL)
        return;

    /* Fall back to a placeholder host; omit the user and port portions when
     * not provided. */
    if (host == NULL || *host == '\0')
        host = "unknown-host";

    /* The username is partially masked before display: it is target account
     * metadata exposed in world-readable process listings. */
    mask_username(user, masked_user, sizeof(masked_user));

    int has_user = (masked_user[0] != '\0');
    int has_port = (port != NULL && *port != '\0');

    if (has_user && has_port)
        snprintf(title, sizeof(title), "%s %s@%s:%s", protocol, masked_user, host, port);
    else if (has_user)
        snprintf(title, sizeof(title), "%s %s@%s", protocol, masked_user, host);
    else if (has_port)
        snprintf(title, sizeof(title), "%s %s:%s", protocol, host, port);
    else
        snprintf(title, sizeof(title), "%s %s", protocol, host);

    guac_process_title_set(title);

}

void guac_thread_name_set(const char* name) {

#ifdef HAVE_PRCTL
    char short_name[GUAC_PROCTITLE_COMM_LENGTH];

    if (name == NULL)
        return;

    /* guac_strlcpy() properly terminates short_name[]. */
    guac_strlcpy(short_name, name, sizeof(short_name));
    prctl(PR_SET_NAME, short_name, 0, 0, 0);
#else
    (void) name;
#endif

}
