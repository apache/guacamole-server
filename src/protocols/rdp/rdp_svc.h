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

#ifndef __GUAC_RDP_RDP_SVC_H
#define __GUAC_RDP_RDP_SVC_H

#include "config.h"

#include <freerdp/utils/svc_plugin.h>
#include <guacamole/client.h>
#include <guacamole/stream.h>

/**
 * The maximum number of bytes to allow within each channel name, including
 * null terminator.
 */
#define GUAC_RDP_SVC_MAX_LENGTH 8

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
    char name[GUAC_RDP_SVC_MAX_LENGTH];

    /**
     * The output pipe, opened when the RDP server receives a connection to
     * the static channel.
     */
    guac_stream* output_pipe;

} guac_rdp_svc;

/**
 * Allocate a new SVC with the given name.
 *
 * @param client
 *     The guac_client associated with the current RDP session.
 *
 * @param name
 *     The name of the virtual channel to allocate.
 *
 * @return
 *     A newly-allocated static virtual channel.
 */
guac_rdp_svc* guac_rdp_alloc_svc(guac_client* client, char* name);

/**
 * Free the given SVC.
 *
 * @param svc
 *     The static virtual channel to free.
 */
void guac_rdp_free_svc(guac_rdp_svc* svc);

/**
 * Sends the "pipe" instruction describing the given static virtual channel
 * along the given socket. This pipe instruction will relate the SVC's
 * underlying output stream with the SVC's name and the mimetype
 * "application/octet-stream".
 *
 * @param socket
 *     The socket along which the "pipe" instruction should be sent.
 *
 * @param svc
 *     The static virtual channel that the "pipe" instruction should describe.
 */
void guac_rdp_svc_send_pipe(guac_socket* socket, guac_rdp_svc* svc);

/**
 * Sends the "pipe" instructions describing all static virtual channels
 * available to the given user along that user's socket. Each pipe instruction
 * will relate the associated SVC's underlying output stream with the SVC's
 * name and the mimetype "application/octet-stream".
 *
 * @param user
 *     The user to send the "pipe" instructions to.
 */
void guac_rdp_svc_send_pipes(guac_user* user);

/**
 * Add the given SVC to the list of all available SVCs.
 *
 * @param client
 *     The guac_client associated with the current RDP session.
 *
 * @param svc
 *     The static virtual channel to add to the list of all such channels
 *     available.
 */
void guac_rdp_add_svc(guac_client* client, guac_rdp_svc* svc);

/**
 * Retrieve the SVC with the given name from the list stored in the client.
 *
 * @param client
 *     The guac_client associated with the current RDP session.
 *
 * @param name
 *     The name of the static virtual channel to retrieve.
 *
 * @return
 *     The static virtual channel with the given name, or NULL if no such
 *     virtual channel exists.
 */
guac_rdp_svc* guac_rdp_get_svc(guac_client* client, const char* name);

/**
 * Remove the SVC with the given name from the list stored in the client.
 *
 * @param client
 *     The guac_client associated with the current RDP session.
 *
 * @param name
 *     The name of the static virtual channel to remove.
 *
 * @return
 *     The static virtual channel that was removed, or NULL if no such virtual
 *     channel exists.
 */
guac_rdp_svc* guac_rdp_remove_svc(guac_client* client, const char* name);

/**
 * Write the given blob of data to the virtual channel.
 *
 * @param svc
 *     The static virtual channel to write data to.
 *
 * @param data
 *     The data to write.
 *
 * @param length
 *     The number of bytes to write.
 */
void guac_rdp_svc_write(guac_rdp_svc* svc, void* data, int length);

#endif

