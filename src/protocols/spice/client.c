
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
 * The Original Code is libguac-client-spice.
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
#include <stdbool.h>

#include <guacamole/client.h>
#include <spice-session.h>

#include "guac_handlers.h"

/* Client plugin arguments */
const char* GUAC_CLIENT_ARGS[] = {
    "hostname",
    "port",
    NULL
};

enum __SPICE_ARGS_IDX {
    SPICE_ARGS_HOSTNAME,
    SPICE_ARGS_PORT,
    SPICE_ARGS_COUNT
};

int guac_client_init(guac_client* client, int argc, char** argv) {

    /* STUB */

    GMainLoop* mainloop;
    SpiceSession* session;

    if (argc != SPICE_ARGS_COUNT) {
        guac_client_log_error(client, "Wrong number of arguments");
        return -1;
    }

    /* Init GLIB */
    guac_client_log_info(client, "Init GLIB-2.0...");
    g_type_init();
    mainloop = g_main_loop_new(NULL, false);

    /* Create session */
    guac_client_log_info(client, "Creating SPICE session...");

    session = spice_session_new();

    /* Init session parameters */
    guac_client_log_info(client, "Setting parameters...");
    g_object_set(session, "host", argv[SPICE_ARGS_HOSTNAME], NULL);
    g_object_set(session, "port", argv[SPICE_ARGS_PORT], NULL);

    /* Connect */
    guac_client_log_info(client, "Connecting...");
    if (!spice_session_connect(session)) {
        guac_client_log_error(client, "SPICE connection failed");
        return 1;
    }

    /* Set handlers */
    client->handle_messages   = spice_guac_client_handle_messages;
    client->clipboard_handler = spice_guac_client_clipboard_handler;
    client->key_handler       = spice_guac_client_key_handler;
    client->mouse_handler     = spice_guac_client_mouse_handler;
    client->size_handler      = spice_guac_client_size_handler;
    client->free_handler      = spice_guac_client_free_handler;

    g_main_loop_run(mainloop);
    return 0;
}

