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
#include "error.h"
#include "plugin.h"

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
        guac_error = GUAC_STATUS_NOT_FOUND;
        guac_error_message = dlerror();
        return NULL;
    }

    dlerror(); /* Clear errors */

    /* Get init function */
    alias.obj = dlsym(client_plugin_handle, "guac_client_init");

    /* Fail if cannot find guac_client_init */
    if (dlerror() != NULL) {
        guac_error = GUAC_STATUS_INTERNAL_ERROR;
        guac_error_message = dlerror();
        return NULL;
    }

    /* Get usage strig */
    client_args = (const char**) dlsym(client_plugin_handle, "GUAC_CLIENT_ARGS");

    /* Fail if cannot find GUAC_CLIENT_ARGS */
    if (dlerror() != NULL) {
        guac_error = GUAC_STATUS_INTERNAL_ERROR;
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
        guac_error = GUAC_STATUS_INTERNAL_ERROR;
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

