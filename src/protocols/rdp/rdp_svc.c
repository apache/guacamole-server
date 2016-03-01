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
#include "client.h"
#include "guac_list.h"
#include "rdp.h"
#include "rdp_svc.h"

#include <freerdp/utils/svc_plugin.h>
#include <guacamole/client.h>

#ifdef ENABLE_WINPR
#include <winpr/stream.h>
#else
#include "compat/winpr-stream.h"
#endif

#include <stdlib.h>
#include <string.h>

guac_rdp_svc* guac_rdp_alloc_svc(guac_client* client, char* name) {

    guac_rdp_svc* svc = malloc(sizeof(guac_rdp_svc));

    /* Init SVC */
    svc->client = client;
    svc->plugin = NULL;
    svc->input_pipe = NULL;
    svc->output_pipe = NULL;

    /* Warn about name length */
    if (strnlen(name, GUAC_RDP_SVC_MAX_LENGTH+1) > GUAC_RDP_SVC_MAX_LENGTH)
        guac_client_log(client, GUAC_LOG_INFO,
                "Static channel name \"%s\" exceeds maximum of %i characters "
                "and will be truncated",
                name, GUAC_RDP_SVC_MAX_LENGTH);

    /* Init name */
    strncpy(svc->name, name, GUAC_RDP_SVC_MAX_LENGTH);
    svc->name[GUAC_RDP_SVC_MAX_LENGTH] = '\0';

    return svc;
}

void guac_rdp_free_svc(guac_rdp_svc* svc) {
    free(svc);
}

void guac_rdp_add_svc(guac_client* client, guac_rdp_svc* svc) {

    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    /* Add to list of available SVC */
    guac_common_list_lock(rdp_client->available_svc);
    guac_common_list_add(rdp_client->available_svc, svc);
    guac_common_list_unlock(rdp_client->available_svc);

}

guac_rdp_svc* guac_rdp_get_svc(guac_client* client, const char* name) {

    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    guac_common_list_element* current;
    guac_rdp_svc* found = NULL;

    /* For each available SVC */
    guac_common_list_lock(rdp_client->available_svc);
    current = rdp_client->available_svc->head;
    while (current != NULL) {

        /* If name matches, found */
        guac_rdp_svc* current_svc = (guac_rdp_svc*) current->data;
        if (strcmp(current_svc->name, name) == 0) {
            found = current_svc;
            break;
        }

        current = current->next;

    }
    guac_common_list_unlock(rdp_client->available_svc);

    return found;

}

guac_rdp_svc* guac_rdp_remove_svc(guac_client* client, const char* name) {

    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    guac_common_list_element* current;
    guac_rdp_svc* found = NULL;

    /* For each available SVC */
    guac_common_list_lock(rdp_client->available_svc);
    current = rdp_client->available_svc->head;
    while (current != NULL) {

        /* If name matches, remove entry */
        guac_rdp_svc* current_svc = (guac_rdp_svc*) current->data;
        if (strcmp(current_svc->name, name) == 0) {
            guac_common_list_remove(rdp_client->available_svc, current);
            found = current_svc;
            break;
        }

        current = current->next;

    }
    guac_common_list_unlock(rdp_client->available_svc);

    /* Return removed entry, if any */
    return found;

}

void guac_rdp_svc_write(guac_rdp_svc* svc, void* data, int length) {

    wStream* output_stream;

    /* Do not write of plugin not associated */
    if (svc->plugin == NULL) {
        guac_client_log(svc->client, GUAC_LOG_ERROR,
                "Channel \"%s\" output dropped.",
                svc->name);
        return;
    }

    /* Build packet */
    output_stream = Stream_New(NULL, length);
    Stream_Write(output_stream, data, length);

    /* Send packet */
    svc_plugin_send(svc->plugin, output_stream);

}

