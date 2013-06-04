
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is libguac.
 *
 * The Initial Developer of the Original Code is
 * Michael Jumper.
 * Portions created by the Initial Developer are Copyright (C) 2010
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "client.h"
#include "client-handlers.h"
#include "error.h"
#include "plugin.h"
#include "protocol.h"
#include "socket.h"
#include "time.h"

guac_client_plugin* guac_client_plugin_open(const char* protocol) {

    guac_client_plugin* plugin;

    /* Reference to dlopen()'d plugin */
    void* client_plugin_handle;

    /* Client args description */
    const char** client_args;

    /* Pluggable client */
    char protocol_lib[GUAC_PROTOCOL_LIBRARY_LIMIT] =
        GUAC_PROTOCOL_LIBRARY_PREFIX;
 
    union {
        guac_client_init_handler* client_init;
        void* obj;
    } alias;

    /* Add protocol and .so suffix to protocol_lib */
    strncat(protocol_lib, protocol, GUAC_PROTOCOL_NAME_LIMIT-1);
    strcat(protocol_lib, GUAC_PROTOCOL_LIBRARY_SUFFIX);

    /* Load client plugin */
    client_plugin_handle = dlopen(protocol_lib, RTLD_LAZY);
    if (!client_plugin_handle) {
        guac_error = GUAC_STATUS_BAD_ARGUMENT;
        guac_error_message = dlerror();
        return NULL;
    }

    dlerror(); /* Clear errors */

    /* Get init function */
    alias.obj = dlsym(client_plugin_handle, "guac_client_init");

    /* Fail if cannot find guac_client_init */
    if (dlerror() != NULL) {
        guac_error = GUAC_STATUS_BAD_ARGUMENT;
        guac_error_message = dlerror();
        return NULL;
    }

    /* Get usage strig */
    client_args = (const char**) dlsym(client_plugin_handle, "GUAC_CLIENT_ARGS");

    /* Fail if cannot find GUAC_CLIENT_ARGS */
    if (dlerror() != NULL) {
        guac_error = GUAC_STATUS_BAD_ARGUMENT;
        guac_error_message = dlerror();
        return NULL;
    }

    /* Allocate plugin */
    plugin = malloc(sizeof(guac_client_plugin));
    if (plugin == NULL) {
        guac_error = GUAC_STATUS_NO_MEMORY;
        guac_error_message = "Could not allocate memory for client plugin";
        return NULL;
    } 

    /* Init and return plugin */
    plugin->__client_plugin_handle = client_plugin_handle;
    plugin->init_handler = alias.client_init;
    plugin->args = client_args;
    return plugin;

}

int guac_client_plugin_close(guac_client_plugin* plugin) {

    /* Unload client plugin */
    if (dlclose(plugin->__client_plugin_handle)) {
        guac_error = GUAC_STATUS_BAD_STATE;
        guac_error_message = dlerror();
        return -1;
    }

    /* Free plugin handle */
    free(plugin);
    return 0;

}

int guac_client_plugin_init_client(guac_client_plugin* plugin,
        guac_client* client, int argc, char** argv) {

    return plugin->init_handler(client, argc, argv);

}

