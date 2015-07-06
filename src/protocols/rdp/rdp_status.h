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


#ifndef __GUAC_RDP_STATUS_H
#define __GUAC_RDP_STATUS_H

/**
 * RDP-specific status constants.
 *
 * @file rdp_status.h 
 */

#include "config.h"

/* Include any constants from winpr/file.h, if available */

#ifdef ENABLE_WINPR
#include <winpr/file.h>
#endif

/* Constants which MAY be defined within FreeRDP */

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS                  0x00000000
#define STATUS_NO_MORE_FILES            0x80000006
#define STATUS_DEVICE_OFF_LINE          0x80000010
#define STATUS_NOT_IMPLEMENTED          0xC0000002
#define STATUS_INVALID_PARAMETER        0xC000000D
#define STATUS_NO_SUCH_FILE             0xC000000F
#define STATUS_END_OF_FILE              0xC0000011
#define STATUS_ACCESS_DENIED            0xC0000022
#define STATUS_OBJECT_NAME_COLLISION    0xC0000035
#define STATUS_DISK_FULL                0xC000007F
#define STATUS_FILE_INVALID             0xC0000098  
#define STATUS_FILE_IS_A_DIRECTORY      0xC00000BA
#define STATUS_NOT_SUPPORTED            0xC00000BB
#define STATUS_NOT_A_DIRECTORY          0xC0000103
#define STATUS_TOO_MANY_OPENED_FILES    0xC000011F
#define STATUS_CANNOT_DELETE            0xC0000121
#define STATUS_FILE_DELETED             0xC0000123
#define STATUS_FILE_CLOSED              0xC0000128
#endif

/* Constants which are NEVER defined within FreeRDP */

#define STATUS_FILE_SYSTEM_LIMITATION   0xC0000427
#define STATUS_FILE_TOO_LARGE           0xC0000904

#endif
