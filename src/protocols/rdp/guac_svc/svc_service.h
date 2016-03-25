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

#ifndef __GUAC_SVC_SERVICE_H
#define __GUAC_SVC_SERVICE_H

#include "config.h"
#include "rdp_svc.h"

#include <freerdp/utils/svc_plugin.h>

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
     * The Guacamole-specific SVC structure describing the channel this
     * instance represents.
     */
    guac_rdp_svc* svc;

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

