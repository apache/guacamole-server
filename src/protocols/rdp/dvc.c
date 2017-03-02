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
#include "common/list.h"
#include "dvc.h"
#include "rdp.h"

#include <freerdp/channels/channels.h>
#include <freerdp/freerdp.h>
#include <guacamole/client.h>

#include <assert.h>
#include <stdarg.h>

guac_rdp_dvc_list* guac_rdp_dvc_list_alloc() {

    guac_rdp_dvc_list* list = malloc(sizeof(guac_rdp_dvc_list));

    /* Initialize with empty backing list */
    list->channels = guac_common_list_alloc();
    list->channel_count = 0;

    return list;

}

void guac_rdp_dvc_list_add(guac_rdp_dvc_list* list, const char* name, ...) {

    va_list args;

    guac_rdp_dvc* dvc = malloc(sizeof(guac_rdp_dvc));

    va_start(args, name);

    /* Count number of arguments (excluding terminating NULL) */
    dvc->argc = 1;
    while (va_arg(args, char*) != NULL)
        dvc->argc++;

    /* Reset va_list */
    va_end(args);
    va_start(args, name);

    /* Copy argument values into DVC entry */
    dvc->argv = malloc(sizeof(char*) * dvc->argc);
    dvc->argv[0] = strdup(name);
    int i;
    for (i = 1; i < dvc->argc; i++)
        dvc->argv[i] = strdup(va_arg(args, char*));

    va_end(args);

    /* Add entry to DVC list */
    guac_common_list_add(list->channels, dvc);

    /* Update channel count */
    list->channel_count++;

}

void guac_rdp_dvc_list_free(guac_rdp_dvc_list* list) {

    /* For each channel */
    guac_common_list_element* current = list->channels->head;
    while (current != NULL) {

        /* Free arguments declaration for current channel */
        guac_rdp_dvc* dvc = (guac_rdp_dvc*) current->data;

        /* Free the underlying arguments list if not delegated to FreeRDP */
        if (dvc->argv != NULL) {

            /* Free each argument value */
            for (int i = 0; i < dvc->argc; i++)
                free(dvc->argv[i]);

            free(dvc->argv);
        }

        free(dvc);

        current = current->next;

    }

    /* Free underlying list */
    guac_common_list_free(list->channels);

    /* Free the DVC list itself */
    free(list);

}

int guac_rdp_load_drdynvc(rdpContext* context, guac_rdp_dvc_list* list) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    rdpChannels* channels = context->channels;

    /* Skip if no channels will be loaded */
    if (list->channel_count == 0)
        return 0;

#ifndef HAVE_ADDIN_ARGV
    /* Allocate plugin data array */
    RDP_PLUGIN_DATA* all_plugin_data =
        calloc(list->channel_count + 1, sizeof(RDP_PLUGIN_DATA));

    RDP_PLUGIN_DATA* current_plugin_data = all_plugin_data;
#endif

    /* For each channel */
    guac_common_list_element* current = list->channels->head;
    while (current != NULL) {

        /* Get channel arguments */
        guac_rdp_dvc* dvc = (guac_rdp_dvc*) current->data;
        current = current->next;

        /* guac_rdp_dvc_list_add() guarantees at one argument */
        assert(dvc->argc >= 1);

        /* guac_rdp_load_drdynvc() MUST only be invoked once */
        assert(dvc->argv != NULL);

        /* Log registration of plugin for current channel */
        guac_client_log(client, GUAC_LOG_DEBUG,
                "Registering DVC plugin \"%s\"", dvc->argv[0]);

#ifdef HAVE_ADDIN_ARGV
        /* Register plugin with FreeRDP */
        ADDIN_ARGV* args = malloc(sizeof(ADDIN_ARGV));
        args->argc = dvc->argc;
        args->argv = dvc->argv;
        freerdp_dynamic_channel_collection_add(context->settings, args);
#else
        /* Copy all arguments */
        for (int i = 0; i < dvc->argc; i++)
            current_plugin_data->data[i] = dvc->argv[i];

        /* Store size of entry */
        current_plugin_data->size = sizeof(*current_plugin_data);

        /* Advance to next set of plugin data */
        current_plugin_data++;
#endif

        /* Rely on FreeRDP to free argv storage */
        dvc->argv = NULL;

    }

#ifdef HAVE_ADDIN_ARGV
    /* Load virtual channel management plugin */
    return freerdp_channels_load_plugin(channels, context->instance->settings,
                "drdynvc", context->instance->settings);
#else
    /* Terminate with empty RDP_PLUGIN_DATA element */
    current_plugin_data->size = 0;

    /* Load virtual channel management plugin */
    return freerdp_channels_load_plugin(channels, context->instance->settings,
                "drdynvc", all_plugin_data);
#endif

}

