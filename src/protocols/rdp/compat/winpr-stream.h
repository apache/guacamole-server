/*
 * Copyright (C) 2013 Glyptodon LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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

