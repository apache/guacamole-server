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

#include "guacamole/error.h"
#include "guacamole/mem.h"
#include "guacamole/thread.h"

#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef HAVE_PRCTL
#include <sys/prctl.h>
#endif

/**
 * The size of the buffer used to hold a thread's name, in bytes, including null
 * terminator. Names are truncated to fit this buffer before being assigned.
 *
 * @note
 *     Where not specified by a platform-specific constant like
 *     PTHREAD_MAX_NAMELEN_NP, this limit varies. Here, 16 is used as a fallback
 *     as this is both the most common limit and the strictest bound (no
 *     platform has a thread name length limit smaller than this).
 */
#ifdef PTHREAD_MAX_NAMELEN_NP
#define GUAC_THREAD_NAME_LENGTH PTHREAD_MAX_NAMELEN_NP
#else
#define GUAC_THREAD_NAME_LENGTH 16
#endif

/**
 * Parameters describing a thread being created.
 */
typedef struct guac_thread_params {

    /**
     * The thread routine to run.
     */
    guac_thread_routine* routine;

    /**
     * The arbitrary data to pass to the thread routine.
     */
    void* data;

    /**
     * The name that should be assigned to the new thread.
     */
    char name[GUAC_THREAD_NAME_LENGTH];

} guac_thread_params;

/**
 * Wrapper for the thread routine of a guac_thread. This wrapper assigns the
 * requested thread name prior to starting the thread.
 *
 * @param data
 *     The guac_thread_params describing the thread being created.
 *
 * @return
 *     The value returned by the thread routine within the provided
 *     guac_thread_params.
 */
static void* guac_thread_run(void* data) {

    guac_thread_params params = *((guac_thread_params*) data);
    guac_mem_free(data);

    guac_thread_self_set_name("%s", params.name);

    return params.routine(params.data);

}

void guac_thread_init(guac_thread* thread) {
    memset(thread, 0, sizeof(guac_thread));
}

int guac_thread_create(guac_thread* thread, guac_thread_routine* routine,
        void* data, const char* name_format, ...) {

    guac_thread_init(thread);

    guac_thread_params* params = guac_mem_alloc(sizeof(guac_thread_params));
    if (params == NULL) {
        guac_error = GUAC_STATUS_NO_MEMORY;
        guac_error_message = "Could not allocate memory for thread";
        return 1;
    }

    params->routine = routine;
    params->data = data;

    va_list args;
    va_start(args, name_format);
    vsnprintf(params->name, sizeof(params->name), name_format, args);
    va_end(args);

    int result = pthread_create(&thread->thread, NULL, guac_thread_run, params);
    if (result) {
        guac_mem_free(params);
        errno = result;
        guac_error = GUAC_STATUS_SEE_ERRNO;
        guac_error_message = "Could not create thread";
        return 1;
    }

    thread->started = 1;
    return 0;

}

int guac_thread_join(guac_thread* thread, void** retval) {

    void* result;
    int status = thread->started ? pthread_join(thread->thread, &result) : EINVAL;

    /* Thread is already joined */
    if (status == EINVAL) {
        guac_error = GUAC_STATUS_NOT_FOUND;
        guac_error_message = "No started thread to join";
        return 1;
    }

    /* Deadlock or (somehow) an invalid underlying thread */
    if (status) {
        errno = status;
        guac_error = GUAC_STATUS_SEE_ERRNO;
        guac_error_message = "Could not join thread";
        return 1;
    }

    /* Thread was successfully joined */
    thread->started = 0;

    /* Report lack of return value if canceled */
    if (result == PTHREAD_CANCELED) {
        guac_error = GUAC_STATUS_CANCELED;
        guac_error_message = "Thread was canceled before it could return";
        return 1;
    }

    if (retval != NULL)
        *retval = result;

    return 0;

}

void guac_thread_cancel(guac_thread* thread) {

    if (!thread->started)
        return;

    pthread_cancel(thread->thread);

}

void guac_thread_detach(guac_thread* thread) {

    if (!thread->started)
        return;

    pthread_detach(thread->thread);
    thread->started = 0;

}

void guac_thread_self_set_name(const char* name_format, ...) {

    char name[GUAC_THREAD_NAME_LENGTH];

    va_list args;
    va_start(args, name_format);
    vsnprintf(name, sizeof(name), name_format, args);
    va_end(args);

#ifdef HAVE_PTHREAD_SETNAME_NP_THREAD_FORMAT
    pthread_setname_np(pthread_self(), "%s", name);
#elif defined(HAVE_PTHREAD_SETNAME_NP_THREAD_NAME)
    pthread_setname_np(pthread_self(), name);
#elif defined(HAVE_PTHREAD_SETNAME_NP_NAME)
    pthread_setname_np(name);
#elif defined(HAVE_PRCTL)
    prctl(PR_SET_NAME, name, 0, 0, 0);
#endif

}
