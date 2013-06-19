
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

/*
 * Message handlers.
 */

void guac_rdpdr_process_server_announce(guac_rdpdrPlugin* rdpdr, STREAM* input_stream);

#endif

