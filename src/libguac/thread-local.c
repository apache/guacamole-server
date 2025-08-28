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

#include "config.h"
#include "guacamole/thread-local.h"

#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <sched.h>

/**
 * Maximum number of thread-local keys supported.
 */
#define MAX_THREAD_KEYS 1024

/**
 * Structure to hold destructor information for each key.
 */
typedef struct guac_key_entry {
    guac_thread_local_destructor_t destructor;
    int in_use;
} guac_key_entry_t;

/**
 * Thread-local storage entry.
 */
typedef struct guac_thread_storage {
    void* values[MAX_THREAD_KEYS];
} guac_thread_storage_t;

/**
 * Global key registry protected by mutex.
 */
static guac_key_entry_t key_registry[MAX_THREAD_KEYS];
static pthread_mutex_t key_registry_mutex = PTHREAD_MUTEX_INITIALIZER;
static uintptr_t next_key_id = 1;

/**
 * Thread-local storage using __thread keyword for better performance.
 */
static __thread guac_thread_storage_t* thread_storage = NULL;

/**
 * Global cleanup key created once for all threads.
 */
static pthread_key_t global_cleanup_key;
static pthread_once_t cleanup_key_once = PTHREAD_ONCE_INIT;

/**
 * Cleanup function called when thread exits.
 */
static void cleanup_thread_storage(void* storage) {
    guac_thread_storage_t* ts = (guac_thread_storage_t*)storage;
    if (ts == NULL) return;

    pthread_mutex_lock(&key_registry_mutex);
    
    for (int i = 0; i < MAX_THREAD_KEYS; i++) {
        if (key_registry[i].in_use && ts->values[i] != NULL) {
            if (key_registry[i].destructor) {
                key_registry[i].destructor(ts->values[i]);
            }
        }
    }
    
    pthread_mutex_unlock(&key_registry_mutex);
    free(ts);
}

/**
 * Initialize the global cleanup key once.
 */
static void init_cleanup_key(void) {
    pthread_key_create(&global_cleanup_key, cleanup_thread_storage);
}

/**
 * Initialize thread storage if not already done.
 */
static guac_thread_storage_t* get_thread_storage(void) {
    if (thread_storage == NULL) {
        thread_storage = calloc(1, sizeof(guac_thread_storage_t));
        if (thread_storage != NULL) {
            pthread_once(&cleanup_key_once, init_cleanup_key);
            if (pthread_setspecific(global_cleanup_key, thread_storage) != 0) {
                /* Critical: if pthread_setspecific fails, we must not leave 
                 * the thread_storage pointer set, as cleanup won't be called */
                free(thread_storage);
                thread_storage = NULL;
                return NULL;
            }
        }
    }
    return thread_storage;
}

int guac_thread_local_key_create(guac_thread_local_key_t* key, guac_thread_local_destructor_t destructor) {
    if (key == NULL) return EINVAL;

    pthread_mutex_lock(&key_registry_mutex);
    
    uintptr_t key_id = 0;
    for (int i = 0; i < MAX_THREAD_KEYS; i++) {
        if (!key_registry[i].in_use) {
            key_id = next_key_id++;
            if (next_key_id == 0) next_key_id = 1; // Avoid key_id of 0
            
            key_registry[i].destructor = destructor;
            key_registry[i].in_use = 1;
            *key = (key_id << 16) | i; // Combine ID and index for validation
            break;
        }
    }
    
    pthread_mutex_unlock(&key_registry_mutex);
    
    return (key_id == 0) ? EAGAIN : 0;
}

int guac_thread_local_key_delete(guac_thread_local_key_t key) {
    int index = key & 0xFFFF;
    if (index >= MAX_THREAD_KEYS) return EINVAL;

    pthread_mutex_lock(&key_registry_mutex);
    
    if (key_registry[index].in_use) {
        key_registry[index].in_use = 0;
        key_registry[index].destructor = NULL;
    }
    
    pthread_mutex_unlock(&key_registry_mutex);
    return 0;
}

int guac_thread_local_setspecific(guac_thread_local_key_t key, const void* value) {
    int index = key & 0xFFFF;
    if (index >= MAX_THREAD_KEYS) return EINVAL;

    guac_thread_storage_t* storage = get_thread_storage();
    if (storage == NULL) return ENOMEM;

    pthread_mutex_lock(&key_registry_mutex);
    /* Hold lock during the entire operation to prevent TOCTOU races */
    if (!key_registry[index].in_use) {
        pthread_mutex_unlock(&key_registry_mutex);
        return EINVAL;
    }
    
    storage->values[index] = (void*)value;
    pthread_mutex_unlock(&key_registry_mutex);
    return 0;
}

void* guac_thread_local_getspecific(guac_thread_local_key_t key) {
    int index = key & 0xFFFF;
    if (index >= MAX_THREAD_KEYS) return NULL;

    guac_thread_storage_t* storage = get_thread_storage();
    if (storage == NULL) return NULL;

    pthread_mutex_lock(&key_registry_mutex);
    /* Hold lock during the entire operation to prevent TOCTOU races */
    if (!key_registry[index].in_use) {
        pthread_mutex_unlock(&key_registry_mutex);
        return NULL;
    }
    
    void* value = storage->values[index];
    pthread_mutex_unlock(&key_registry_mutex);
    return value;
}

int guac_thread_local_once(guac_thread_local_once_t* once_control, void (*init_routine)(void)) {
    if (once_control == NULL || init_routine == NULL) return EINVAL;

    if (__sync_bool_compare_and_swap(&once_control->done, 0, 1)) {
        init_routine();
        return 0;
    }
    
    while (!once_control->done) {
        sched_yield();
    }
    
    return 0;
}