/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is libguac-client-rdp.
 *
 * The Initial Developer of the Original Code is
 * Michael Jumper.
 * Portions created by the Initial Developer are Copyright (C) 2010
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef __GUAC_WINPR_STREAM_COMPAT_H
#define __GUAC_WINPR_STREAM_COMPAT_H

#include <freerdp/utils/stream.h>
#include "winpr-wtypes.h"

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
#define Stream_GetPosition             stream_get_pos
#define Stream_SetPosition             stream_set_pos
#define Stream_SetPointer              stream_set_mark
#define Stream_Buffer                  stream_get_head
#define Stream_Pointer                 stream_get_tail

#define wStream                        STREAM
#define wMessage                       RDP_EVENT

wStream* Stream_New(BYTE* buffer, size_t size);
void Stream_Free(wStream* s, BOOL bFreeBuffer);

#endif

