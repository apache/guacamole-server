
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

#ifndef __GUAC_RDPDR_MESSAGES_H
#define __GUAC_RDPDR_MESSAGES_H

#ifdef ENABLE_WINPR
#include <winpr/stream.h>
#else
#include "compat/winpr-stream.h"
#endif

#include "rdpdr_service.h"

/**
 * Identifies the "core" component of RDPDR as the destination of the received
 * packet.
 */
#define RDPDR_CTYP_CORE 0x4472

/**
 * Identifies the printing component of RDPDR as the destination of the
 * received packet.
 */
#define RDPDR_CTYP_PRN 0x5052

/*
 * Packet IDs as required by the RDP spec (see: [MS-RDPEFS].pdf)
 */

#define PAKID_CORE_SERVER_ANNOUNCE     0x496E
#define PAKID_CORE_CLIENTID_CONFIRM    0x4343
#define PAKID_CORE_CLIENT_NAME         0x434E
#define PAKID_CORE_DEVICELIST_ANNOUNCE 0x4441
#define PAKID_CORE_DEVICE_REPLY        0x6472
#define PAKID_CORE_DEVICE_IOREQUEST    0x4952
#define PAKID_CORE_DEVICE_IOCOMPLETION 0x4943
#define PAKID_CORE_SERVER_CAPABILITY   0x5350
#define PAKID_CORE_CLIENT_CAPABILITY   0x4350
#define PAKID_CORE_DEVICELIST_REMOVE   0x444D
#define PAKID_PRN_CACHE_DATA           0x5043
#define PAKID_CORE_USER_LOGGEDON       0x554C
#define PAKID_PRN_USING_XPS            0x5543

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
 * Name of the printer itself.
 */
#define GUAC_PRINTER_NAME          "G\0u\0a\0c\0a\0m\0o\0l\0e\0\0\0"
#define GUAC_PRINTER_NAME_LENGTH   20

/**
 * Name of the filesystem.
 */
#define GUAC_FILESYSTEM_NAME          "G\0u\0a\0c\0a\0m\0o\0l\0e\0\0\0"
#define GUAC_FILESYSTEM_NAME_LENGTH   20

/**
 * Label of the filesystem.
 */
#define GUAC_FILESYSTEM_LABEL          "G\0U\0A\0C\0F\0I\0L\0E\0"
#define GUAC_FILESYSTEM_LABEL_LENGTH   16

/*
 * Capability types
 */

#define CAP_GENERAL_TYPE    1
#define CAP_PRINTER_TYPE    2
#define CAP_PORT_TYPE       3
#define CAP_DRIVE_TYPE      4
#define CAP_SMARTCARD_TYPE  5

/*
 * General capability header versions.
 */

#define GENERAL_CAPABILITY_VERSION_01 1
#define GENERAL_CAPABILITY_VERSION_02 2

/*
 * Print capability header versions.
 */

#define PRINT_CAPABILITY_VERSION_01   1

/*
 * Drive capability header versions.
 */
#define DRIVE_CAPABILITY_VERSION_01   1
#define DRIVE_CAPABILITY_VERSION_02   2

/*
 * Legal client major version numbers.
 */

#define RDP_CLIENT_MAJOR_ALL 1

/*
 * Legal client minor version numbers.
 */

#define RDP_CLIENT_MINOR_6_1 0xC
#define RDP_CLIENT_MINOR_5_2 0xA
#define RDP_CLIENT_MINOR_5_1 0x5
#define RDP_CLIENT_MINOR_5_0 0x2

/*
 * PDU flags used by the extendedPDU field.
 */

#define RDPDR_DEVICE_REMOVE_PDUS  0x1
#define RDPDR_CLIENT_DISPLAY_NAME 0x2
#define RDPDR_USER_LOGGEDON_PDU   0x4

/*
 * Device types.
 */

#define RDPDR_DTYP_SERIAL     0x00000001
#define RDPDR_DTYP_PARALLEL   0x00000002
#define RDPDR_DTYP_PRINT      0x00000004
#define RDPDR_DTYP_FILESYSTEM 0x00000008
#define RDPDR_DTYP_SMARTCARD  0x00000020

/*
 * Printer flags.
 */

#define RDPDR_PRINTER_ANNOUNCE_FLAG_ASCII          0x00000001
#define RDPDR_PRINTER_ANNOUNCE_FLAG_DEFAULTPRINTER 0x00000002
#define RDPDR_PRINTER_ANNOUNCE_FLAG_NETWORKPRINTER 0x00000004
#define RDPDR_PRINTER_ANNOUNCE_FLAG_TSPRINTER      0x00000008
#define RDPDR_PRINTER_ANNOUNCE_FLAG_XPSFORMAT      0x00000010

/*
 * I/O requests.
 */

#define IRP_MJ_CREATE                   0x00000000
#define IRP_MJ_CLOSE                    0x00000002
#define IRP_MJ_READ                     0x00000003
#define IRP_MJ_WRITE                    0x00000004
#define IRP_MJ_DEVICE_CONTROL           0x0000000E
#define IRP_MJ_QUERY_VOLUME_INFORMATION 0x0000000A
#define IRP_MJ_SET_VOLUME_INFORMATION   0x0000000B
#define IRP_MJ_QUERY_INFORMATION        0x00000005
#define IRP_MJ_SET_INFORMATION          0x00000006
#define IRP_MJ_DIRECTORY_CONTROL        0x0000000C
#define IRP_MJ_LOCK_CONTROL             0x00000011

#define IRP_MN_QUERY_DIRECTORY         0x00000001
#define IRP_MN_NOTIFY_CHANGE_DIRECTORY 0x00000002

/*
 * Status constants.
 */
#define STATUS_SUCCESS                  0x00000000
#define STATUS_NO_MORE_FILES            0x80000006
#define STATUS_DEVICE_OFF_LINE          0x80000010
#define STATUS_INVALID_PARAMETER        0xC000000D
#define STATUS_NO_SUCH_FILE             0xC000000F
#define STATUS_END_OF_FILE              0xC0000011
#define STATUS_ACCESS_DENIED            0xC0000022
#define STATUS_FILE_INVALID             0xC0000098  
#define STATUS_FILE_IS_A_DIRECTORY      0xC00000BA
#define STATUS_TOO_MANY_OPENED_FILES    0xC000011F
#define STATUS_CANNOT_DELETE            0xC0000121
#define STATUS_FILE_DELETED             0xC0000123
#define STATUS_FILE_CLOSED              0xC0000128
#define STATUS_FILE_SYSTEM_LIMITATION   0xC0000427
#define STATUS_FILE_TOO_LARGE           0xC0000904

/*
 * Volume information constants.
 */

#define FileFsVolumeInformation    0x00000001
#define FileFsSizeInformation      0x00000003
#define FileFsDeviceInformation    0x00000004 
#define FileFsAttributeInformation 0x00000005 
#define FileFsFullSizeInformation  0x00000007 

/*
 * File information constants.
 */

#define FileBasicInformation        0x00000004
#define FileStandardInformation     0x00000005
#define FileRenameInformation       0x0000000A
#define FileDispositionInformation  0x0000000D
#define FileAllocationInformation   0x00000013
#define FileEndOfFileInformation    0x00000014
#define FileAttributeTagInformation 0x00000023 

/*
 * Directory information constants.
 */

#define FileDirectoryInformation     0x00000001
#define FileFullDirectoryInformation 0x00000002
#define FileBothDirectoryInformation 0x00000003
#define FileNamesInformation         0x0000000C

/*
 * Message handlers.
 */

void guac_rdpdr_process_server_announce(guac_rdpdrPlugin* rdpdr, wStream* input_stream);
void guac_rdpdr_process_clientid_confirm(guac_rdpdrPlugin* rdpdr, wStream* input_stream);
void guac_rdpdr_process_device_reply(guac_rdpdrPlugin* rdpdr, wStream* input_stream);
void guac_rdpdr_process_device_iorequest(guac_rdpdrPlugin* rdpdr, wStream* input_stream);
void guac_rdpdr_process_server_capability(guac_rdpdrPlugin* rdpdr, wStream* input_stream);
void guac_rdpdr_process_user_loggedon(guac_rdpdrPlugin* rdpdr, wStream* input_stream);
void guac_rdpdr_process_prn_cache_data(guac_rdpdrPlugin* rdpdr, wStream* input_stream);
void guac_rdpdr_process_prn_using_xps(guac_rdpdrPlugin* rdpdr, wStream* input_stream);

#endif

