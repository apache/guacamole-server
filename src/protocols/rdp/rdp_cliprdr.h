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


#ifndef __GUAC_RDP_RDP_CLIPRDR_H
#define __GUAC_RDP_RDP_CLIPRDR_H

#include "config.h"

#include <guacamole/client.h>

#ifdef ENABLE_WINPR
#include <winpr/stream.h>
#else
#include "compat/winpr-stream.h"
#endif

#ifdef HAVE_FREERDP_CLIENT_CLIPRDR_H
#include <freerdp/client/cliprdr.h>
#else
#include "compat/client-cliprdr.h"
#endif

/**
 * Clipboard format for text encoded in Windows CP1252.
 */
#define GUAC_RDP_CLIPBOARD_FORMAT_CP1252 1

/**
 * Clipboard format for text encoded in UTF-16.
 */
#define GUAC_RDP_CLIPBOARD_FORMAT_UTF16 2

/**
 * Called within the main RDP connection thread whenever a CLIPRDR message is
 * received. This function will dispatch that message to an appropriate
 * function, specific to that message type.
 *
 * @param client The guac_client associated with the current RDP session.
 * @param event The received CLIPRDR message.
 */
void guac_rdp_process_cliprdr_event(guac_client* client, wMessage* event);

/**
 * Handles the given CLIPRDR event, which MUST be a Monitor Ready event. It
 * is the responsibility of this function to respond to the Monitor Ready
 * event with a list of supported clipboard formats.
 *
 * @param client The guac_client associated with the current RDP session.
 *
 * @param event
 *     The received CLIPRDR message, which must be a Monitor Ready event.
 */
void guac_rdp_process_cb_monitor_ready(guac_client* client, wMessage* event);

/**
 * Handles the given CLIPRDR event, which MUST be a Format List event. It
 * is the responsibility of this function to respond to the Format List 
 * event with a request for clipboard data in one of the enumerated formats.
 * This event is fired whenever remote clipboard data is available.
 *
 * @param client The guac_client associated with the current RDP session.
 *
 * @param event
 *     The received CLIPRDR message, which must be a Format List event.
 */
void guac_rdp_process_cb_format_list(guac_client* client,
        RDP_CB_FORMAT_LIST_EVENT* event);

/**
 * Handles the given CLIPRDR event, which MUST be a Data Request event. It
 * is the responsibility of this function to respond to the Data Request
 * event with a data response containing the current clipoard contents.
 *
 * @param client The guac_client associated with the current RDP session.
 *
 * @param event
 *     The received CLIPRDR message, which must be a Data Request event.
 */
void guac_rdp_process_cb_data_request(guac_client* client,
        RDP_CB_DATA_REQUEST_EVENT* event);

/**
 * Handles the given CLIPRDR event, which MUST be a Data Response event. It
 * is the responsibility of this function to read and forward the received
 * clipboard data to connected clients.
 *
 * @param client The guac_client associated with the current RDP session.
 *
 * @param event
 *     The received CLIPRDR message, which must be a Data Response event.
 */
void guac_rdp_process_cb_data_response(guac_client* client,
        RDP_CB_DATA_RESPONSE_EVENT* event);

#endif

