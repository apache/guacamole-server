/*
 * Copyright (C) 2013 Glyptodon LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "config.h"

#include "pool.h"

#include <stdlib.h>

guac_pool* guac_pool_alloc(int size) {

    pthread_mutexattr_t lock_attributes;
    guac_pool* pool = malloc(sizeof(guac_pool));

    /* If unable to allocate, just return NULL. */
    if (pool == NULL)
        return NULL;

    /* Initialize empty pool */
    pool->min_size = size;
    pool->active = 0;
    pool->__next_value = 0;
    pool->__head = NULL;
    pool->__tail = NULL;

    /* Init lock */
    pthread_mutexattr_init(&lock_attributes);
    pthread_mutexattr_setpshared(&lock_attributes, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&(pool->__lock), &lock_attributes);

    return pool;

}

void guac_pool_free(guac_pool* pool) {

    /* Free all ints in pool */
    guac_pool_int* current = pool->__head;
    while (current != NULL) {

        guac_pool_int* old = current;
        current = current->__next;

        free(old);
    }

    /* Destroy lock */
    pthread_mutex_destroy(&(pool->__lock));

    /* Free pool */
    free(pool);

}

int guac_pool_next_int(guac_pool* pool) {

    int value;

    /* Acquire exclusive access */
    pthread_mutex_lock(&(pool->__lock));

    pool->active++;

    /* If more integers are needed, return a new one. */
    if (pool->__head == NULL || pool->__next_value < pool->min_size) {
        value = pool->__next_value++;
        pthread_mutex_unlock(&(pool->__lock));
        return value;
    }

    /* Otherwise, remove first integer. */
    value = pool->__head->value;

    /* If only one element exists, reset pool to empty. */
    if (pool->__tail == pool->__head) {
        free(pool->__head);
        pool->__head = NULL;
        pool->__tail = NULL;
    }

    /* Otherwise, advance head. */
    else {
        guac_pool_int* old_head = pool->__head;
        pool->__head = old_head->__next;
        free(old_head);
    }

    /* Return retrieved value. */
    pthread_mutex_unlock(&(pool->__lock));
    return value;
}

void guac_pool_free_int(guac_pool* pool, int value) {

    /* Allocate and initialize new returned value */
    guac_pool_int* pool_int = malloc(sizeof(guac_pool_int));
    pool_int->value = value;
    pool_int->__next = NULL;

    /* Acquire exclusive access */
    pthread_mutex_lock(&(pool->__lock));

    pool->active--;

    /* If pool empty, store as sole entry. */
    if (pool->__tail == NULL)
        pool->__head = pool->__tail = pool_int;

    /* Otherwise, append to end of pool. */
    else {
        pool->__tail->__next = pool_int;
        pool->__tail = pool_int;
    }

    /* Value has been freed */
    pthread_mutex_unlock(&(pool->__lock));

}

