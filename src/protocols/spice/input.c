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

#include "config.h"

#include "common/cursor.h"
#include "common/display.h"
#include "spice.h"
#include "spice-constants.h"

#include <guacamole/recording.h>
#include <guacamole/user.h>
#include <spice-client-glib-2.0/spice-client.h>

int guac_spice_user_mouse_handler(guac_user* user, int x, int y, int mask) {

    guac_client* client = user->client;
    guac_spice_client* spice_client = (guac_spice_client*) client->data;

    /* Store the old button mask. */
    int curr_button_mask = spice_client->display->cursor->button_mask;

    guac_user_log(user, GUAC_LOG_TRACE, "Handling mouse event: %d, %d, 0x%08x", x, y, mask);

    /* Update current mouse location/state */
    guac_common_cursor_update(spice_client->display->cursor, user, x, y, mask);

    /* Report mouse position and button state within recording. */
    if (spice_client->recording != NULL)
        guac_recording_report_mouse(spice_client->recording, x, y, mask);

    /* Send SPICE event only if finished connecting */
    if (spice_client->inputs_channel != NULL) {
        spice_inputs_channel_position(spice_client->inputs_channel, x, y, GUAC_SPICE_DEFAULT_DISPLAY_ID, mask);

        /* Button state has changed, so we need to evaluate which buttons changed. */
        if (curr_button_mask != mask) {

            /* Left click. */
            if (curr_button_mask ^ GUAC_CLIENT_MOUSE_LEFT && mask & GUAC_CLIENT_MOUSE_LEFT) {
                guac_user_log(user, GUAC_LOG_TRACE, "Left click!");
                spice_inputs_channel_button_press(spice_client->inputs_channel, SPICE_MOUSE_BUTTON_LEFT, mask);
            }

            /* Left release. */
            if (curr_button_mask & GUAC_CLIENT_MOUSE_LEFT && mask ^ GUAC_CLIENT_MOUSE_LEFT) {
                guac_user_log(user, GUAC_LOG_TRACE, "Left release!");
                spice_inputs_channel_button_release(spice_client->inputs_channel, SPICE_MOUSE_BUTTON_LEFT, mask);
            }

            /* Middle click. */
            if (curr_button_mask ^ GUAC_CLIENT_MOUSE_MIDDLE && mask & GUAC_CLIENT_MOUSE_MIDDLE) {
                guac_user_log(user, GUAC_LOG_TRACE, "Middle click!");
                spice_inputs_channel_button_press(spice_client->inputs_channel, SPICE_MOUSE_BUTTON_MIDDLE, mask);
            }

            /* Middle release. */
            if (curr_button_mask & GUAC_CLIENT_MOUSE_MIDDLE && mask ^ GUAC_CLIENT_MOUSE_MIDDLE) {
                guac_user_log(user, GUAC_LOG_TRACE, "Middle release!");
                spice_inputs_channel_button_release(spice_client->inputs_channel, SPICE_MOUSE_BUTTON_MIDDLE, mask);
            }

            /* Right click. */
            if (curr_button_mask ^ GUAC_CLIENT_MOUSE_RIGHT && mask & GUAC_CLIENT_MOUSE_RIGHT) {
                guac_user_log(user, GUAC_LOG_TRACE, "Right click!");
                spice_inputs_channel_button_press(spice_client->inputs_channel, SPICE_MOUSE_BUTTON_RIGHT, mask);
            }

            /* Right release. */
            if (curr_button_mask & GUAC_CLIENT_MOUSE_RIGHT && mask ^ GUAC_CLIENT_MOUSE_RIGHT) {
                guac_user_log(user, GUAC_LOG_TRACE, "Right release!");
                spice_inputs_channel_button_release(spice_client->inputs_channel, SPICE_MOUSE_BUTTON_RIGHT, mask);
            }

            /* Scroll up. */
            if (curr_button_mask ^ GUAC_CLIENT_MOUSE_SCROLL_UP && mask & GUAC_CLIENT_MOUSE_SCROLL_UP) {
                guac_user_log(user, GUAC_LOG_TRACE, "Scroll up!");
                spice_inputs_channel_button_press(spice_client->inputs_channel, SPICE_MOUSE_BUTTON_UP, mask);
            }

            /* Scroll up stop. */
            if (curr_button_mask & GUAC_CLIENT_MOUSE_SCROLL_UP && mask ^ GUAC_CLIENT_MOUSE_SCROLL_UP) {
                guac_user_log(user, GUAC_LOG_TRACE, "Scroll up stop!");
                spice_inputs_channel_button_release(spice_client->inputs_channel, SPICE_MOUSE_BUTTON_UP, mask);
            }

            /* Scroll down. */
            if (curr_button_mask ^ GUAC_CLIENT_MOUSE_SCROLL_DOWN && mask & GUAC_CLIENT_MOUSE_SCROLL_DOWN) {
                guac_user_log(user, GUAC_LOG_TRACE, "Scroll down!");
                spice_inputs_channel_button_press(spice_client->inputs_channel, SPICE_MOUSE_BUTTON_DOWN, mask);
            }

            /* Scroll down stop. */
            if (curr_button_mask & GUAC_CLIENT_MOUSE_SCROLL_DOWN && mask ^ GUAC_CLIENT_MOUSE_SCROLL_DOWN) {
                guac_user_log(user, GUAC_LOG_TRACE, "Scroll down stop!");
                spice_inputs_channel_button_release(spice_client->inputs_channel, SPICE_MOUSE_BUTTON_DOWN, mask);
            }

        }

    }

    return 0;
}

int guac_spice_user_key_handler(guac_user* user, int keysym, int pressed) {
    
    guac_spice_client* spice_client = (guac_spice_client*) user->client->data;
    int retval = 0;

    pthread_rwlock_rdlock(&(spice_client->lock));

    guac_user_log(user, GUAC_LOG_TRACE, "Handling keypress: 0x%08x", keysym);
    
    /* Report key state within recording */
    if (spice_client->recording != NULL)
        guac_recording_report_key(spice_client->recording,
                keysym, pressed);

    /* Skip if keyboard not yet ready */
    if (spice_client->inputs_channel == NULL || spice_client->keyboard == NULL)
        goto complete;
        
    /* Update keysym state */
    retval = guac_spice_keyboard_update_keysym(spice_client->keyboard,
                keysym, pressed, GUAC_SPICE_KEY_SOURCE_CLIENT);

complete:
    pthread_rwlock_unlock(&(spice_client->lock));

    return retval;
}

void guac_spice_mouse_mode_update(SpiceChannel* channel, guac_client* client) {
    
    guac_client_log(client, GUAC_LOG_DEBUG, "Updating mouse mode, not implemented.");
    
}