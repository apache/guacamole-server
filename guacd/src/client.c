
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

#include <stdlib.h>

#include <guacamole/guacio.h>
#include <guacamole/client.h>
#include <guacamole/log.h>

#include "client.h"
#include "thread.h"

void guac_client_stop(guac_client* client) {
    client->state = STOPPING;
}

void* __guac_client_output_thread(void* data) {

    guac_client* client = (guac_client*) data;
    GUACIO* io = client->io;

    guac_timestamp last_ping_timestamp = guac_current_timestamp();

    /* Guacamole client output loop */
    while (client->state == RUNNING) {

        /* Occasionally ping client with repeat of last sync */
        guac_timestamp timestamp = guac_current_timestamp();
        if (timestamp - last_ping_timestamp > GUAC_SYNC_FREQUENCY) {
            last_ping_timestamp = timestamp;
            if (
                   guac_send_sync(io, client->last_sent_timestamp)
                || guac_flush(io)
               ) {
                guac_client_stop(client);
                return NULL;
            }
        }

        /* Handle server messages */
        if (client->handle_messages) {

            /* Get previous GUACIO state */
            int last_total_written = io->total_written;

            /* Only handle messages if synced within threshold */
            if (client->last_sent_timestamp - client->last_received_timestamp
                    < GUAC_SYNC_THRESHOLD) {

                int retval = client->handle_messages(client);
                if (retval) {
                    guac_log_error("Error handling server messages");
                    guac_client_stop(client);
                    return NULL;
                }

                /* If data was written during message handling */
                if (io->total_written != last_total_written) {

                    /* Sleep as necessary */
                    guac_sleep(GUAC_SERVER_MESSAGE_HANDLE_FREQUENCY);

                    /* Send sync instruction */
                    client->last_sent_timestamp = guac_current_timestamp();
                    if (guac_send_sync(io, client->last_sent_timestamp)) {
                        guac_client_stop(client);
                        return NULL;
                    }

                }

                if (guac_flush(io)) {
                    guac_client_stop(client);
                    return NULL;
                }

            }

            /* If sync threshold exceeded, don't spin waiting for resync */
            else
                guac_sleep(GUAC_SERVER_MESSAGE_HANDLE_FREQUENCY);

        }

        /* If no message handler, just sleep until next sync ping */
        else
            guac_sleep(GUAC_SYNC_FREQUENCY);

    } /* End of output loop */

    guac_client_stop(client);
    return NULL;

}

void* __guac_client_input_thread(void* data) {

    guac_client* client = (guac_client*) data;
    GUACIO* io = client->io;

    guac_instruction instruction;

    /* Guacamole client input loop */
    while (client->state == RUNNING && guac_read_instruction(io, GUAC_USEC_TIMEOUT, &instruction) > 0) {

        /* Call handler, stop on error */
        if (guac_client_handle_instruction(client, &instruction) < 0) {
            guac_free_instruction_data(&instruction);
            break;
        }

        /* Free allocate instruction data */
        guac_free_instruction_data(&instruction);

    }

    guac_client_stop(client);
    return NULL;

}

int guac_start_client(guac_client* client) {

    guac_thread input_thread, output_thread;

    if (guac_thread_create(&output_thread, __guac_client_output_thread, (void*) client)) {
        return -1;
    }

    if (guac_thread_create(&input_thread, __guac_client_input_thread, (void*) client)) {
        guac_client_stop(client);
        guac_thread_join(output_thread);
        return -1;
    }

    /* Wait for I/O threads */
    guac_thread_join(input_thread);
    guac_thread_join(output_thread);

    /* Done */
    return 0;

}


