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

#ifndef _GUAC_PLUGIN_H
#define _GUAC_PLUGIN_H

#include "client-types.h"
#include "plugin-constants.h"
#include "plugin-types.h"

/**
 * Provides functions and structures required for handling a client plugin.
 *
 * @file plugin.h
 */

struct guac_client_plugin {

    /**
     * Reference to dlopen'd client plugin.
     */
    void* __client_plugin_handle;

    /**
     * Reference to the init handler of this client plugin. This
     * function will be called when the client plugin is started.
     */
    guac_client_init_handler* init_handler;

    /**
     * NULL-terminated array of all arguments accepted by this client
     * plugin, in order. The values of these arguments will be passed
     * to the init_handler if the client plugin is started.
     */
    const char** args;

};

/**
 * Open the plugin which provides support for the given protocol, if it
 * exists.
 *
 * @param protocol The name of the protocol to retrieve the client plugin
 *                 for.
 * @return The client plugin supporting the given protocol, or NULL if
 *         an error occurs or no such plugin exists.
 */
guac_client_plugin* guac_client_plugin_open(const char* protocol);

/**
 * Close the given plugin, releasing all associated resources. This function
 * must be called after use of a client plugin is finished.
 *
 * @param plugin The client plugin to close.
 * @return Zero on success, non-zero if an error occurred while releasing
 *         the resources associated with the plugin.
 */
int guac_client_plugin_close(guac_client_plugin* plugin);

/**
 * Initializes the given guac_client using the initialization routine provided
 * by the given guac_client_plugin.
 *
 * @param plugin The client plugin to use to initialize the new client.
 * @param client The guac_client to initialize.
 * @param argc The number of arguments being passed to the client.
 * @param argv All arguments to be passed to the client.
 * @return Zero if initialization was successful, non-zero otherwise.
 */
int guac_client_plugin_init_client(guac_client_plugin* plugin, 
        guac_client* client, int argc, char** argv);

#endif
