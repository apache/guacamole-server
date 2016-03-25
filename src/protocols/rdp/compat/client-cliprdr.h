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


#ifndef __GUAC_CLIENT_CLIPRDR_COMPAT_H
#define __GUAC_CLIENT_CLIPRDR_COMPAT_H

#include "config.h"

#include <freerdp/plugins/cliprdr.h>

#define CliprdrChannel_Class        RDP_EVENT_CLASS_CLIPRDR
#define CliprdrChannel_FormatList   RDP_EVENT_TYPE_CB_FORMAT_LIST
#define CliprdrChannel_MonitorReady RDP_EVENT_TYPE_CB_MONITOR_READY
#define CliprdrChannel_DataRequest  RDP_EVENT_TYPE_CB_DATA_REQUEST
#define CliprdrChannel_DataResponse RDP_EVENT_TYPE_CB_DATA_RESPONSE

#endif

