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


#ifndef _GUAC_RDP_RDP_POINTER_H
#define _GUAC_RDP_RDP_POINTER_H

#include "config.h"
#include "common/display.h"

#include <freerdp/freerdp.h>

/**
 * Guacamole-specific rdpPointer data.
 */
typedef struct guac_rdp_pointer {

    /**
     * FreeRDP pointer data - MUST GO FIRST.
     */
    rdpPointer pointer;

    /**
     * The display layer containing cached image data.
     */
    guac_common_display_layer* layer;

} guac_rdp_pointer;

/**
 * Caches a new pointer, which can later be set via guac_rdp_pointer_set() as
 * the current mouse pointer.
 *
 * @param context
 *     The rdpContext associated with the current RDP session.
 *
 * @param pointer
 *     The pointer to cache.
 */
void guac_rdp_pointer_new(rdpContext* context, rdpPointer* pointer);

/**
 * Sets the given cached pointer as the current pointer. The given pointer must
 * have already been initialized through a call to guac_rdp_pointer_new().
 *
 * @param context
 *     The rdpContext associated with the current RDP session.
 *
 * @param pointer
 *     The pointer to set as the current mouse pointer.
 */
void guac_rdp_pointer_set(rdpContext* context, rdpPointer* pointer);

/**
 * Frees all Guacamole-related data associated with the given pointer, allowing
 * FreeRDP to free the rest safely.
 *
 * @param context
 *     The rdpContext associated with the current RDP session.
 *
 * @param pointer
 *     The pointer to free.
 */
void guac_rdp_pointer_free(rdpContext* context, rdpPointer* pointer);

/**
 * Hides the current mouse pointer.
 *
 * @param context
 *     The rdpContext associated with the current RDP session.
 */
void guac_rdp_pointer_set_null(rdpContext* context);

/**
 * Sets the system-dependent (as in dependent on the client system) default
 * pointer as the current pointer, rather than a cached pointer.
 *
 * @param context
 *     The rdpContext associated with the current RDP session.
 */
void guac_rdp_pointer_set_default(rdpContext* context);

#endif
