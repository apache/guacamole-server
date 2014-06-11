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

#ifndef __GUAC_RDP_RDP_SVC_H
#define __GUAC_RDP_RDP_SVC_H

#include "config.h"

#include <freerdp/utils/svc_plugin.h>
#include <guacamole/client.h>
#include <guacamole/stream.h>

/**
 * The maximum number of characters to allow for each channel name.
 */
#define GUAC_RDP_SVC_MAX_LENGTH 7

/**
 * Structure describing a static virtual channel, and the corresponding
 * Guacamole pipes.
 */
typedef struct guac_rdp_svc {

    /**
     * Reference to the client owning this static channel.
     */
    guac_client* client;

    /**
     * Reference to associated SVC plugin.
     */
    rdpSvcPlugin* plugin;

    /**
     * The name of the RDP channel in use, and the name to use for each pipe.
     */
    char name[GUAC_RDP_SVC_MAX_LENGTH+1];

    /**
     * The pipe opened by the Guacamole client, if any. This should be
     * opened in response to the output pipe.
     */
    guac_stream* input_pipe;

    /**
     * The output pipe, opened when the RDP server receives a connection to
     * the static channel.
     */
    guac_stream* output_pipe;

} guac_rdp_svc;

/**
 * Allocate a new SVC with the given name.
 */
guac_rdp_svc* guac_rdp_alloc_svc(guac_client* client, char* name);

/**
 * Free the given SVC.
 */
void guac_rdp_free_svc(guac_rdp_svc* svc);

/**
 * Add the given SVC to the list of all available SVCs.
 */
void guac_rdp_add_svc(guac_client* client, guac_rdp_svc* svc);

/**
 * Retrieve the SVC with the given name from the list stored in the client.
 */
guac_rdp_svc* guac_rdp_get_svc(guac_client* client, const char* name);

/**
 * Remove the SVC with the given name from the list stored in the client.
 */
guac_rdp_svc* guac_rdp_remove_svc(guac_client* client, const char* name);

/**
 * Write the given blob of data to the virtual channel.
 */
void guac_rdp_svc_write(guac_rdp_svc* svc, void* data, int length);

#endif

