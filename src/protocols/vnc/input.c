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

#include "display.h"
#include "input.h"
#include "vnc.h"

#include <guacamole/display.h>
#include <guacamole/recording.h>
#include <guacamole/user.h>
#include <rfb/rfbclient.h>

int guac_vnc_user_mouse_handler(guac_user* user, int x, int y, int mask) {

    guac_client* client = user->client;
    guac_vnc_client* vnc_client = (guac_vnc_client*) client->data;
    rfbClient* rfb_client = vnc_client->rfb_client;

    /* Store current mouse location/state */
    guac_display_render_thread_notify_user_moved_mouse(vnc_client->render_thread, user, x, y, mask);

    /* Report mouse position within recording */
    if (vnc_client->recording != NULL)
        guac_recording_report_mouse(vnc_client->recording, x, y, mask);

    /* Send VNC event only if finished connecting */
    if (rfb_client != NULL)
        SendPointerEvent(rfb_client, x, y, mask);

    return 0;
}

/**
 * Translates the keysyms that Guacamole uses to represent platform-specific
 * keys into the keysyms that established VNC clients conventionally send for
 * those keys.
 *
 * @param keysym
 *     The keysym received from the Guacamole client.
 *
 * @return
 *     The keysym that should be sent to the VNC server.
 */
static int guac_vnc_translate_keysym(int keysym) {

    switch (keysym) {

        /* Command is conventionally sent as Super by VNC clients */
        case 0xFFE7: return 0xFFEB; /* Command_L (Meta_L) -> Super_L */
        case 0xFFE8: return 0xFFEC; /* Command_R (Meta_R) -> Super_R */

        /* Option is conventionally sent as Alt by VNC clients */
        case 0xFFED: return 0xFFE9; /* Option_L (Hyper_L) -> Alt_L */
        case 0xFFEE: return 0xFFEA; /* Option_R (Hyper_R) -> Alt_R */

    }

    return keysym;

}

/**
 * Toggles the lock controlled by the given lock keysym within the VNC session
 * by pressing and releasing that key. The key will be pressed/released only if
 * the given lock flag is set within the reported lock key state.
 *
 * @param rfb_client
 *     The VNC client to send key events through.
 *
 * @param led_state
 *     The lock key state reported by the VNC server, as a bitmask of
 *     rfbKeyboardMask* flags.
 *
 * @param led_mask
 *     The single rfbKeyboardMask* flag to test.
 *
 * @param keysym
 *     The keysym of the lock key that toggles the tested lock.
 */
static void guac_vnc_release_lock(rfbClient* rfb_client, int led_state,
        int led_mask, int keysym) {
    if (led_state & led_mask) {
        SendKeyEvent(rfb_client, keysym, TRUE);
        SendKeyEvent(rfb_client, keysym, FALSE);
    }
}

void guac_vnc_keyboard_led_state(rfbClient* rfb_client, int state, int pad) {

    guac_client* client = rfbClientGetClientData(rfb_client, GUAC_VNC_CLIENT_KEY);
    guac_vnc_client* vnc_client = (guac_vnc_client*) client->data;

    /* Clear lock states once we know which locks need to be cleared */
    if (!vnc_client->lock_state_synced) {
        guac_vnc_release_lock(rfb_client, state, rfbKeyboardMaskCapsLock,   0xFFE5 /* Caps_Lock */);
        guac_vnc_release_lock(rfb_client, state, rfbKeyboardMaskNumLock,    0xFF7F /* Num_Lock */);
        guac_vnc_release_lock(rfb_client, state, rfbKeyboardMaskScrollLock, 0xFF14 /* Scroll_Lock */);
        vnc_client->lock_state_synced = 1;
    }

}

int guac_vnc_user_key_handler(guac_user* user, int keysym, int pressed) {

    guac_vnc_client* vnc_client = (guac_vnc_client*) user->client->data;
    rfbClient* rfb_client = vnc_client->rfb_client;

    /* Report key state within recording */
    if (vnc_client->recording != NULL)
        guac_recording_report_key(vnc_client->recording,
                keysym, pressed);

    /* Send VNC event only if finished connecting */
    if (rfb_client != NULL) {

        /* Ensure the lock key state of the VNC session matches the
         * all-released state assumed by connecting clients before any key
         * events are forwarded */
        if (!vnc_client->lock_state_synced) {
            vnc_client->lock_state_synced = 1;
            guac_client_log(user->client, GUAC_LOG_WARNING, "VNC server did "
                "not report which lock keys are active. Client-side use of "
                "Caps Lock, etc. may not match server-side lock state.");
        }

        SendKeyEvent(rfb_client, guac_vnc_translate_keysym(keysym), pressed);

    }

    return 0;
}

#ifdef LIBVNC_HAS_RESIZE_SUPPORT
int guac_vnc_user_size_handler(guac_user* user, int width, int height,
        int x_position, int top_offset, int left_offset) {

    guac_user_log(user, GUAC_LOG_TRACE, "Running user size handler.");

    /* Get the Guacamole VNC client */
    guac_vnc_client* vnc_client = (guac_vnc_client*) user->client->data;

    /* Send display update */
    guac_vnc_display_set_size(vnc_client->rfb_client, width, height);

    return 0;

}
#endif // LIBVNC_HAS_RESIZE_SUPPORT
