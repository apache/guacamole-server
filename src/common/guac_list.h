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

#ifndef __GUAC_LIST_H
#define __GUAC_LIST_H

#include "config.h"

#include <pthread.h>

/**
 * Generic linked list element.
 */
typedef struct guac_common_list_element guac_common_list_element;

struct guac_common_list_element {

    /**
     * The next element in the list, or NULL if none.
     */
    guac_common_list_element* next;

    /**
     * Generic data.
     */
    void* data;

    /**
     * The pointer which points to this element, whether another element's
     * next pointer, or the entire list's head pointer.
     */
    guac_common_list_element** _ptr;

};

/**
 * Generic linked list.
 */
typedef struct guac_common_list {

    /**
     * The first element in the list.
     */
    guac_common_list_element* head;

    /**
     * Mutex which is locked when exclusive access to the list is required.
     * Possession of the lock is not enforced outside the
     * guac_common_list_lock() function.
     */
    pthread_mutex_t _lock;

} guac_common_list;

/**
 * Creates a new list.
 *
 * @return A newly-allocated list.
 */
guac_common_list* guac_common_list_alloc();

/**
 * Frees the given list.
 *
 * @param list The list to free.
 */
void guac_common_list_free(guac_common_list* list);

/**
 * Adds the given data to the list as a new element, returning the created
 * element.
 *
 * @param list The list to add an element to.
 * @param data The data to associate with the newly-created element.
 * @param The newly-created element.
 */
guac_common_list_element* guac_common_list_add(guac_common_list* list,
        void* data);

/**
 * Removes the given element from the list.
 *
 * @param list The list to remove the element from.
 * @param element The element to remove.
 */
void guac_common_list_remove(guac_common_list* list,
        guac_common_list_element* element);

/**
 * Acquires exclusive access to the list. No list functions implicitly lock or
 * unlock the list, so any list access which must be threadsafe must use
 * guac_common_list_lock() and guac_common_list_unlock() manually.
 *
 * @param list The list to acquire exclusive access to.
 */
void guac_common_list_lock(guac_common_list* list);

/**
 * Releases exclusive access to the list.
 *
 * @param list The list to release from exclusive access.
 */
void guac_common_list_unlock(guac_common_list* list);

#endif

