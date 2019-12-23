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

#ifndef GUAC_RDP_CHANNELS_RDPDR_MESSAGES_H
#define GUAC_RDP_CHANNELS_RDPDR_MESSAGES_H

#include "config.h"
#include "channels/rdpdr/rdpdr.h"

#include <winpr/stream.h>

#include <stdint.h>

/**
 * A 32-bit arbitrary value for the osType field of certain requests. As this
 * value is defined as completely arbitrary and required to be ignored by the
 * server, we send "GUAC" as an integer.
 */
#define GUAC_OS_TYPE (*((uint32_t*) "GUAC"))

/**
 * Name of the printer driver that should be used on the server.
 */
#define GUAC_PRINTER_DRIVER        "M\0S\0 \0P\0u\0b\0l\0i\0s\0h\0e\0r\0 \0I\0m\0a\0g\0e\0s\0e\0t\0t\0e\0r\0\0\0"
#define GUAC_PRINTER_DRIVER_LENGTH 50

/**
 * Label of the filesystem.
 */
#define GUAC_FILESYSTEM_LABEL          "G\0U\0A\0C\0F\0I\0L\0E\0"
#define GUAC_FILESYSTEM_LABEL_LENGTH   16

/*
 * Message handlers.
 */

void guac_rdpdr_process_server_announce(guac_rdp_common_svc* svc, wStream* input_stream);
void guac_rdpdr_process_clientid_confirm(guac_rdp_common_svc* svc, wStream* input_stream);
void guac_rdpdr_process_device_reply(guac_rdp_common_svc* svc, wStream* input_stream);
void guac_rdpdr_process_device_iorequest(guac_rdp_common_svc* svc, wStream* input_stream);
void guac_rdpdr_process_server_capability(guac_rdp_common_svc* svc, wStream* input_stream);
void guac_rdpdr_process_user_loggedon(guac_rdp_common_svc* svc, wStream* input_stream);
void guac_rdpdr_process_prn_cache_data(guac_rdp_common_svc* svc, wStream* input_stream);
void guac_rdpdr_process_prn_using_xps(guac_rdp_common_svc* svc, wStream* input_stream);

#endif

