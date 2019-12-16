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

#ifndef GUAC_RDP_SVC_H
#define GUAC_RDP_SVC_H

#include "config.h"

#include <freerdp/svc.h>
#include <guacamole/client.h>
#include <guacamole/stream.h>
#include <winpr/wtsapi.h>

/**
 * The maximum number of bytes to allow within each channel name, including
 * null terminator.
 */
#define GUAC_RDP_SVC_MAX_LENGTH 8

/**
 * Structure describing a static virtual channel, and the corresponding
 * Guacamole pipes and FreeRDP resources.
 */
typedef struct guac_rdp_svc {

    /**
     * Reference to the client owning this static channel.
     */
    guac_client* client;

    /**
     * The output pipe, opened when the RDP server receives a connection to
     * the static channel.
     */
    guac_stream* output_pipe;

    /**
     * The definition of this static virtual channel, including its name. The
     * name of the SVC is also used as the name of the associated Guacamole
     * pipe streams.
     */
    CHANNEL_DEF channel_def;

    /**
     * Functions and data specific to the FreeRDP side of the virtual channel
     * and plugin.
     */
    CHANNEL_ENTRY_POINTS_FREERDP_EX entry_points;

    /**
     * Handle which identifies the client connection, typically referred to
     * within the FreeRDP source as pInitHandle. This handle is provided to the
     * channel entry point and the channel init event handler. The handle must
     * eventually be used within the channel open event handler to obtain a
     * handle to the channel itself.
     */
    PVOID init_handle;

    /**
     * Handle which identifies the channel itself, typically referred to within
     * the FreeRDP source as OpenHandle. This handle is obtained through a call
     * to entry_points.pVirtualChannelOpenEx() in response to receiving a
     * CHANNEL_EVENT_CONNECTED event via the init event handler.
     *
     * Data is received in CHANNEL_EVENT_DATA_RECEIVED events via the open
     * event handler, and data is written through calls to
     * entry_points.pVirtualChannelWriteEx().
     */
    DWORD open_handle;

} guac_rdp_svc;

/**
 * Initializes arbitrary static virtual channel (SVC) support for RDP, loading
 * a new instance of Guacamole's arbitrary SVC plugin for FreeRDP ("guacsvc")
 * supporting the channel having the given name. Data sent from within the RDP
 * session using this channel will be sent along an identically-named pipe
 * stream to the Guacamole client, and data sent along a pipe stream having the
 * same name will be written to the SVC and received within the RDP session. If
 * failures occur while loading the plugin, messages noting the specifics of
 * those failures will be logged, and support for the given channel will not be
 * functional.
 *
 * This MUST be called within the PreConnect callback of the freerdp instance
 * for static virtual channel support to be loaded.
 *
 * @param rdpContext
 *     The rdpContext associated with the FreeRDP side of the RDP connection.
 *
 * @param name
 *     The name of the SVC which should be handled by the new instance of the
 *     plugin.
 */
void guac_rdp_svc_load_plugin(rdpContext* context, char* name);

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
 * Add the given SVC to the list of all available SVCs. This function must be
 * invoked after the SVC is connected for inbound pipe streams having that
 * SVC's name to result in received data being sent into the RDP session.
 *
 * @param client
 *     The guac_client associated with the current RDP session.
 *
 * @param svc
 *     The static virtual channel to add to the list of all such channels
 *     available.
 */
void guac_rdp_svc_add(guac_client* client, guac_rdp_svc* svc);

/**
 * Retrieve the SVC with the given name from the list stored in the client. The
 * requested SVC must previously have been added using guac_rdp_svc_add().
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
guac_rdp_svc* guac_rdp_svc_get(guac_client* client, const char* name);

/**
 * Removes the SVC with the given name from the list stored in the client.
 * Inbound pipe streams having the given name will no longer be routed to the
 * associated SVC.
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
guac_rdp_svc* guac_rdp_svc_remove(guac_client* client, const char* name);

/**
 * Writes the given blob of data to the virtual channel such that it can be
 * received within the RDP session.
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

/**
 * Handler for "blob" instructions which automatically writes received data to
 * the associated SVC using guac_rdp_svc_write().
 */
guac_user_blob_handler guac_rdp_svc_blob_handler;

/**
 * Handler for "pipe" instructions which automatically prepares received pipe
 * streams to automatically write received blobs to the SVC having the same
 * name as the pipe stream. Received pipe streams are associated with the
 * relevant guac_rdp_svc instance and the SVC-specific "blob" instructino
 * handler (guac_rdp_svc_blob_handler).
 */
guac_user_pipe_handler guac_rdp_svc_pipe_handler;

#endif

