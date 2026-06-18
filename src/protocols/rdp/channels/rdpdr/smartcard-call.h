/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * Smart Card Structure Packing
 *
 * Copyright 2025 Armin Novak <armin.novak@thincast.com>
 * Copyright 2025 Thincast Technologies GmbH
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "channels/rdpdr/rdpdr-smartcard.h"
#include "channels/rdpdr/remote-smartcard.h"

#include <winpr/stream.h>

typedef struct s_scard_call_context
{
    wHashTable* rgSCardContextList;
    RemoteSmartcard* smartcard;
    void* userdata;
    wLinkedList* names;
} scard_call_context;

LONG guac_rdpdr_smartcard_irp_device_control_call(guac_rdp_common_svc* svc,
                                                  scard_call_context* ctx,
                                                  guac_rdpdr_iorequest* request,
                                                  guac_rdp_scard_operation* op,
                                                  NTSTATUS* io_status);
