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
#include "guac_list.h"

#include <stdlib.h>
#include <pthread.h>

guac_common_list* guac_common_list_alloc() {

    guac_common_list* list = malloc(sizeof(guac_common_list));

    pthread_mutex_init(&list->_lock, NULL);
    list->head = NULL;

    return list;

}

void guac_common_list_free(guac_common_list* list) {
    free(list);
}

guac_common_list_element* guac_common_list_add(guac_common_list* list,
        void* data) {

    /* Allocate element, initialize as new head */
    guac_common_list_element* element =
        malloc(sizeof(guac_common_list_element));
    element->data = data;
    element->next = list->head;
    element->_ptr = &(list->head);

    /* If head already existed, point it at this element */
    if (list->head != NULL)
        list->head->_ptr = &(element->next);

    /* Set as new head */
    list->head = element;
    return element;

}

void guac_common_list_remove(guac_common_list* list,
        guac_common_list_element* element) {

    /* Point previous (or head) to next */
    *(element->_ptr) = element->next;

    if (element->next != NULL)
        element->next->_ptr = element->_ptr;

    free(element);

}

void guac_common_list_lock(guac_common_list* list) {
    pthread_mutex_lock(&list->_lock);
}

void guac_common_list_unlock(guac_common_list* list) {
    pthread_mutex_unlock(&list->_lock);
}

