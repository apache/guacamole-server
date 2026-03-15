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

#include <limits.h>
#include <pthread.h>
#include <stdint.h>

#include "guacamole/error.h"
#include "guacamole/mem.h"
#include "guacamole/rwlock.h"

/**
 * The value indicating that the current thread holds neither the read or write
 * locks.
 */
#define GUAC_REENTRANT_LOCK_NO_LOCK 0

/**
 * The value indicating that the current thread holds the read lock.
 */
#define GUAC_REENTRANT_LOCK_READ_LOCK 1

/**
 * The value indicating that the current thread holds the write lock.
 */
#define GUAC_REENTRANT_LOCK_WRITE_LOCK 2

/**
 * The maximum number of distinct guac_rwlock instances a single thread may
 * hold simultaneously.
 */
#define GUAC_RWLOCK_MAX_HELD 16

/**
 * Per-lock state tracked for a single thread. One slot is allocated per
 * distinct lock that the thread currently holds.
 */
typedef struct guac_rwlock_thread_state {

    /**
     * The lock this slot describes. NULL indicates the slot is empty.
     */
    guac_rwlock* lock;

    /**
     * Which lock the current thread holds: GUAC_REENTRANT_LOCK_NO_LOCK,
     * GUAC_REENTRANT_LOCK_READ_LOCK, or GUAC_REENTRANT_LOCK_WRITE_LOCK.
     */
    int flag;

    /**
     * The reentrant depth, representing the number of times the current thread
     * has acquired this lock without a corresponding release.
     */
    unsigned int count;

} guac_rwlock_thread_state;

/**
 * A single process wide key whose per-thread value is a pointer to that
 * thread's array of guac_rwlock_thread_state entries. Created exactly once
 * via pthread_once.
 */
static pthread_key_t guac_rwlock_key;
static pthread_once_t guac_rwlock_key_init = PTHREAD_ONCE_INIT;

/**
 * Destructor registered with pthread_key_create that frees the per-thread
 * guac_rwlock_thread_state array when a thread exits.
 *
 * @param pointer
 *     Pointer to the per-thread state array to free.
 */
static void guac_rwlock_free_pointer(void* pointer) {

    guac_mem_free(pointer);

}

/**
 * Creates the single global thread-local key. Invoked exactly once via
 * pthread_once.
 */
static void guac_rwlock_create_key(void) {

    pthread_key_create(&guac_rwlock_key, guac_rwlock_free_pointer);

}

/**
 * Returns the per-thread state array, allocating and registering it on first
 * use.
 *
 * @return
 *     A pointer to the calling thread's guac_rwlock_thread_state array, or
 *     NULL if allocation fails.
 */
static guac_rwlock_thread_state* guac_rwlock_get_thread_states(void) {

    pthread_once(&guac_rwlock_key_init, guac_rwlock_create_key);

    guac_rwlock_thread_state* states = pthread_getspecific(guac_rwlock_key);
    if (states == NULL) {
        states = guac_mem_zalloc(sizeof(guac_rwlock_thread_state), GUAC_RWLOCK_MAX_HELD);
        pthread_setspecific(guac_rwlock_key, states);
    }

    return states;

}

/**
 * Returns the state slot for the given lock for the current thread, or NULL
 * if the current thread does not hold that lock.
 *
 * @param lock
 *     The lock whose state slot should be retrieved.
 *
 * @return
 *     The state slot for the given lock, or NULL if the current thread does
 *     not hold that lock.
 */
static guac_rwlock_thread_state* guac_rwlock_state_get(guac_rwlock* lock) {

    guac_rwlock_thread_state* states = guac_rwlock_get_thread_states();

    for (int i = 0; i < GUAC_RWLOCK_MAX_HELD; i++) {
        if (states[i].lock == lock)
            return &states[i];
    }

    return NULL;

}

/**
 * Returns the state slot for the given lock for the current thread, creating
 * and initializing a new slot if one does not already exist. Returns NULL if
 * all slots are in use.
 *
 * @param lock
 *     The lock whose state slot should be retrieved or created.
 *
 * @return
 *     The state slot for the given lock, or NULL if all slots are in use.
 */
static guac_rwlock_thread_state* guac_rwlock_state_get_or_create(guac_rwlock* lock) {

    guac_rwlock_thread_state* states = guac_rwlock_get_thread_states();

    guac_rwlock_thread_state* empty = NULL;
    for (int i = 0; i < GUAC_RWLOCK_MAX_HELD; i++) {
        if (states[i].lock == lock)
            return &states[i];
        if (empty == NULL && states[i].lock == NULL)
            empty = &states[i];
    }

    if (empty != NULL) {
        empty->lock = lock;
        empty->flag = GUAC_REENTRANT_LOCK_NO_LOCK;
        empty->count = 0;
    }

    return empty;

}

/**
 * Clears a state slot, marking it as empty so it can be reused.
 *
 * @param state
 *     The state slot to clear.
 */
static void guac_rwlock_state_clear(guac_rwlock_thread_state* state) {
    state->lock = NULL;
    state->flag = GUAC_REENTRANT_LOCK_NO_LOCK;
    state->count = 0;
}

void guac_rwlock_init(guac_rwlock* lock) {

    /* Configure to allow sharing this lock with child processes */
    pthread_rwlockattr_t lock_attributes;
    pthread_rwlockattr_init(&lock_attributes);
    pthread_rwlockattr_setpshared(&lock_attributes, PTHREAD_PROCESS_SHARED);

    /* Initialize the rwlock */
    pthread_rwlock_init(lock, &lock_attributes);
    pthread_rwlockattr_destroy(&lock_attributes);

}

void guac_rwlock_destroy(guac_rwlock* lock) {

    /* Destroy the rwlock */
    pthread_rwlock_destroy(lock);

}

int guac_rwlock_acquire_write_lock(guac_rwlock* reentrant_rwlock) {

    guac_rwlock_thread_state* state = guac_rwlock_state_get_or_create(reentrant_rwlock);

    if (state == NULL) {
        guac_error = GUAC_STATUS_TOO_MANY;
        guac_error_message = "Unable to acquire write lock because there's"
                " too many locks held simultaneously by this thread";
        return 1;
    }

    /* If acquiring this lock again would overflow the counter storage */
    if (state->count >= UINT_MAX) {
        guac_error = GUAC_STATUS_TOO_MANY;
        guac_error_message = "Unable to acquire write lock because there's"
                " insufficient space to store another level of lock depth";

        return 1;

    }

    /* If the current thread already holds the write lock, increment the count */
    if (state->flag == GUAC_REENTRANT_LOCK_WRITE_LOCK) {
        state->count++;

        /* This thread already has the lock */
        return 0;
    }

    /*
     * The read lock must be released before the write lock can be acquired.
     * This is a little odd because it may mean that a function further down
     * the stack may have requested a read lock, which will get upgraded to a
     * write lock by another function without the caller knowing about it. This
     * shouldn't cause any issues, however.
     */
    if (state->flag == GUAC_REENTRANT_LOCK_READ_LOCK) {
        int unlock_err = pthread_rwlock_unlock(reentrant_rwlock);
        if (unlock_err) {
            guac_error = GUAC_STATUS_SEE_ERRNO;
            guac_error_message = "Unable to release read lock for write lock upgrade";
            return 1;
        }
    }

    /* Acquire the write lock */
    int err = pthread_rwlock_wrlock(reentrant_rwlock);
    if (err) {

        /* The read lock was released above but the write lock was not acquired,
         * so the current thread no longer holds any lock */
        if (state->flag == GUAC_REENTRANT_LOCK_READ_LOCK)
            guac_rwlock_state_clear(state);

        guac_error = GUAC_STATUS_SEE_ERRNO;
        guac_error_message = "Unable to acquire write lock";
        return 1;

    }

    state->flag = GUAC_REENTRANT_LOCK_WRITE_LOCK;
    state->count++;

    return 0;

}

int guac_rwlock_acquire_read_lock(guac_rwlock* reentrant_rwlock) {

    guac_rwlock_thread_state* state = guac_rwlock_state_get_or_create(reentrant_rwlock);

    if (state == NULL) {

        guac_error = GUAC_STATUS_TOO_MANY;
        guac_error_message = "Unable to acquire read lock because there's"
                " too many locks held simultaneously by this thread";

        return 1;

    }

    /* If acquiring this lock again would overflow the counter storage */
    if (state->count >= UINT_MAX) {

        guac_error = GUAC_STATUS_TOO_MANY;
        guac_error_message = "Unable to acquire read lock because there's"
                " insufficient space to store another level of lock depth";

        return 1;

    }

    /* The current thread may read if either the read or write lock is held */
    if (
            state->flag == GUAC_REENTRANT_LOCK_READ_LOCK ||
            state->flag == GUAC_REENTRANT_LOCK_WRITE_LOCK
    ) {

        /* Increment the depth counter */
        state->count++;

        /* This thread already has the lock */
        return 0;
    }

    /* Acquire the lock */
    int err = pthread_rwlock_rdlock(reentrant_rwlock);
    if (err) {
        guac_error = GUAC_STATUS_SEE_ERRNO;
        guac_error_message = "Unable to acquire read lock";
        return 1;
    }

    /* Set the flag that the current thread has the read lock */
    state->flag  = GUAC_REENTRANT_LOCK_READ_LOCK;
    state->count = 1;

    return 0;

}

int guac_rwlock_release_lock(guac_rwlock* reentrant_rwlock) {

    guac_rwlock_thread_state* state = guac_rwlock_state_get(reentrant_rwlock);

    /*
     * Return an error if an attempt is made to release a lock that the current
     * thread does not control.
     */
    if (state == NULL || state->count == 0) {

        guac_error = GUAC_STATUS_INVALID_ARGUMENT;
        guac_error_message = "Unable to free rwlock because it's not held by"
                " the current thread";

        return 1;

    }

    /* Release the lock if this is the last locked level */
    if (state->count == 1) {

        pthread_rwlock_unlock(reentrant_rwlock);

        /* Set the flag that the current thread holds no locks */
        guac_rwlock_state_clear(state);

        return 0;
    }

    /* Do not release the lock since it's still in use - just decrement */
    state->count--;

    return 0;

}
