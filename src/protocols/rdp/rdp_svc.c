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
#include "rdp_svc.h"

#include <freerdp/freerdp.h>
#include <guacamole/client.h>

#ifdef ENABLE_WINPR
#include <winpr/stream.h>
#else
#include "compat/winpr-stream.h"
#endif

guac_rdp_svc* guac_rdp_alloc_svc(guac_client* client, char* name) {

    guac_rdp_svc* svc = malloc(sizeof(guac_rdp_svc));

    /* Init SVC */
    svc->client = client;
    svc->name = strdup(name);
    svc->input_pipe = NULL;
    svc->output_pipe = NULL;

    return svc;
}

void guac_rdp_free_svc(guac_rdp_svc* svc) {
    free(svc->name);
    free(svc);
}

void guac_rdp_add_svc(guac_client* client, guac_rdp_svc* svc) {

    rdp_guac_client_data* client_data = (rdp_guac_client_data*) client->data;

    /* Add to list of available SVC */
    guac_common_list_lock(client_data->available_svc);
    guac_common_list_add(client_data->available_svc, svc);
    guac_common_list_unlock(client_data->available_svc);

}

guac_rdp_svc* guac_rdp_get_svc(guac_client* client, const char* name) {

    rdp_guac_client_data* client_data = (rdp_guac_client_data*) client->data;
    guac_common_list_element* current;
    guac_rdp_svc* found = NULL;

    /* For each available SVC */
    guac_common_list_lock(client_data->available_svc);
    current = client_data->available_svc->head;
    while (current != NULL) {

        /* If name matches, found */
        guac_rdp_svc* current_svc = (guac_rdp_svc*) current->data;
        if (strcmp(current_svc->name, name) == 0) {
            found = current_svc;
            break;
        }

        current = current->next;

    }
    guac_common_list_unlock(client_data->available_svc);

    return found;

}

guac_rdp_svc* guac_rdp_remove_svc(guac_client* client, const char* name) {

    rdp_guac_client_data* client_data = (rdp_guac_client_data*) client->data;
    guac_common_list_element* current;
    guac_rdp_svc* found = NULL;

    /* For each available SVC */
    guac_common_list_lock(client_data->available_svc);
    current = client_data->available_svc->head;
    while (current != NULL) {

        /* If name matches, remove entry */
        guac_rdp_svc* current_svc = (guac_rdp_svc*) current->data;
        if (strcmp(current_svc->name, name) == 0) {
            guac_common_list_remove(client_data->available_svc, current);
            found = current_svc;
            break;
        }

        current = current->next;

    }
    guac_common_list_unlock(client_data->available_svc);

    /* Return removed entry, if any */
    return found;

}

