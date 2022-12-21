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
#include "common/recording.h"
#include "ssh.h"
#include "click.h"
#include "terminal/terminal.h"

#include <guacamole/client.h>
#include <guacamole/recording.h>
#include <guacamole/user.h>
#include <libssh2.h>

#include <pthread.h>

#include <stdio.h>
#include <sys/timeb.h>


int state = 0;
long long start_1, start_2, start_3 = 0;
long long end_1, end_2, end_3 = 0;

long long getSystemTime() {
    struct timeb t;
    ftime(&t);
    return 1000 * t.time + t.millitm;
}

int guac_ssh_user_mouse_handler(guac_user* user, int x, int y, int mask) {

    guac_client* client = user->client;
    guac_ssh_client* ssh_client = (guac_ssh_client*) client->data;
    guac_terminal* term = ssh_client->term;

    /* Skip if terminal not yet ready */
    if (term == NULL)
        return 0;

    /* Report mouse position within recording */
    if (ssh_client->recording != NULL)
        guac_common_recording_report_mouse(ssh_client->recording, x, y, mask);

    guac_socket* socket = term->display->client->socket;
    guac_layer* select_layer = term->display->select_layer;
    int row = y / term->display->char_height - term->scroll_offset;
    int col = x / term->display->char_width;
    int released_mask =  term->mouse_mask & ~mask;
    int pressed_mask  = ~term->mouse_mask &  mask;

    guac_click default_click = {
        .select_row = row,
        .select_col = col,
        .select_head = col,
        .select_tail = col,
        .term = term,
        .client = client,
        .socket = socket,
        .select_layer = select_layer
    };

    guac_click* click = &default_click;

    if (state == 0 && (pressed_mask & GUAC_CLIENT_MOUSE_LEFT)){
        
        /* Clear selection effect */
        guac_click_draw_blank(click);

        /* Send mouse event */
        guac_terminal_send_mouse(term, user, x, y, mask);
        return 0;

    }

    else if (state == 0 && (released_mask & GUAC_CLIENT_MOUSE_LEFT)){
        
        /* Start timer */
        start_1 = getSystemTime();
        state = 1;

        /* Send mouse event */
        guac_terminal_send_mouse(term, user, x, y, mask);
        return 0;
    }

    else if (state == 1 && (pressed_mask & GUAC_CLIENT_MOUSE_LEFT)){

        end_1 = getSystemTime();
        long long interval_1 = end_1 - start_1;
        if (interval_1 < 300){
            guac_double_click(click);
            start_2 = getSystemTime();
        }
        else
            state = 0;
        
        /* Send mouse event */
        guac_terminal_send_mouse(term, user, x, y, mask);
        return 0;
    }

    else if (state == 1 && (released_mask & GUAC_CLIENT_MOUSE_LEFT)){
        end_2 = getSystemTime();
        long long interval_2 = end_2 - start_2;
        if (interval_2 < 300){
            start_3 = getSystemTime();
            state = 2;
        }
        else 
            state = 0;
        
        /* Send mouse event */
        guac_terminal_send_mouse(term, user, x, y, mask);
        return 0;
    }

    else if (state == 2 && (pressed_mask & GUAC_CLIENT_MOUSE_LEFT)){
        end_3 = getSystemTime();
        long long interval_3 = end_3 - start_3;
        if (interval_3 < 300){
            guac_triple_click(click);
        }
        else {
            guac_click_draw_blank(click);
            state = 0;
        }

        /* Send mouse event */
        guac_terminal_send_mouse(term, user, x, y, mask);
        return 0;
    }

    else if (state == 2 && (released_mask & GUAC_CLIENT_MOUSE_LEFT)){
        
        state = 0;
        /* Send mouse event */
        guac_terminal_send_mouse(term, user, x, y, mask);
        return 0;
    }
    
    else{
        /* Send mouse event */
        guac_terminal_send_mouse(term, user, x, y, mask);
        return 0;
    }

}

int guac_ssh_user_key_handler(guac_user* user, int keysym, int pressed) {

    guac_client* client = user->client;
    guac_ssh_client* ssh_client = (guac_ssh_client*) client->data;
    guac_terminal* term = ssh_client->term;

    /* Report key state within recording */
    if (ssh_client->recording != NULL)
        guac_common_recording_report_key(ssh_client->recording,
                keysym, pressed);

    /* Skip if terminal not yet ready */
    if (term == NULL)
        return 0;

    /* Send key */
    guac_terminal_send_key(term, keysym, pressed);
    return 0;
}

int guac_ssh_user_size_handler(guac_user* user, int width, int height) {

    /* Get terminal */
    guac_client* client = user->client;
    guac_ssh_client* ssh_client = (guac_ssh_client*) client->data;
    guac_terminal* terminal = ssh_client->term;

    /* Skip if terminal not yet ready */
    if (terminal == NULL)
        return 0;

    /* Resize terminal */
    guac_terminal_resize(terminal, width, height);

    /* Update SSH pty size if connected */
    if (ssh_client->term_channel != NULL) {
        pthread_mutex_lock(&(ssh_client->term_channel_lock));
        libssh2_channel_request_pty_size(ssh_client->term_channel,
                terminal->term_width, terminal->term_height);
        pthread_mutex_unlock(&(ssh_client->term_channel_lock));
    }

    return 0;
}

