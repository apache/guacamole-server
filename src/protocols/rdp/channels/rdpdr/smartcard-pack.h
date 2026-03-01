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

#include "channels/rdpdr/rdpdr.h"
#include "channels/rdpdr/scard.h"
#include "channels/rdpdr/remote-smartcard.h"

#include <winpr/stream.h>
#include <guacamole/client.h>

#define SMARTCARD_COMMON_TYPE_HEADER_LENGTH 8
#define SMARTCARD_PRIVATE_TYPE_HEADER_LENGTH 8

LONG guac_rdpdr_scard_unpack_common_type_header(wStream* s, guac_client* client);
LONG guac_rdpdr_scard_unpack_private_type_header(wStream* s, guac_client* client);
LONG smartcard_pack_write_size_align(wStream* s, size_t size, UINT32 alignment);
void smartcard_pack_common_type_header(wStream* s);
void smartcard_pack_private_type_header(wStream* s, UINT32 objectBufferLength);

LONG smartcard_pack_establish_context_return(wStream* s, RemoteSmartcard* smartcard);
LONG smartcard_unpack_establish_context_call(wStream* s, EstablishContext_Call* call);

LONG smartcard_pack_list_readers_return(wStream* s, const ListReaders_Return* ret, BOOL unicode);
LONG smartcard_unpack_list_readers_call(wStream* s, ListReaders_Call* call, BOOL unicode);

LONG smartcard_unpack_redir_scard_context_ref(wLog* log, wStream* s, UINT32 pbContextNdrPtr, REDIR_SCARDCONTEXT* context);
LONG smartcard_pack_redir_scard_context(wStream* s, RemoteSmartcard* smartcard, DWORD* index);
LONG smartcard_pack_redir_scard_context_ref(wStream* s, RemoteSmartcard* smartcard);

LONG smartcard_unpack_read_size_align(wStream* s, size_t size, UINT32 alignment);

LONG smartcard_unpack_get_device_type_id_call(wStream* s, GetDeviceTypeId_Call* call);
LONG smartcard_pack_device_type_id_return(wStream* s, const GetDeviceTypeId_Return* ret);

LONG smartcard_unpack_get_status_change_w_call(wStream* s, GetStatusChangeW_Call* call);
LONG smartcard_pack_get_status_change_return(wStream* s, const GetStatusChange_Return* ret, BOOL unicode);

LONG smartcard_unpack_context_call(wStream* s, Context_Call* call, const char* name);
