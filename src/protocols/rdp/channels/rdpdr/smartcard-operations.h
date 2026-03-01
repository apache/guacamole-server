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

#include <winpr/stream.h>

void smartcard_operation_free(guac_rdp_scard_operation* op, BOOL allocated);

LONG guac_rdpdr_smartcard_irp_device_control_decode(wStream* input_stream,
                                                    UINT32 CompletionId,
                                                    UINT32 FileId,
                                                    guac_rdp_scard_operation* operation);
