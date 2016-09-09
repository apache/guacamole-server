
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
#include "guac_client.h"
#include "guac_drawable.h"
#include "list.h"

#include <xf86.h>
#include <xf86str.h>
#include <guacamole/client.h>

#define GUAC_DRV_MULTICAST_CALL(func, clients, ...) do { \
                                                          \
    guac_drv_list_element* current;                       \
                                                          \
    /* For each client */                                 \
    guac_drv_list_lock(clients);                          \
    current = clients->head;                              \
    while (current != NULL) {                             \
        func((guac_client*) current->data, __VA_ARGS__);  \
        current = current->next;                          \
    }                                                     \
    guac_drv_list_unlock(clients);                        \
                                                          \
} while (0);

void guac_drv_multicast_create_drawable(guac_drv_list* clients,
        guac_drv_drawable* drawable) {
    GUAC_DRV_MULTICAST_CALL(guac_drv_client_create_drawable,
            clients, drawable);
}

void guac_drv_multicast_shade_drawable(guac_drv_list* clients,
        guac_drv_drawable* drawable) {
    GUAC_DRV_MULTICAST_CALL(guac_drv_client_shade_drawable,
            clients, drawable);
}

void guac_drv_multicast_destroy_drawable(guac_drv_list* clients,
        guac_drv_drawable* drawable) {
    GUAC_DRV_MULTICAST_CALL(guac_drv_client_destroy_drawable,
            clients, drawable);
}

void guac_drv_multicast_move_drawable(guac_drv_list* clients,
        guac_drv_drawable* drawable) {
    GUAC_DRV_MULTICAST_CALL(guac_drv_client_move_drawable,
            clients, drawable);
}

void guac_drv_multicast_resize_drawable(guac_drv_list* clients,
        guac_drv_drawable* drawable) {
    GUAC_DRV_MULTICAST_CALL(guac_drv_client_resize_drawable,
            clients, drawable);
}

void guac_drv_multicast_copy(guac_drv_list* clients,
        guac_drv_drawable* src, int srcx, int srcy, int w, int h,
        guac_drv_drawable* dst, int dstx, int dsty) {
    GUAC_DRV_MULTICAST_CALL(guac_drv_client_copy, clients,
            src, srcx, srcy, w, h,
            dst, dstx, dsty);
}

void guac_drv_multicast_draw(guac_drv_list* clients,
        guac_drv_drawable* drawable, int x, int y, int w, int h) {
    GUAC_DRV_MULTICAST_CALL(guac_drv_client_draw, clients, drawable,
            x, y, w, h);
}

void guac_drv_multicast_crect(guac_drv_list* clients,
        guac_drv_drawable* drawable, int x, int y, int w, int h,
        int r, int g, int b, int a) {
    GUAC_DRV_MULTICAST_CALL(guac_drv_client_crect, clients, drawable,
            x, y, w, h,
            r, g, b, a);
}

void guac_drv_multicast_drect(guac_drv_list* clients,
        guac_drv_drawable* drawable, int x, int y, int w, int h,
        guac_drv_drawable* fill) {
    GUAC_DRV_MULTICAST_CALL(guac_drv_client_drect, clients,
            drawable, x, y, w, h, fill);
}

void guac_drv_multicast_end_frame(guac_drv_list* clients) {

    guac_drv_list_element* current;

    /* For each client, end frame */
    guac_drv_list_lock(clients);
    current = clients->head;
    while (current != NULL) {
        guac_drv_client_end_frame((guac_client*) current->data);
        current = current->next;
    }
    guac_drv_list_unlock(clients);

}

