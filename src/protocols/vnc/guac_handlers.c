
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
 * The Original Code is libguac-client-vnc.
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

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <iconv.h>

#include <rfb/rfbclient.h>

#include <guacamole/client.h>

#include "client.h"
#include "convert.h"

int vnc_guac_client_handle_messages(guac_client* client) {

    int wait_result;
    rfbClient* rfb_client = ((vnc_guac_client_data*) client->data)->rfb_client;

    /* Initially wait for messages */
    wait_result = WaitForMessage(rfb_client, 1000000);
    while (wait_result > 0) {

        /* Handle any message received */
        if (!HandleRFBServerMessage(rfb_client)) {
            guac_client_log_error(client, "Error handling VNC server message\n");
            return 1;
        }

        /* Check for additional messages */
        wait_result = WaitForMessage(rfb_client, 0);

    }

    /* If an error occurs, log it and fail */
    if (wait_result < 0) {
        guac_client_log_error(client, "Error waiting for VNC server message\n");
        return 1;
    }

    return 0;

}


int vnc_guac_client_mouse_handler(guac_client* client, int x, int y, int mask) {

    rfbClient* rfb_client = ((vnc_guac_client_data*) client->data)->rfb_client;

    SendPointerEvent(rfb_client, x, y, mask);

    return 0;
}

int vnc_guac_client_key_handler(guac_client* client, int keysym, int pressed) {

    rfbClient* rfb_client = ((vnc_guac_client_data*) client->data)->rfb_client;

    SendKeyEvent(rfb_client, keysym, pressed);

    return 0;
}

int vnc_guac_client_clipboard_handler(guac_client* client, char* data) {

    rfbClient* rfb_client = ((vnc_guac_client_data*) client->data)->rfb_client;

    /* Convert UTF-8 character data to ISO_8859-1 */
    # if (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 2) || __GLIBC__ > 2 || _LIBICONV_VERSION >= 0x0105
    char* iso_8559_1_data = convert("UTF-8", "ISO_8859-1//TRANSLIT", data);
    #else
    char* iso_8559_1_data = convert("UTF-8", "ISO_8859-1", data);
    #endif

    /* If the conversion was successful, send the converted character data. */
    if(iso_8559_1_data) {
        SendClientCutText(rfb_client, iso_8559_1_data, strlen(iso_8559_1_data));
        free(iso_8559_1_data);
    /* Otherwise, just send an empty string. */
    } 
    else
        SendClientCutText(rfb_client, "", 0);

    return 0;
}

int vnc_guac_client_free_handler(guac_client* client) {

    vnc_guac_client_data* guac_client_data = (vnc_guac_client_data*) client->data;
    rfbClient* rfb_client = guac_client_data->rfb_client;

    /* Free encodings string, if used */
    if (guac_client_data->encodings != NULL)
        free(guac_client_data->encodings);

    /* Free generic data struct */
    free(client->data);

    /* Free memory not free'd by libvncclient's rfbClientCleanup() */
    if (rfb_client->frameBuffer != NULL) free(rfb_client->frameBuffer);
    if (rfb_client->raw_buffer != NULL) free(rfb_client->raw_buffer);
    if (rfb_client->rcSource != NULL) free(rfb_client->rcSource);

    /* Free VNC rfbClientData linked list (not free'd by rfbClientCleanup()) */
    while (rfb_client->clientData != NULL) {
        rfbClientData* next = rfb_client->clientData->next;
        free(rfb_client->clientData);
        rfb_client->clientData = next;
    }

    /* Clean up VNC client*/
    rfbClientCleanup(rfb_client);

    return 0;
}

