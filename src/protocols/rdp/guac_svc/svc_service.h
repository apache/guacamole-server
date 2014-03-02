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

#ifndef __GUAC_SVC_SERVICE_H
#define __GUAC_SVC_SERVICE_H

#include "config.h"

#include <pthread.h>

#include <freerdp/utils/svc_plugin.h>
#include <guacamole/client.h>

#ifdef ENABLE_WINPR
#include <winpr/stream.h>
#else
#include "compat/winpr-stream.h"
#endif

/**
 * Structure representing the current state of an arbitrary static virtual
 * channel.
 */
typedef struct guac_svcPlugin {

    /**
     * The FreeRDP parts of this plugin. This absolutely MUST be first.
     * FreeRDP depends on accessing this structure as if it were an instance
     * of rdpSvcPlugin.
     */
    rdpSvcPlugin plugin;

    /**
     * Reference to the client owning this instance.
     */
    guac_client* client;

    /**
     * The name of the RDP channel in use, and the name to use for each pipe.
     */
    char* name;

    /**
     * The pipe opened by the Guacamole client.
     */
    guac_stream* input_pipe;

    /**
     * The output pipe, opened in response to the open input pipe.
     */
    guac_stream* output_pipe;

} guac_svcPlugin;

/**
 * Handler called when this plugin is loaded by FreeRDP.
 */
void guac_svc_process_connect(rdpSvcPlugin* plugin);

/**
 * Handler called when this plugin receives data along its designated channel.
 */
void guac_svc_process_receive(rdpSvcPlugin* plugin,
        wStream* input_stream);

/**
 * Handler called when this plugin is being unloaded.
 */
void guac_svc_process_terminate(rdpSvcPlugin* plugin);

/**
 * Handler called when this plugin receives an event.
 */
void guac_svc_process_event(rdpSvcPlugin* plugin, wMessage* event);

#endif

