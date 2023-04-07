
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

#ifndef GUAC_DRV_CRTC_H
#define GUAC_DRV_CRTC_H

#include "config.h"

#include <xorg-server.h>
#include <xf86.h>
#include <xf86Crtc.h>

extern xf86CrtcConfigFuncsRec guac_drv_crtc_configfuncs;

extern xf86CrtcFuncsRec guac_drv_crtc_funcs;

extern xf86OutputFuncsRec guac_drv_output_funcs;

#endif

