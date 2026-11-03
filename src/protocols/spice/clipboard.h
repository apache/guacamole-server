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

#include <guacamole/client.h>
#include <guacamole/user.h>
#include <spice-client.h>

/**
 * Connects the clipboard-related signal handlers of the given SPICE main
 * channel, enabling clipboard exchange (via the SPICE guest agent) between the
 * Guacamole client and the remote desktop.
 *
 * @param client
 *     The guac_client associated with the SPICE connection.
 *
 * @param channel
 *     The SPICE main channel to handle.
 */
void guac_spice_clipboard_connect(guac_client* client,
        SpiceMainChannel* channel);

/**
 * Handler for inbound (client-to-server) clipboard streams.
 *
 * @param user
 *     The user who is sending clipboard data.
 *
 * @param stream
 *     The stream through which the clipboard data will be received.
 *
 * @param mimetype
 *     The mimetype of the clipboard data that will be sent along the stream.
 *
 * @return
 *     Always zero.
 */
guac_user_clipboard_handler guac_spice_clipboard_handler;

/**
 * Handler for blobs of inbound clipboard data.
 *
 * @param user
 *     The user who is sending clipboard data.
 *
 * @param stream
 *     The stream along which the blob was received.
 *
 * @param data
 *     The received clipboard data.
 *
 * @param length
 *     The number of bytes in the data buffer.
 *
 * @return
 *     Always zero.
 */
guac_user_blob_handler guac_spice_clipboard_blob_handler;

/**
 * Handler for the end of an inbound clipboard stream.
 *
 * @param user
 *     The user who sent the clipboard data.
 *
 * @param stream
 *     The stream that has ended.
 *
 * @return
 *     Always zero.
 */
guac_user_end_handler guac_spice_clipboard_end_handler;

#endif
