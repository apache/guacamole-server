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

#ifndef GUAC_SPICE_CLIPBOARD_H
#define GUAC_SPICE_CLIPBOARD_H

#include "config.h"

#include <guacamole/client.h>
#include <guacamole/user.h>

#include <spice-client-glib-2.0/spice-client.h>

/**
 * Handler for inbound clipboard data from Guacamole users.
 */
guac_user_clipboard_handler guac_spice_clipboard_handler;

/**
 * Handler for stream data related to clipboard.
 */
guac_user_blob_handler guac_spice_clipboard_blob_handler;

/**
 * Handler for end-of-stream related to clipboard.
 */
guac_user_end_handler guac_spice_clipboard_end_handler;

/**
 * A handler that will be registered with the Spice client to handle clipboard
 * data sent from the Spice server to the client.
 * 
 * @param channel
 *     The main Spice channel on which this event was fired.
 * 
 * @param selection
 *     The clipboard on which the selection occurred.
 * 
 * @param type
 *     The type of the data that is on the clipboard.
 * 
 * @param data
 *     A pointer to the location containing the data that is on the clipboard.
 * 
 * @param size
 *     The amount of data in bytes.
 * 
 * @param client
 *     The guac_client associated with this event handler, passed when the
 *     handler was registered.
 */
void guac_spice_clipboard_selection_handler(SpiceMainChannel* channel,
        guint selection, guint type, gpointer data, guint size,
        guac_client* client);

/**
 * A handler that will be registered with the Spice client to handle clipboard
 * events where the guest (vdagent) within the Spice server notifies the client
 * that data is available on the clipboard.
 * 
 * @param channel
 *     The main SpiceChannel on which this event is fired.
 * 
 * @param selection
 *     The Spice clipboard from which the event is fired.
 * 
 * @param types
 *     The type of data being sent by the agent.
 * 
 * @param ntypes
 *     The number of data types represented.
 * 
 * @param client
 *     The guac_client that was passed in when the callback was registered.
 */
void guac_spice_clipboard_selection_grab_handler(SpiceMainChannel* channel,
        guint selection, guint32* types, guint ntypes, guac_client* client);

/**
 * A handler that will be called by the Spice client when the Spice server
 * is done with the clipboard and releases control of it. 
 * 
 * @param chennl
 *     The main Spice channel on which this event is fired.
 * 
 * @param selection
 *     The Spice server clipboard releasing control.
 * 
 * @param client
 *     The guac_client that was registered with the callback.
 */
void guac_spice_clipboard_selection_release_handler(SpiceMainChannel* channel,
        guint selection, guac_client* client);

/**
 * A handler that will be called by the Spice client when the Spice server
 * would like to check and receive the contents of the client's clipboard.
 * 
 * @param channel
 *     The main Spice channel on which this event is fired.
 * 
 * @param selection
 *     The Spice server clipboard that is requesting data.
 * 
 * @param type
 *     The type of data to be sent to the Spice server.
 * 
 * @param client
 *     The guac_client object that was registered with the callback.
 */
void guac_spice_clipboard_selection_request_handler(SpiceMainChannel* channel,
        guint selection, guint type, guac_client* client);

#endif /* GUAC_SPICE_CLIPBOARD_H */

