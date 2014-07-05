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


#ifndef _GUACD_CLIENT_MAP_H
#define _GUACD_CLIENT_MAP_H

#include "config.h"
#include "client.h"
#include "guac_list.h"

#include <guacamole/client.h>

#define GUACD_CLIENT_MAP_BUCKETS GUACD_CLIENT_MAX_CONNECTIONS*2

/**
 * Set of all active connections to guacd, indexed by connection ID.
 */
typedef struct guacd_client_map {

    /**
     * Internal hash buckets. Each bucket is a linked list containing all
     * guac_client instances which hash to this bucket location.
     */
    guac_common_list* __buckets[GUACD_CLIENT_MAP_BUCKETS];

} guacd_client_map;

/**
 * Allocates a new client map, which persists for the life of guacd.
 */
guacd_client_map* guacd_client_map_alloc();

/**
 * Adds the given client to the given client map. On success, zero is returned.
 * If adding the client fails (due to lack of space, or duplicate ID), a
 * non-zero value is returned instead.
 */
int guacd_client_map_add(guacd_client_map* map, guac_client* client);

/**
 * Retrieves the client having the given ID, or NULL if no such client
 * is stored.
 */
guac_client* guacd_client_map_retrieve(guacd_client_map* map, const char* id);

/**
 * Removes the client having the given ID, returning the corresponding client.
 * If no such client exists, NULL is returned.
 */
guac_client* guacd_client_map_remove(guacd_client_map* map, const char* id);

#endif

