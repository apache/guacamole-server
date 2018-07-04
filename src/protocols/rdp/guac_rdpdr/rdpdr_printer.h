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


#ifndef __GUAC_RDPDR_PRINTER_H
#define __GUAC_RDPDR_PRINTER_H

#include "config.h"

#include "rdpdr_service.h"

#ifdef ENABLE_WINPR
#include <winpr/stream.h>
#else
#include "compat/winpr-stream.h"
#endif

/**
 * Registers a new printer device within the RDPDR plugin. This must be done
 * before RDPDR connection finishes.
 * 
 * @param rdpdr
 *     The RDP device redirection plugin where the device is registered.
 * 
 * @param printer_name
 *     The name of the printer that will be registered with the RDP
 *     connection and passed through to the server.
 */
void guac_rdpdr_register_printer(guac_rdpdrPlugin* rdpdr, char* printer_name);

#endif

