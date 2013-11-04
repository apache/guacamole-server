
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
 * Portions created by the Initial Developer are Copyright (C) 2011
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

#ifndef __GUAC_RDP_STATUS_H
#define __GUAC_RDP_STATUS_H

/**
 * RDP-specific status constants.
 *
 * @file rdp_status.h 
 */

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
#define STATUS_FILE_SYSTEM_LIMITATION   0xC0000427
#define STATUS_FILE_TOO_LARGE           0xC0000904

#endif

