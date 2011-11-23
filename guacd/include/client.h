
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
 * The Original Code is guacd.
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


#ifndef _GUACD_CLIENT_H
#define _GUACD_CLIENT_H

/**
 * Provides functions and structures required for defining (and handling) a proxy client.
 *
 * @file client.h
 */

/**
 * The time to allow between sync responses in milliseconds. If a sync
 * instruction is sent to the client and no response is received within this
 * timeframe, server messages will not be handled until a sync instruction is
 * received from the client.
 */
#define GUAC_SYNC_THRESHOLD 500

/**
 * The time to allow between server sync messages in milliseconds. A sync
 * message from the server will be sent every GUAC_SYNC_FREQUENCY milliseconds.
 * As this will induce a response from a client that is not malfunctioning,
 * this is used to detect when a client has died. This must be set to a
 * reasonable value to avoid clients being disconnected unnecessarily due
 * to timeout.
 */
#define GUAC_SYNC_FREQUENCY 5000

/**
 * The amount of time to wait after handling server messages. If a client
 * plugin has a message handler, and sends instructions when server messages
 * are being handled, there will be a pause of this many milliseconds before
 * the next call to the message handler.
 */
#define GUAC_SERVER_MESSAGE_HANDLE_FREQUENCY 50

/**
 * The number of milliseconds to wait for messages in any phase before
 * timing out and closing the connection with an error.
 */
#define GUAC_TIMEOUT      15000

/**
 * The number of microseconds to wait for messages in any phase before
 * timing out and closing the conncetion with an error. This is always
 * equal to GUAC_TIMEOUT * 1000.
 */
#define GUAC_USEC_TIMEOUT (GUAC_TIMEOUT*1000)


void guac_client_stop(guac_client* client);
int guac_start_client(guac_client* client);

#endif
