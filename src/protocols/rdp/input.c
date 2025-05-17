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

#include <freerdp/freerdp.h>
#include <freerdp/input.h>
#include <guacamole/client.h>
#include <guacamole/display.h>
#include <guacamole/recording.h>
#include <guacamole/rwlock.h>
#include <guacamole/user.h>

#include <stdlib.h>

int guac_rdp_user_mouse_handler(guac_user* user, int x, int y, int mask) {

    guac_client* client = user->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    guac_rwlock_acquire_read_lock(&(rdp_client->lock));

    /* Skip if not yet connected */
    freerdp* rdp_inst = rdp_client->rdp_inst;
    if (rdp_inst == NULL)
        goto complete;

    guac_rdp_input_event mouse_event = {

        .type = GUAC_RDP_INPUT_EVENT_MOUSE,
        .user = user,

        .details.mouse = {
            .x = x,
            .y = y,
            .mask = mask
        }

    };

    guac_fifo_enqueue(&rdp_client->input_events, &mouse_event);

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

int guac_rdp_user_size_handler(guac_user* user, int width, int height) {

    guac_client* client = user->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    guac_rdp_settings* settings = rdp_client->settings;
    freerdp* rdp_inst = rdp_client->rdp_inst;

    /* Convert client pixels to remote pixels */
    width  = width  * settings->resolution / user->info.optimal_resolution;
    height = height * settings->resolution / user->info.optimal_resolution;

    /* Send display update */
    guac_rdp_disp_set_size(rdp_client->disp, settings, rdp_inst, width, height);

    return 0;

}
