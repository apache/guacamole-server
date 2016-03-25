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


#ifndef __GUAC_WINPR_STREAM_COMPAT_H
#define __GUAC_WINPR_STREAM_COMPAT_H

#include "config.h"

#include "winpr-wtypes.h"

#include <freerdp/utils/stream.h>

#include <stddef.h>

/* FreeRDP 1.0 streams */

#define Stream_Write                   stream_write
#define Stream_Write_UINT8             stream_write_uint8
#define Stream_Write_UINT16            stream_write_uint16
#define Stream_Write_UINT32            stream_write_uint32
#define Stream_Write_UINT64            stream_write_uint64

#define Stream_Read                    stream_read
#define Stream_Read_UINT8              stream_read_uint8
#define Stream_Read_UINT16             stream_read_uint16
#define Stream_Read_UINT32             stream_read_uint32
#define Stream_Read_UINT64             stream_read_uint64

#define Stream_Seek                    stream_seek
#define Stream_Seek_UINT8              stream_seek_uint8
#define Stream_Seek_UINT16             stream_seek_uint16
#define Stream_Seek_UINT32             stream_seek_uint32
#define Stream_Seek_UINT64             stream_seek_uint64

#define Stream_GetPointer              stream_get_mark
#define Stream_EnsureRemainingCapacity stream_check_size
#define Stream_Write                   stream_write
#define Stream_Zero                    stream_write_zero
#define Stream_Fill                    stream_set_byte
#define Stream_GetPosition             stream_get_pos
#define Stream_SetPosition             stream_set_pos
#define Stream_SetPointer              stream_set_mark
#define Stream_Buffer                  stream_get_head
#define Stream_Pointer                 stream_get_tail
#define Stream_Length                  stream_get_size

#define wStream                        STREAM
#define wMessage                       RDP_EVENT

wStream* Stream_New(BYTE* buffer, size_t size);
void Stream_Free(wStream* s, BOOL bFreeBuffer);

#endif

