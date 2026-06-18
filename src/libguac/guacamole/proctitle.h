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

#ifndef _GUAC_PROCTITLE_H
#define _GUAC_PROCTITLE_H

/**
 * @file proctitle.h
 *
 * These functions allow guacd and its per-connection child processes to
 * show meaningful process and thread names in tools such as `ps`, `top`,
 * `htop`, and `gdb`. Process titles can be updated at runtime to identify
 * the active connection (for example, `vnc user@example.com:5900`), while
 * worker threads can be assigned short descriptive names (for example,
 * `display-wrk` or `user-input`).
 *
 * Process titles are visible through normal process-listing interfaces and
 * should be treated as local-observable metadata. Callers must not include
 * passwords, tokens, keys, or other secrets.
 *
 * Process title support operates on a single argv/environ memory region
 * captured once at process startup by guac_process_title_init(). The captured
 * buffer address and size are stored as process-wide static state and
 * reused by all subsequent title updates. Writes are serialized with a
 * static mutex so callers can invoke from any thread.
 *
 * Linux exposes argv+environ as a contiguous block whose contents are
 * returned through `/proc/<pid>/cmdline`. Process title updates work by:
 *   - Capturing that block's address and length at startup.
 *   - Moving `environ` to a heap copy so `getenv()` continues to work
 *     after the original storage is overwritten.
 *   - Reusing the captured block as a writable title buffer, NUL-padding
 *     any unused bytes.
 *
 * Thread names are exposed through Linux's thread `comm` mechanism:
 *   - `prctl(PR_SET_NAME)` updates the calling thread's name
 *     (15 characters plus NUL). Used by guac_thread_name_set().
 *
 *   - Writing `/proc/self/task/<tid>/comm` updates the name of any
 *     thread in the process. guac_process_title_set() uses this to update
 *     the main thread so process-oriented tools such as `top` and
 *     `ps -e` display the active connection.
 *
 * --- When to call ---
 *
 *   guac_process_title_init:
 *       Call once from main() before creating threads and before any
 *       code modifies `environ`.
 *
 *   guac_process_title_set:
 *       Call any time after initialization. Typically used once per
 *       child process after the connection type and target are known.
 *
 *   guac_thread_name_set:
 *       Call at thread startup to assign a descriptive worker name.
 */

/**
 * Recommended buffer size for formatting a process title before passing it
 * to guac_process_title_set(). Fits a protocol name, user, host, and port in
 * typical deployments.
 */
#define GUAC_PROCESS_TITLE_BUFSIZE 256

/**
 * Initializes process title support by capturing the writable argv/environ
 * region and moving `environ` to the heap. Must be called from main()
 * before any other thread starts and before anything else reuses argv or
 * environ. Safe to call more than once (subsequent calls no-op) and safe
 * with bad args (no-op).
 *
 * If the heap environ copy fails to allocate, leaves environ in place and
 * disables cmdline updates: later guac_process_title_set() calls still update
 * the main-thread comm.
 *
 * @param argc
 *     The argument count as passed to main().
 *
 * @param argv
 *     The argument vector as passed to main().
 */
void guac_process_title_init(int argc, char** argv);

/**
 * Updates the process title: both /proc/<pid>/cmdline (the COMMAND column
 * in `ps`/`htop`) and the *main thread's* short comm (visible in `top`,
 * `ps -e`). The calling thread's own comm is left alone: use
 * guac_thread_name_set() for that.
 *
 * If guac_process_title_init() was not called successfully, the cmdline part
 * is skipped and only the comm write is attempted.
 *
 * @param title
 *     The new title. Truncated to fit the captured argv region (cmdline)
 *     and to 15 chars for the main thread comm. NULL no-ops.
 */
void guac_process_title_set(const char* title);

/**
 * Convenience wrapper around guac_process_title_set() that formats a network
 * connection title in the form "<protocol> [user@]host[:port]". Falls back to
 * "unknown-host" when host is NULL or empty, and omits the "user@" or ":port"
 * portion when the respective argument is NULL or empty. The port is taken as
 * a string so callers with either a numeric or named port can use it; numeric
 * ports should be converted with guac_itoa_safe() first.
 *
 * The username is partially masked before display (e.g. "bbennett" becomes
 * "bb****tt") so the full target account name is not exposed in world-readable
 * process listings. Short or non-printable-ASCII usernames are masked in full.
 * This is obfuscation to reduce casual disclosure, not an access control.
 *
 * @param protocol
 *     Short protocol label, e.g. "vnc", "rdp", "ssh". A NULL value no-ops.
 *
 * @param user
 *     The connecting username, or NULL/empty to omit the "user@" portion. The
 *     value is partially masked before it appears in the process title.
 *
 * @param host
 *     The target hostname, or NULL/empty to substitute "unknown-host".
 *
 * @param port
 *     The target port as a string, or NULL/empty to omit the ":port" portion.
 */
void guac_process_title_set_endpoint(const char* protocol, const char* user,
        const char* host, const char* port);

/**
 * Sets the *calling* thread's short name (visible in
 * /proc/<pid>/task/<tid>/comm, `top -H`, `ps -L`, ...) 
 * via prctl(PR_SET_NAME).
 *
 * Thread-safe: the underlying prctl targets only the calling thread.
 *
 * Note: threads spawned later by the calling thread inherit
 * this name as their initial comm. If you call into a library
 * that creates its own workers, those workers will appear with
 * the parent's name until they rename themselves.
 *
 * Each call site is preceded by a comment of the form
 * "Thread name <name>: <description>". To list every named thread
 * and what it does, search the source. The description may wrap onto
 * the following line, so include one line of trailing context:
 *
 *     grep -rn -A1 "Thread name " src/
 *
 * @param name
 *     The new thread name. Truncated to 15 chars + NULL.
 */
void guac_thread_name_set(const char* name);

#endif
