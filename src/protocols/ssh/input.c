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
#include "terminal/terminal.h"
#include "terminal/terminal-priv.h"

#include <guacamole/client.h>
#include <guacamole/recording.h>
#include <guacamole/user.h>
#include <libssh2.h>

#include <pthread.h>

int guac_ssh_user_mouse_handler(guac_user* user, int x, int y, int mask) {

    guac_client* client = user->client;
    guac_ssh_client* ssh_client = (guac_ssh_client*) client->data;
    guac_terminal* term = ssh_client->term;

    /* Get mouse information */
    term->select_row = y / term->display->char_height - term->scroll_offset;
    term->select_col = x / term->display->char_width;
    printf("Column: %d\n", term->select_col);
    term->select_head = term->select_col;
    term->select_tail = term->select_col;
    int released_mask =  term->mouse_mask & ~mask;
    int pressed_mask  = ~term->mouse_mask &  mask;

    /* Skip if terminal not yet ready */
    if (term == NULL)
        return 0;

    /* Report mouse position within recording */
    if (ssh_client->recording != NULL)
        guac_common_recording_report_mouse(ssh_client->recording, x, y, mask);
    
    /* Clear the selection effect if not on selecting*/
    if (term->mouse_state == 0 && (pressed_mask & GUAC_CLIENT_MOUSE_LEFT)){

        /* Clear selection effect */
        guac_terminal_draw_blank(term);

        /* Send mouse event */
        guac_terminal_send_mouse(term, user, x, y, mask);
        return 0;
    }

    /* Start time recoding when mouse first pressed */
    else if (term->mouse_state == 0 && (released_mask & GUAC_CLIENT_MOUSE_LEFT)){
        
        /* Start timer */
        term->start_1 = guac_timestamp_current();
        term->mouse_state = 1;

        /* Send mouse event */
        guac_terminal_send_mouse(term, user, x, y, mask);
        return 0;
    }

    /* Second click events */
    else if (term->mouse_state == 1 && (pressed_mask & GUAC_CLIENT_MOUSE_LEFT)){

        term->end_1 = guac_timestamp_current();
        guac_timestamp interval_1 = term->end_1 - term->start_1;

        /* Time interval determination */
        if (interval_1 < 300){
            guac_terminal_double_click(term);
            term->start_2 = guac_timestamp_current();
        }
        else
            term->mouse_state = 0;
        
        /* Send mouse event */
        guac_terminal_send_mouse(term, user, x, y, mask);
        return 0;
    }

    /* After second events */
    else if (term->mouse_state == 1 && (released_mask & GUAC_CLIENT_MOUSE_LEFT)){
        term->end_2 = guac_timestamp_current();
        guac_timestamp interval_2 = term->end_2 - term->start_2;

        /* Time interval determination */
        if (interval_2 < 300){
            term->start_3 = guac_timestamp_current();
            term->mouse_state = 2;
        }
        else 
            term->mouse_state = 0;
        
        /* Send mouse event */
        guac_terminal_send_mouse(term, user, x, y, mask);
        return 0;
    }

    /* Third click events */
    else if (term->mouse_state == 2 && (pressed_mask & GUAC_CLIENT_MOUSE_LEFT)){
        term->end_3 = guac_timestamp_current();
        guac_timestamp interval_3 = term->end_3 - term->start_3;

        /* Time interval determination */
        if (interval_3 < 300){
            guac_terminal_triple_click(term);
        }
        else {
            guac_terminal_draw_blank(term);
            term->mouse_state = 0;
        }

        /* Send mouse event */
        guac_terminal_send_mouse(term, user, x, y, mask);
        return 0;
    }

    /* Other conditions */
    else if (term->mouse_state == 2 && (released_mask & GUAC_CLIENT_MOUSE_LEFT)){
        term->mouse_state = 0;

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
        guac_recording_report_key(ssh_client->recording,
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
                guac_terminal_get_columns(terminal),
                guac_terminal_get_rows(terminal));
        pthread_mutex_unlock(&(ssh_client->term_channel_lock));
    }

    return 0;
}

