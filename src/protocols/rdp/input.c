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

#include "channels/disp.h"
#include "channels/rdpei.h"
#include "input.h"
#include "guacamole/display.h"
#include "keyboard.h"
#include "rdp.h"
#include "settings.h"
#include "../../guaclog/log.h"

#include <freerdp/freerdp.h>
#include <freerdp/input.h>
#include <guacamole/client.h>
#include <guacamole/display.h>
#include <guacamole/recording.h>
#include <guacamole/rwlock.h>
#include <guacamole/user.h>

#include <stdlib.h>
#include <syslog.h>


int guac_rdp_user_mouse_handler(guac_user* user, int x, int y, int mask) {

    guac_client* client = user->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    guac_rwlock_acquire_read_lock(&(rdp_client->lock));

    /* Skip if not yet connected */
    freerdp* rdp_inst = rdp_client->rdp_inst;
    if (rdp_inst == NULL)
        goto complete;

    /* Store current mouse location/state */
    guac_display_notify_user_moved_mouse(rdp_client->display, user, x, y, mask);

    /* Report mouse position within recording */
    if (rdp_client->recording != NULL)
        guac_recording_report_mouse(rdp_client->recording, x, y, mask);

    /* If button mask unchanged, just send move event */
    if (mask == rdp_client->mouse_button_mask) {
        pthread_mutex_lock(&(rdp_client->message_lock));
        GUAC_RDP_CONTEXT(rdp_inst)->input->MouseEvent(
                GUAC_RDP_CONTEXT(rdp_inst)->input, PTR_FLAGS_MOVE, x, y);
        pthread_mutex_unlock(&(rdp_client->message_lock));
    }

    /* Otherwise, send events describing button change */
    else {

        /* Mouse buttons which have JUST become released */
        int released_mask =  rdp_client->mouse_button_mask & ~mask;

        /* Mouse buttons which have JUST become pressed */
        int pressed_mask  = ~rdp_client->mouse_button_mask &  mask;

        /* Release event */
        if (released_mask & 0x07) {

            /* Calculate flags */
            int flags = 0;
            if (released_mask & 0x01) flags |= PTR_FLAGS_BUTTON1;
            if (released_mask & 0x02) flags |= PTR_FLAGS_BUTTON3;
            if (released_mask & 0x04) flags |= PTR_FLAGS_BUTTON2;

            pthread_mutex_lock(&(rdp_client->message_lock));
            GUAC_RDP_CONTEXT(rdp_inst)->input->MouseEvent(
                    GUAC_RDP_CONTEXT(rdp_inst)->input, flags, x, y);
            pthread_mutex_unlock(&(rdp_client->message_lock));

        }

        /* Press event */
        if (pressed_mask & 0x07) {

            /* Calculate flags */
            int flags = PTR_FLAGS_DOWN;
            if (pressed_mask & 0x01) flags |= PTR_FLAGS_BUTTON1;
            if (pressed_mask & 0x02) flags |= PTR_FLAGS_BUTTON3;
            if (pressed_mask & 0x04) flags |= PTR_FLAGS_BUTTON2;
            if (pressed_mask & 0x08) flags |= PTR_FLAGS_WHEEL | 0x78;
            if (pressed_mask & 0x10) flags |= PTR_FLAGS_WHEEL | PTR_FLAGS_WHEEL_NEGATIVE | 0x88;

            /* Send event */
            pthread_mutex_lock(&(rdp_client->message_lock));
            GUAC_RDP_CONTEXT(rdp_inst)->input->MouseEvent(
                    GUAC_RDP_CONTEXT(rdp_inst)->input, flags, x, y);
            pthread_mutex_unlock(&(rdp_client->message_lock));

        }

        /* Scroll event */
        if (pressed_mask & 0x18) {

            /* Down */
            if (pressed_mask & 0x08) {
                pthread_mutex_lock(&(rdp_client->message_lock));
                GUAC_RDP_CONTEXT(rdp_inst)->input->MouseEvent(
                        GUAC_RDP_CONTEXT(rdp_inst)->input, PTR_FLAGS_WHEEL | 0x78, x, y);
                pthread_mutex_unlock(&(rdp_client->message_lock));
            }

            /* Up */
            if (pressed_mask & 0x10) {
                pthread_mutex_lock(&(rdp_client->message_lock));
                GUAC_RDP_CONTEXT(rdp_inst)->input->MouseEvent(
                        GUAC_RDP_CONTEXT(rdp_inst)->input, PTR_FLAGS_WHEEL | PTR_FLAGS_WHEEL_NEGATIVE | 0x88, x, y);
                pthread_mutex_unlock(&(rdp_client->message_lock));
            }

        }

        rdp_client->mouse_button_mask = mask;
    }

complete:
    guac_rwlock_release_lock(&(rdp_client->lock));

    return 0;
}

int guac_rdp_user_touch_handler(guac_user* user, int id, int x, int y,
        int x_radius, int y_radius, double angle, double force) {

    guac_client* client = user->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    guac_rwlock_acquire_read_lock(&(rdp_client->lock));

    /* Skip if not yet connected */
    freerdp* rdp_inst = rdp_client->rdp_inst;
    if (rdp_inst == NULL)
        goto complete;

    /* Report touch event within recording */
    if (rdp_client->recording != NULL)
        guac_recording_report_touch(rdp_client->recording, id, x, y,
                x_radius, y_radius, angle, force);

    /* Forward touch event along RDPEI channel */
    guac_rdp_rdpei_touch_update(rdp_client->rdpei, id, x, y, force);

complete:
    guac_rwlock_release_lock(&(rdp_client->lock));

    return 0;
}

int guac_rdp_user_key_handler(guac_user* user, int keysym, int pressed) {

    guac_client* client = user->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    int retval = 0;

    guac_rwlock_acquire_read_lock(&(rdp_client->lock));

    /* Report key state within recording */
    if (rdp_client->recording != NULL)
        guac_recording_report_key(rdp_client->recording,
                keysym, pressed);

    /* Skip if keyboard not yet ready */
    if (rdp_client->keyboard == NULL)
        goto complete;

    /* Update keysym state */
    retval = guac_rdp_keyboard_update_keysym(rdp_client->keyboard,
                keysym, pressed, GUAC_RDP_KEY_SOURCE_CLIENT);

complete:
    guac_rwlock_release_lock(&(rdp_client->lock));

    return retval;

}

int guac_rdp_user_size_handler(guac_user* user, int width, int height, int monitors, char** argv) {

    guac_client* client = user->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    guac_rdp_settings* settings = rdp_client->settings;
    freerdp* rdp_inst = rdp_client->rdp_inst;

    /* Convert client pixels to remote pixels */
    width  = width  * settings->resolution / user->info.optimal_resolution;
    height = height * settings->resolution / user->info.optimal_resolution;

    if (monitors == 1) {
        for (int i = 1; i < settings->max_secondary_monitors; i++) {

            // Default to zero if no monitor is configured
            rdp_client->disp->monitors[i].width = 0;
            rdp_client->disp->monitors[i].height = 0;
        }

        rdp_client->disp->monitors[0].width = width;
        rdp_client->disp->monitors[0].height = height;

    } else {
        syslog(6, "SfT - guac_rdp_user_size_handler max_monitors = %d", settings->max_secondary_monitors);
        // Update monitor dimensions based on the number of monitors
        for (int i = 0; i < settings->max_secondary_monitors; i++) {
                
            if (i < monitors) {
                if (monitors == 1) break;
                int i_monitor_width = atoi(argv[3 + i * 2]);
                int i_monitor_height = atoi(argv[3 + i * 2 + 1]);

                // Adjust dimensions based on resolution settings
                i_monitor_width  = i_monitor_width  * settings->resolution / user->info.optimal_resolution;
                i_monitor_height = i_monitor_height * settings->resolution / user->info.optimal_resolution;

                syslog(6, "SfT - guac_rdp_user_size_handler monitor_index = %d size(%d, %d)", i, i_monitor_width, i_monitor_height);

                if (i == 0) {
                    width = i_monitor_width;
                    height = i_monitor_height;
                }

                // Set monitor dimensions
                rdp_client->disp->monitors[i].width = i_monitor_width;
                rdp_client->disp->monitors[i].height = i_monitor_height;
            } else {
                // Default to zero if no monitor is configured
                rdp_client->disp->monitors[i].width = 0;
                rdp_client->disp->monitors[i].height = 0;
            }
        }
    }

    /* Send display update */
    guac_rdp_disp_set_size(rdp_client->disp, settings, rdp_inst, width, height, monitors);

    return 0;

}
