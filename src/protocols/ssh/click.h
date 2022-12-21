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

#ifndef GUAC_SSH_CLICK_H
#define GUAC_SSH_CLICK_H

#include "config.h"

#include <guacamole/user.h>
#include "terminal/terminal.h"

typedef struct guac_click {

    int select_row;

    int select_col;
    
    int select_head;
    
    int select_tail;

    guac_terminal* term;

    guac_client* client;

    guac_socket* socket; 
    
    guac_layer* select_layer;

} guac_click;

/* Display selection effect */
void guac_click_draw_select(guac_click* click);

void guac_click_draw_blank(guac_click* click);


/* Function of selecting */
void guac_click_select_word(guac_click* click);

void guac_click_select_blank(guac_click* click);

void guac_click_select_mark(guac_click* click);

void guac_click_select_line(guac_click* click);

/* Select determination */
guac_click* guac_click_get_word_border(guac_click* click);

guac_click* guac_click_get_blank_border(guac_click* click);

/* Click determination */
void guac_double_click(guac_click* click);

void guac_triple_click(guac_click* click);

#endif