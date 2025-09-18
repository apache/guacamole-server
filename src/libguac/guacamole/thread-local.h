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

#ifndef GUAC_THREAD_LOCAL_H
#define GUAC_THREAD_LOCAL_H

#include <stdint.h>

/**
 * Thread-local storage abstraction that provides pthread_key_t compatible
 * interface but uses more efficient implementations where possible.
 * 
 * This provides two different implementations:
 * 1. Direct __thread storage for simple cases (like error handling)
 * 2. Hash-table based storage for complex cases (like rwlock with process sharing)
 */

/**
 * Type representing a thread-local key, compatible with pthread_key_t usage patterns.
 */
typedef uintptr_t guac_thread_local_key_t;

/**
 * Destructor function type for thread-local storage cleanup.
 */
typedef void (*guac_thread_local_destructor_t)(void*);

/**
 * Once control structure for one-time initialization.
 */
typedef struct guac_thread_local_once_t {
    volatile int done;
    int mutex_init;
    void* mutex_data;
} guac_thread_local_once_t;

/**
 * Initializer for guac_thread_local_once_t structures.
 */
#define GUAC_THREAD_LOCAL_ONCE_INIT {0, 0, NULL}

/**
 * Create a new thread-local storage key.
 *
 * @param key
 *     Pointer to store the created key.
 *
 * @param destructor
 *     Function to call when thread exits to clean up the stored value.
 *     May be NULL if no cleanup is needed.
 *
 * @return
 *     Zero on success, non-zero on error.
 */
int guac_thread_local_key_create(guac_thread_local_key_t* key, guac_thread_local_destructor_t destructor);

/**
 * Delete a thread-local storage key.
 *
 * @param key
 *     The key to delete.
 *
 * @return
 *     Zero on success, non-zero on error.
 */
int guac_thread_local_key_delete(guac_thread_local_key_t key);

/**
 * Set thread-specific data for the given key.
 *
 * @param key
 *     The thread-local storage key.
 *
 * @param value
 *     The value to store.
 *
 * @return
 *     Zero on success, non-zero on error.
 */
int guac_thread_local_setspecific(guac_thread_local_key_t key, const void* value);

/**
 * Get thread-specific data for the given key.
 *
 * @param key
 *     The thread-local storage key.
 *
 * @return
 *     The stored value, or NULL if no value has been set.
 */
void* guac_thread_local_getspecific(guac_thread_local_key_t key);

/**
 * Ensure that a function is called exactly once across all threads.
 *
 * @param once_control
 *     Control structure for tracking initialization state.
 *
 * @param init_routine
 *     Function to call exactly once.
 *
 * @return
 *     Zero on success, non-zero on error.
 */
int guac_thread_local_once(guac_thread_local_once_t* once_control, void (*init_routine)(void));

#endif /* GUAC_THREAD_LOCAL_H */