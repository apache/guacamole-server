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

#ifndef GUAC_RDP_DVC_H
#define GUAC_RDP_DVC_H

#include "config.h"
#include "common/list.h"

#include <freerdp/freerdp.h>

/**
 * The set of all arguments that should be passed to a given dynamic virtual
 * channel plugin, including the name of that plugin.
 */
typedef struct guac_rdp_dvc {

    /**
     * The number of arguments in the argv array. This MUST be at least 1.
     */
    int argc;

    /**
     * The argument values being passed to the dynamic virtual channel plugin.
     * The first entry in this array is always the name of the plugin. If
     * guac_rdp_load_drdynvc() has been invoked, and freeing the argument
     * values is being delegated to FreeRDP, this will be NULL.
     */
    char** argv;

} guac_rdp_dvc;

/**
 * A list of dynamic virtual channels which should be provided to the DRDYNVC
 * plugin once loaded via guac_rdp_load_drdynvc(). This interface exists purely
 * to bridge incompatibilities between differing versions of FreeRDP and its
 * DRDYNVC plugin. Any allocated guac_rdp_dvc_list is unlikely to be needed
 * after the DRDYNVC plugin has been loaded.
 */
typedef struct guac_rdp_dvc_list {

    /**
     * Array of all dynamic virtual channels which should be registered with
     * the DRDYNVC plugin once loaded. Each list element will point to a
     * guac_rdp_dvc structure which must eventually be freed.
     */
    guac_common_list* channels;

    /**
     * The number of channels within the list.
     */
    int channel_count;

} guac_rdp_dvc_list;

/**
 * Allocates a new, empty list of dynamic virtual channels. New channels may
 * be added via guac_rdp_dvc_list_add(). The loading of those channels'
 * associated plugins will be deferred until guac_rdp_load_drdynvc() is
 * invoked.
 *
 * @return
 *     A newly-allocated, empty list of dynamic virtual channels.
 */
guac_rdp_dvc_list* guac_rdp_dvc_list_alloc();

/**
 * Adds the given dynamic virtual channel plugin name and associated arguments
 * to the list. The provied arguments list is NOT optional and MUST be
 * NULL-terminated, even if there are no arguments for the named dynamic
 * virtual channel plugin. Though FreeRDP requires that the arguments for a
 * dynamic virtual channel plugin contain the name of the plugin itself as the
 * first argument, the name must be excluded from the arguments provided here.
 * This function will automatically take care of adding the plugin name to
 * the arguments.
 *
 * @param list
 *     The guac_rdp_dvc_list to which the given plugin name and arguments
 *     should be added, for later bulk registration via
 *     guac_rdp_load_drdynvc().
 *
 * @param name
 *     The name of the dynamic virtual channel plugin that should be given
 *     the provided arguments when guac_rdp_load_drdynvc() is invoked.
 *
 * @param ...
 *     The string (char*) arguments which should be passed to the dynamic
 *     virtual channel plugin when it is loaded via guac_rdp_load_drdynvc(),
 *     excluding the plugin name itself.
 */
void guac_rdp_dvc_list_add(guac_rdp_dvc_list* list, const char* name, ...);

/**
 * Frees the given list of dynamic virtual channels. Note that, while each
 * individual entry within this list will be freed, it is partially up to
 * FreeRDP to free the storage associated with the arguments passed to the
 * virtual channels.
 *
 * @param list
 *     The list to free.
 */
void guac_rdp_dvc_list_free(guac_rdp_dvc_list* list);

/**
 * Loads FreeRDP's DRDYNVC plugin and registers the dynamic virtual channel
 * plugins described by the given guac_rdp_dvc_list. This function MUST be
 * invoked no more than once per RDP connection. Invoking this function
 * multiple times, even if the guac_rdp_dvc_list is different each time, will
 * result in undefined behavior.
 *
 * @param context
 *     The rdpContext associated with the RDP connection for which the DRDYNVC
 *     plugin should be loaded.
 *
 * @param list
 *     A guac_rdp_dvc_list describing the dynamic virtual channel plugins that
 *     should be registered with the DRDYNVC plugin, along with any arguments.
 */
int guac_rdp_load_drdynvc(rdpContext* context, guac_rdp_dvc_list* list);

#endif

