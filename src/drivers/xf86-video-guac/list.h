
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

#ifndef __GUAC_LIST_H
#define __GUAC_LIST_H

#include "config.h"

#include <pthread.h>

/**
 * Generic linked list element.
 */
typedef struct guac_drv_list_element guac_drv_list_element;

struct guac_drv_list_element {

    /**
     * The next element in the list, or NULL if none.
     */
    guac_drv_list_element* next;

    /**
     * Generic data.
     */
    void* data;

    /**
     * The pointer which points to this element, whether another element's
     * next pointer, or the entire list's head pointer.
     */
    guac_drv_list_element** _ptr;

};

/**
 * Generic linked list.
 */
typedef struct guac_drv_list {

    /**
     * The first element in the list.
     */
    guac_drv_list_element* head;

    /**
     * Mutex which is locked when exclusive access to the list is required.
     * Possession of the lock is not enforced outside the guac_drv_list_lock()
     * function.
     */
    pthread_mutex_t _lock;

} guac_drv_list;

/**
 * Creates a new list.
 */
guac_drv_list* guac_drv_list_alloc();

/**
 * Frees the given list.
 */
void guac_drv_list_free(guac_drv_list* list);

/**
 * Adds the given data to the list as a new element, returning the created
 * element.
 */
guac_drv_list_element* guac_drv_list_add(guac_drv_list* list, void* data);

/**
 * Removes the given element from the list.
 */
void guac_drv_list_remove(guac_drv_list* list, guac_drv_list_element* element);

/**
 * Acquires exclusive access to the list.
 */
void guac_drv_list_lock(guac_drv_list* list);

/**
 * Releases exclusive access to the list.
 */
void guac_drv_list_unlock(guac_drv_list* list);

#endif

