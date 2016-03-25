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
 * @param client
 *     The guac_client associated with the current RDP session.
 *
 * @param event
 *     The RAIL event to process.
 */
void guac_rdp_process_rail_event(guac_client* client, wMessage* event);

/**
 * Handles the event sent when updating system parameters. The event given
 * MUST be a SYSPARAM event.
 *
 * @param client
 *     The guac_client associated with the current RDP session.
 *
 * @param event
 *     The system parameter event to process.
 */
void guac_rdp_process_rail_get_sysparam(guac_client* client, wMessage* event);

#endif

