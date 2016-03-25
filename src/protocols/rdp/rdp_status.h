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
