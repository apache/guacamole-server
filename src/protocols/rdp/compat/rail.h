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


#ifndef __GUAC_RAIL_COMPAT_H
#define __GUAC_RAIL_COMPAT_H

#include "config.h"

#include <freerdp/rail.h>

#define RailChannel_Class                  RDP_EVENT_CLASS_RAIL
#define RailChannel_ClientSystemParam      RDP_EVENT_TYPE_RAIL_CLIENT_SET_SYSPARAMS
#define RailChannel_GetSystemParam         RDP_EVENT_TYPE_RAIL_CHANNEL_GET_SYSPARAMS
#define RailChannel_ServerExecuteResult    RDP_EVENT_TYPE_RAIL_CHANNEL_EXEC_RESULTS
#define RailChannel_ServerSystemParam      RDP_EVENT_TYPE_RAIL_CHANNEL_SERVER_SYSPARAM
#define RailChannel_ServerMinMaxInfo       RDP_EVENT_TYPE_RAIL_CHANNEL_SERVER_MINMAXINFO
#define RailChannel_ServerLocalMoveSize    RDP_EVENT_TYPE_RAIL_CHANNEL_SERVER_LOCALMOVESIZE
#define RailChannel_ServerGetAppIdResponse RDP_EVENT_TYPE_RAIL_CHANNEL_APPID_RESP
#define RailChannel_ServerLanguageBarInfo  RDP_EVENT_TYPE_RAIL_CHANNEL_LANGBARINFO

#endif

