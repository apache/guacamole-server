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

#ifndef GUAC_THREAD_H
#define GUAC_THREAD_H

/**
 * Provides a named thread implementation (guac_thread) which automatically
 * tracks whether the thread has been started.
 *
 * @file thread.h
 */

#include "thread-fntypes.h"
#include "thread-types.h"

#include <pthread.h>

struct guac_thread {

    /**
     * The underlying POSIX thread. This value is only meaningful if started
     * is non-zero.
     */
    pthread_t thread;

    /**
     * Non-zero if the thread has been started and has not yet been joined or
     * detached, zero otherwise.
     */
    int started;

};

/**
 * Initializes the given guac_thread such that it may be safely joined or
 * detached, even if not yet started.
 *
 * @note
 *     Zeroing the memory of a guac_thread is equivalent to calling this
 *     function.
 *
 * @param thread
 *     The guac_thread to initialize.
 */
void guac_thread_init(guac_thread* thread);

/**
 * Initializes and starts a new thread which runs the given routine, assigning
 * the new thread the given name. On platforms where thread names are supported,
 * this name will be visible in process listings that show running threads.
 *
 * Names longer than 15 characters may be truncated, depending on whether the
 * local platform supports longer thread names. Callers should avoid using names
 * that are longer than 15 characters.
 *
 * If the thread cannot be started, guac_error is set appropriately. The given
 * guac_thread is still initialized and may be safely joined or detached.
 *
 * @ref guac_error            | Meaning
 * -------------------------- | -------
 * @ref GUAC_STATUS_NO_MEMORY | Memory for the underlying thread could not be allocated.
 * @ref GUAC_STATUS_SEE_ERRNO | An internal failure prevented the underlying thread from being created.
 *
 * @param thread
 *     The guac_thread to initialize and start. Any previous state of this
 *     structure is overwritten. If reusing a guac_thread, it MUST be joined or
 *     detached first.
 *
 * @param routine
 *     The function to run within the new thread.
 *
 * @param data
 *     Arbitrary data to pass to the given routine.
 *
 * @param name_format
 *     A printf-style format string producing the name of the new thread.
 *
 * @param ...
 *     Any arguments required by the format string.
 *
 * @return
 *     Zero if the thread was started successfully, non-zero otherwise.
 */
int guac_thread_create(guac_thread* thread, guac_thread_routine* routine,
        void* data, const char* name_format, ...);

/**
 * Assigns or updates the name of the calling thread to the given name. The
 * calling thread need not be a guac_thread. On platforms where thread names are
 * supported, this name will be visible in process listings that show running
 * threads.
 *
 * Names longer than 15 characters may be truncated, depending on whether the
 * local platform supports longer thread names. Callers should avoid using names
 * that are longer than 15 characters.
 *
 * @note
 *     Threads started with guac_thread_create() are named during startup and
 *     need not be manually named again with this function unless the name needs
 *     to change.
 *
 * @param name_format
 *     A printf-style format string producing the name of the calling thread.
 *
 * @param ...
 *     Any arguments required by the format string.
 */
void guac_thread_self_set_name(const char* name_format, ...);

/**
 * Waits for the given thread to terminate and frees any underlying resources.
 * All threads must eventually be explicitly joined unless they have been
 * detached with guac_thread_detach(), including canceled threads. If provided,
 * the value returned by the thread's routine will be stored in retval. If the
 * thread's routine could not finish because the thread was canceled, non-zero
 * is returned, and guac_error is set to GUAC_STATUS_CANCELED. If the thread has
 * not been started, has already been joined, or has been detached, then the
 * thread cannot be joined, non-zero is returned, and guac_error is set
 * appropriately.
 *
 * @note
 *     Behavior is undefined if multiple threads invoke `guac_thread_join()`
 *     concurrently on the same thread.
 *
 * @ref guac_error            | Meaning
 * -------------------------- | -------
 * @ref GUAC_STATUS_CANCELED  | The thread has been successfully joined but was at some point canceled with @ref guac_thread_cancel() and did not finish.
 * @ref GUAC_STATUS_NOT_FOUND | The thread was not started, has already been joined, or has been detached.
 * @ref GUAC_STATUS_SEE_ERRNO | The system detected a deadlock condition or an invalid/corrupt underlying thread.
 *
 * @param thread
 *     The guac_thread to wait for.
 *
 * @param retval
 *     A pointer to the location that should receive the value returned by the
 *     thread's routine, or NULL to discard the return value. If the join
 *     operation fails, or the thread was canceled before being joined, this
 *     location is left untouched.
 *
 * @return
 *     Zero if the thread was successfully joined and was not canceled, non-zero
 *     otherwise.
 */
int guac_thread_join(guac_thread* thread, void** retval);

/**
 * Requests cancellation of the given thread, causing the thread to terminate
 * once it reaches an interruptible state (cancellation point). This function is
 * not guaranteed to terminate the thread unless it reaches such a state. The
 * thread must still be joined or detached. If the thread has not been started,
 * or has already been joined or detached, this function has no effect.
 *
 * @param thread
 *     The guac_thread to cancel.
 */
void guac_thread_cancel(guac_thread* thread);

/**
 * Detaches the given thread, such that its resources are automatically
 * released after the thread is no longer running. If the thread has not been
 * started, or has already been joined or detached, this function has no effect.
 * A detached thread cannot be joined.
 *
 * @param thread
 *     The guac_thread to detach.
 */
void guac_thread_detach(guac_thread* thread);

#endif
