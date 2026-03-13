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

#ifndef __GUAC_RWLOCK_H
#define __GUAC_RWLOCK_H

#include <pthread.h>

/**
 * This file implements reentrant read-write locks using thread-local storage
 * to keep track of how locks are held and released by the current thread,
 * since the pthread locks do not support reentrant behavior.
 *
 * A thread will attempt to acquire the requested lock on the first acquire
 * function call, and will release it once the number of unlock requests
 * matches the number of lock requests. Therefore, it is safe to acquire a lock
 * and then call a function that also acquires the same lock, provided that
 * the caller and the callee request to unlock the lock when done with it.
 *
 * Any lock that's locked using one of the functions defined in this file
 * must _only_ be unlocked using the unlock function defined here to avoid
 * unexpected behavior.
 */

/**
 * A reentrant read-write lock. Callers must use only the guac_rwlock_*
 * functions to acquire and release this lock. Using pthread rwlock functions
 * directly will break the reentrant tracking.
 */
typedef pthread_rwlock_t guac_rwlock;

/**
 * Initialize the provided guac reentrant rwlock. The lock will be configured to be
 * visible to child processes.
 *
 * @param lock
 *     The guac reentrant rwlock to be initialized.
 */
void guac_rwlock_init(guac_rwlock* lock);

/**
 * Clean up and destroy the provided guac reentrant rwlock.
 *
 * @param lock
 *     The guac reentrant rwlock to be destroyed.
 */
void guac_rwlock_destroy(guac_rwlock* lock);

/**
 * Acquires the write lock for the provided guac reentrant rwlock, or increments
 * its hold count if the current thread already holds it. If the current thread
 * holds a read lock, it will be released and a write lock acquired.
 *
 * If an error occurs while attempting to acquire the lock, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param reentrant_rwlock
 *     The guac reentrant rwlock for which the write lock should be acquired
 *     reentrantly.
 *
 * @return
 *     Zero if the lock is successfully acquired, or a non-zero value if an
 *     error occurred.
 */
int guac_rwlock_acquire_write_lock(guac_rwlock* reentrant_rwlock);

/**
 * Acquires the read lock for the provided guac reentrant rwlock, or increments
 * its hold count if the current thread already holds a read or write lock.
 *
 * If an error occurs while attempting to acquire the lock, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param reentrant_rwlock
 *     The guac reentrant rwlock for which the read lock should be acquired
 *     reentrantly.
 *
 * @return
 *     Zero if the lock is successfully acquired, or a non-zero value if an
 *     error occurred.
 */
int guac_rwlock_acquire_read_lock(guac_rwlock* reentrant_rwlock);

/**
 * Releases the rwlock associated with the provided guac reentrant rwlock if
 * this is the last level held by the current thread. Otherwise, decrements the
 * hold count to reflect the pending release.
 *
 * If an error occurs while attempting to release the lock, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param reentrant_rwlock
 *     The guac reentrant rwlock that should be released.
 *
 * @return
 *     Zero if the lock is successfully released, or a non-zero value if an
 *     error occurred.
 */
int guac_rwlock_release_lock(guac_rwlock* reentrant_rwlock);

#endif

