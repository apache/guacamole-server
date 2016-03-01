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


#ifndef _GUACD_PROC_MAP_H
#define _GUACD_PROC_MAP_H

#include "config.h"
#include "guac_list.h"
#include "proc.h"
#include "user.h"

#include <guacamole/client.h>

/**
 * The number of hash buckets in each process map.
 */
#define GUACD_PROC_MAP_BUCKETS GUACD_CLIENT_MAX_CONNECTIONS*2

/**
 * Set of all active connections to guacd, indexed by connection ID.
 */
typedef struct guacd_proc_map {

    /**
     * Internal hash buckets. Each bucket is a linked list containing all
     * guac_client instances which hash to this bucket location.
     */
    guac_common_list* __buckets[GUACD_PROC_MAP_BUCKETS];

} guacd_proc_map;

/**
 * Allocates a new client process map, which persists for the life of guacd.
 */
guacd_proc_map* guacd_proc_map_alloc();

/**
 * Adds the given process to the client process map. On success, zero is returned.  If
 * adding the client fails (due to lack of space, or duplicate ID), a non-zero
 * value is returned instead. The client process is stored by the connection ID
 * of the underlying guac_client.
 */
int guacd_proc_map_add(guacd_proc_map* map, guacd_proc* proc);

/**
 * Retrieves the client process having the client with the given ID, or NULL if
 * no such process is stored.
 */
guacd_proc* guacd_proc_map_retrieve(guacd_proc_map* map, const char* id);

/**
 * Removes the client process having the client with the given ID, returning
 * the corresponding process.  If no such process exists, NULL is returned.
 */
guacd_proc* guacd_proc_map_remove(guacd_proc_map* map, const char* id);

#endif

