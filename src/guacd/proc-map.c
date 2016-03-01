/*
 * Copyright (C) 2014 Glyptodon LLC
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
#include "proc.h"
#include "proc-map.h"
#include "user.h"

#include <guacamole/client.h>

#include <stdlib.h>
#include <string.h>

/**
 * Returns a hash code based on the connection ID of the given client.
 */
static unsigned int __guacd_client_hash(const char* str) {

    unsigned int hash_value = 0;
    int c;

    /* Apply each character in string to the hash code */
    while ((c = *(str++)))
        hash_value = hash_value * 65599 + c;

    return hash_value;

}

/**
 * Locates the bucket corresponding to the hash code indicated by the give id.
 * Each bucket is an instance of guac_common_list.
 */
static guac_common_list* __guacd_proc_find_bucket(guacd_proc_map* map, const char* id) {

    const int index = __guacd_client_hash(id) % GUACD_PROC_MAP_BUCKETS;
    return map->__buckets[index];

}

/**
 * Given a list of guacd_proc instances, returns the
 * guacd_proc having the guac_client with the given ID, or NULL if no
 * such client is stored.
 */
static guac_common_list_element* __guacd_proc_find(guac_common_list* bucket, const char* id) {

    guac_common_list_element* current = bucket->head;

    /* Search for matching element within bucket */
    while (current != NULL) {

        /* Check connection ID */
        guacd_proc* proc = (guacd_proc*) current->data;
        if (strcmp(proc->client->connection_id, id) == 0)
            break;

        current = current->next;
    }

    return current;

}

guacd_proc_map* guacd_proc_map_alloc() {

    guacd_proc_map* map = malloc(sizeof(guacd_proc_map));
    guac_common_list** current;

    int i;

    /* Init all buckets */
    current = map->__buckets;

    for (i=0; i<GUACD_PROC_MAP_BUCKETS; i++) {
        *current = guac_common_list_alloc();
        current++;
    }

    return map;

}

int guacd_proc_map_add(guacd_proc_map* map, guacd_proc* proc) {

    const char* identifier = proc->client->connection_id;
    guac_common_list* bucket = __guacd_proc_find_bucket(map, identifier);
    guac_common_list_element* found;

    /* Retrieve corresponding element, if any */
    guac_common_list_lock(bucket);
    found = __guacd_proc_find(bucket, identifier);

    /* If no such element, we can add the new client successfully */
    if (found == NULL) {
        guac_common_list_add(bucket, proc);
        guac_common_list_unlock(bucket);
        return 0;
    }

    /* Otherwise, fail - already exists */
    guac_common_list_unlock(bucket);
    return 1;

}

guacd_proc* guacd_proc_map_retrieve(guacd_proc_map* map, const char* id) {

    guacd_proc* proc;

    guac_common_list* bucket = __guacd_proc_find_bucket(map, id);
    guac_common_list_element* found;

    /* Retrieve corresponding element, if any */
    guac_common_list_lock(bucket);
    found = __guacd_proc_find(bucket, id);

    /* If no such element, fail */
    if (found == NULL) {
        guac_common_list_unlock(bucket);
        return NULL;
    }

    proc = (guacd_proc*) found->data;

    guac_common_list_unlock(bucket);
    return proc;

}

guacd_proc* guacd_proc_map_remove(guacd_proc_map* map, const char* id) {

    guacd_proc* proc;

    guac_common_list* bucket = __guacd_proc_find_bucket(map, id);
    guac_common_list_element* found;

    /* Retrieve corresponding element, if any */
    guac_common_list_lock(bucket);
    found = __guacd_proc_find(bucket, id);

    /* If no such element, fail */
    if (found == NULL) {
        guac_common_list_unlock(bucket);
        return NULL;
    }

    proc = (guacd_proc*) found->data;
    guac_common_list_remove(bucket, found);

    guac_common_list_unlock(bucket);
    return proc;

}

