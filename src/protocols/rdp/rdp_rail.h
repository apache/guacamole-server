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


#ifndef __GUAC_RDP_RDP_RAIL_H
#define __GUAC_RDP_RDP_RAIL_H

#include "config.h"

#include <guacamole/client.h>

#ifdef ENABLE_WINPR
#include <winpr/stream.h>
#else
#include "compat/winpr-stream.h"
#endif

/**
 * Dispatches a given RAIL event to the appropriate handler.
 *
 * @param client The guac_client associated with the current RDP session.
 * @param event The RAIL event to process.
 */
void guac_rdp_process_rail_event(guac_client* client, wMessage* event);

/**
 * Handles the event sent when updating system parameters. The event given
 * MUST be a SYSPARAM event.
 *
 * @param client The guac_client associated with the current RDP session.
 * @param event The system parameter event to process.
 */
void guac_rdp_process_rail_get_sysparam(guac_client* client, wMessage* event);

#endif

