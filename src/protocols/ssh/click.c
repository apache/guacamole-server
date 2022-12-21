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
#include "click.h"
#include "ssh.h"
#include "terminal/terminal.h"
#include "terminal/select.h"

#include <guacamole/client.h>
#include <guacamole/unicode.h>
#include <guacamole/user.h>
#include <libssh2.h>

#include <pthread.h>

void guac_click_draw_select(guac_click* click){

    int height = click->term->display->char_height;
    int width = click->term->display->char_width;

    guac_protocol_send_rect(click->socket, click->select_layer, 
    click->select_head * width, click->select_row * height,
    (click->select_tail - click->select_head + 1) * width, height);

    guac_protocol_send_cfill(click->socket, GUAC_COMP_SRC, click->select_layer,
    0x00, 0x80, 0xFF, 0x60);

}

void guac_click_draw_blank(guac_click* click){

    guac_protocol_send_rect(click->socket, click->select_layer, 0, 0, 1, 1);

    guac_protocol_send_cfill(click->socket, GUAC_COMP_SRC, click->select_layer,
    0x00, 0x00, 0x00, 0x00);

}

void guac_click_select_word(guac_click* click){

    int head = click->select_head;
    int tail = click->select_tail;
    
    /* Clear clipboard for new selection */
    guac_common_clipboard_reset(click->term->clipboard, "text/plain");
    char buffer[1024];
    int i = head;

    /* Get information of selected row */
    guac_terminal_buffer_row* buffer_row = guac_terminal_buffer_get_row(click->term->buffer, click->select_row, 0);
    int index = (click->term->buffer->top + click->select_row) % click->term->buffer->available;
    int length = buffer_row->length;

    if(index < 0)
        index += click->term->buffer->available;

    if(head < 0 || head > length - 1)
        return;

    if(tail < 0 || tail > length - 1)
        tail = length - 1;
    
    /* Write each character to clipboard */
    

    while (i <= tail){

        int remaining = sizeof(buffer);
        char* current = buffer;

        for(i = head; i <= tail; i++) {
            int codepoint = buffer_row->characters[i].value;

            if (codepoint == 0 || codepoint == GUAC_CHAR_CONTINUATION)
                continue;

            int bytes = guac_utf8_write(codepoint, current, remaining);

            if (bytes == 0)
                break;

            current += bytes;
            remaining -= bytes;
        }

        guac_common_clipboard_append(click->term->clipboard, buffer, current - buffer);
        guac_common_clipboard_send(click->term->clipboard, click->client);

    }    

}

void guac_click_select_blank(guac_click* click){

    int head = click->select_head;
    int tail = click->select_tail;
    
    /* Clear clipboard for new selection */
    guac_common_clipboard_reset(click->term->clipboard, "text/plain");
    char buffer[1024];

    /* Get information of selected row */
    guac_terminal_buffer_row* buffer_row = guac_terminal_buffer_get_row(click->term->buffer, click->select_row, 0);
    int index = (click->term->buffer->top + click->select_row) % click->term->buffer->available;
    int l = buffer_row->length;

    if(index < 0)
        index += click->term->buffer->available;

    if(head < 0 || head > l - 1)
        return;

    if(tail < 0 || tail > l - 1)
        tail = l - 1;
    
    int i = head;

    /* Write blank to clipboard */
    while (i <= tail)
    {
        int remaining = sizeof(buffer);
        char* current = buffer;

        for(i = head; i <= tail; i++) {
            int codepoint = 32;

            int bytes = guac_utf8_write(codepoint, current, remaining);

            if (bytes == 0)
                    break;
                    
            current += bytes;
            remaining -= bytes;

        }

        guac_common_clipboard_append(click->term->clipboard, buffer, current - buffer);
        guac_common_clipboard_send(click->term->clipboard, click->client);

    }    

}

void guac_click_select_mark(guac_click* click){

    /* Clear clipboard for new selection */
    guac_common_clipboard_reset(click->term->clipboard, "text/plain");
    char buffer[1024];
    char* current = buffer;

    /* Get information of selected row */
    guac_terminal_buffer_row* buffer_row = guac_terminal_buffer_get_row(click->term->buffer, click->select_row, 0);
    
    /* Write char to clipboard */
    int bytes = guac_utf8_write(buffer_row->characters[click->select_col].value, current, sizeof(buffer));
    current += bytes;

    guac_common_clipboard_append(click->term->clipboard, buffer, current - buffer);
    guac_common_clipboard_send(click->term->clipboard, click->client);

}

void guac_click_select_line(guac_click* click){
    
    /* Clear clipboard for new selection */
    guac_common_clipboard_reset(click->term->clipboard, "text/plain");
    char buffer[1024];

    /* Get information of selected row */
    guac_terminal_buffer_row* buffer_row = guac_terminal_buffer_get_row(click->term->buffer, click->select_row, 0);
    int index = (click->term->buffer->top + click->select_row) % click->term->buffer->available;
    int i = 0;
    int length = buffer_row->length;

    if(index < 0)
        index += click->term->buffer->available;
    
    /* Write each character to clipboard */
    
    while (i <= length){

        int remaining = sizeof(buffer);
        char* current = buffer;

        for(i = 0; i <= length; i++) {
            int codepoint = buffer_row->characters[i].value;

            if (codepoint == 0)
                codepoint = 32;

            if (codepoint == GUAC_CHAR_CONTINUATION)
                continue;

            int bytes = guac_utf8_write(codepoint, current, remaining);

            if (bytes == 0)
                break;

            current += bytes;
            remaining -= bytes;
        }

        guac_common_clipboard_append(click->term->clipboard, buffer, current - buffer);
        guac_common_clipboard_send(click->term->clipboard, click->client);

    }    

}

guac_click* guac_click_get_word_border(guac_click* click){

    /* Get select information */

    guac_terminal* term = click->term;
    
    int row = click->select_row;
    int col = click->select_col;
    int head = click->select_head;
    int tail = click->select_tail;

    int sentinel = term->display->operations[row * term->display->width + col].character.value;
    int flag = sentinel;
            
    /* Get head */
    while (((flag > 64 && flag < 91) || (flag > 96 && flag < 123) || (flag > 47 && flag < 58) || (flag == 95)) && (head >= 0 && head <= term->display->width)){
        flag = term->display->operations[row * term->display->width + head].character.value;
        head--;
    }
    

    /* Reset flag */
    flag = sentinel;

    /* Get tail */
    while (((flag > 64 && flag < 91) || (flag > 96 && flag < 123) || (flag > 47 && flag < 58) || (flag == 95)) &&(tail >= 0 && tail <= term->display->width)){
        flag = term->display->operations[row * term->display->width + tail].character.value;
        tail++;
    }
    

            
    head += 2;
    tail -= 2;

    int h = term->display->operations[row * term->display->width + head - 1].character.value;
    if (head == 1 && ((h > 64 && h < 91) || (h > 96 && h < 123) || (h > 47 && h < 58)  || (h == 95)))
        head = 0;

    printf("Head: %d\n", head);
    printf("Tail: %d\n", tail);

    click->select_head = head;
    click->select_tail = tail;

    return click;

}

guac_click* guac_click_get_blank_border(guac_click* click){
    
    /* Get select information */
    guac_terminal* term = click->term;
    
    int row = click->select_row;
    int col = click->select_col;
    int head = click->select_head;
    int tail = click->select_tail;

    int sentinel = term->display->operations[row * term->display->width + col].character.value;
    int flag = sentinel;

    /* Get head */
    while ((flag == 0 || flag == 32) && (head >= 0 && head <= term->display->width)){
        flag = term->display->operations[row * term->display->width + head].character.value;
        head--;
    }

    /* Reset flag */
    flag = sentinel;

    /* Get tail */
    while ((flag == 0 || flag == 32) && (tail >= 0 && tail <= term->display->width)){
        flag = term->display->operations[row * term->display->width + tail].character.value;
        tail++;
    }
                
    head += 2;
    tail -= 2;

    if(head == 1 && (term->display->operations[row * term->display->width + head].character.value == 0 ||
                     term->display->operations[row * term->display->width + head].character.value == 32))
        head = 0;
    
    click->select_head = head;
    click->select_tail = tail;

    return click;
}

void guac_double_click(guac_click* click){

    /* Set selection variables */
    guac_terminal* term = click->term;
    int row = click->select_row;
    int col = click->select_col;
    printf("Row: %d, Col: %d\n", row, col);
    int sentinel = term->display->operations[row * term->display->width + col].character.value;

    /* Determination */

    /* Words */

    if ((sentinel > 64 && sentinel < 91) || (sentinel > 96 && sentinel < 123) || (sentinel > 47 && sentinel < 58) || (sentinel == 95)){
        
        /* get border */
        guac_click* selection = guac_click_get_word_border(click);

        /* Copy to clipboard */
        guac_click_select_word(selection);

        /* Draw selection */
        guac_click_draw_select(selection);

        return;

    }

    /* Marks */

    else if ((sentinel > 32 && sentinel < 48) || (sentinel > 57 && sentinel < 65) || (sentinel > 90 && sentinel < 95) || (sentinel > 122 && sentinel < 127) ||(sentinel == 96)){

        /* Copy to clipboard */
        guac_click_select_mark(click);

        /* Draw selection */
        click->select_head = click->select_col;
        click->select_tail = click->select_col;

        guac_click_draw_select(click);

        return;

    }

    /* Blank */

    else if (sentinel == 0 || sentinel == 32){

        /* Get blank border */
        guac_click* selection = guac_click_get_blank_border(click);

        /* Blank position */
        int head = selection->select_head;
        int tail = selection->select_tail;
        int width = selection->term->display->width;

        if (head == 0 || tail < width - 1){
            
            /* get border */
            guac_click* selection = guac_click_get_blank_border(click);

            /* Copy to clipboard */
            guac_click_select_blank(selection);

            /* Draw selection */
            guac_click_draw_select(selection);

            return;
        }

        else if (head > 0 && tail == width - 1){

            /* Move selected column */
            sentinel = term->display->operations[row * term->display->width + col].character.value;
            while(sentinel == 0 || sentinel == 32){
                col--;
                sentinel = term->display->operations[row * term->display->width + col].character.value;
            }

            selection->select_col = col;
            selection->select_head = col;
            selection->select_tail = col;
            sentinel = term->display->operations[row * term->display->width + col].character.value;

            /* Determination */

            /* Words */

            if ((sentinel > 64 && sentinel < 91) || (sentinel > 96 && sentinel < 123) || (sentinel > 47 && sentinel < 58) || (sentinel == 95)){
                
                /* get border */
                guac_click* moved_selection = guac_click_get_word_border(selection);

                /* Copy to clipboard */
                guac_click_select_word(moved_selection);

                /* Draw selection */
                guac_click_draw_select(moved_selection);

                return;

            }

            /* Marks */

            else if ((sentinel > 32 && sentinel < 48) || (sentinel > 57 && sentinel < 65) || (sentinel > 90 && sentinel < 95) || (sentinel > 122 && sentinel < 127) ||(sentinel == 96)){

                /* Copy to clipboard */
                guac_click_select_mark(selection);

                /* Draw selection */
                selection->select_head = selection->select_col;
                selection->select_tail = selection->select_col;

                guac_click_draw_select(selection);

                return;

            }

        }

    }

}

void guac_triple_click(guac_click* click){

    click->select_head = 0;
    click->select_tail = click->term->display->width;

    guac_click_select_line(click);

    guac_click_draw_select(click);

}