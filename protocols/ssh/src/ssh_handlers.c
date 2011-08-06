
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
 * The Original Code is libguac-client-ssh.
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

#include <stdlib.h>
#include <string.h>

#include <libssh/libssh.h>

#include <cairo/cairo.h>
#include <pango/pangocairo.h>

#include <guacamole/log.h>
#include <guacamole/guacio.h>
#include <guacamole/protocol.h>
#include <guacamole/client.h>

#include "ssh_handlers.h"
#include "ssh_client.h"

int ssh_guac_client_handle_messages(guac_client* client) {

    GUACIO* io = client->io;
    ssh_guac_client_data* client_data = (ssh_guac_client_data*) client->data;
    char buffer[8192];

    ssh_channel read_channels[2];
    struct timeval timeout;

    /* Channels to read */
    read_channels[0] = client_data->term_channel;
    read_channels[1] = NULL;

    /* Time to wait */
    timeout.tv_sec = GUAC_SYNC_FREQUENCY / 1000;
    timeout.tv_usec = (GUAC_SYNC_FREQUENCY % 1000) * 1000;

    /* Wait for data to be available */
    if (channel_select(read_channels, NULL, NULL, &timeout) == SSH_OK) {

        int bytes_read = 0;

        /* While data available, write to terminal */
        while (channel_is_open(client_data->term_channel)
                && !channel_is_eof(client_data->term_channel)
                && (bytes_read = channel_read_nonblocking(client_data->term_channel, buffer, sizeof(buffer), 0)) > 0) {

            ssh_guac_terminal_write(client_data->term, buffer, bytes_read);
            guac_flush(io);

        }

        /* Notify on error */
        if (bytes_read < 0) {
            guac_send_error(io, "Error reading data.");
            guac_flush(io);
            return 1;
        }
    }

    return 0;

}

int ssh_guac_client_key_handler(guac_client* client, int keysym, int pressed) {

    ssh_guac_client_data* client_data = (ssh_guac_client_data*) client->data;

    /* If key pressed */
    if (pressed) {

        char data;

        /* If simple ASCII key */
        if (keysym >= 0x00 && keysym <= 0xFF)
            data = (char) keysym;

        else if (keysym == 0xFF08) data = 0x08;
        else if (keysym == 0xFF09) data = 0x09;
        else if (keysym == 0xFF0D) data = 0x0D;
        else if (keysym == 0xFF1B) data = 0x1B;

        else
            return 0;

        return channel_write(client_data->term_channel, &data, 1);

    }

    return 0;

}

