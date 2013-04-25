
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
 * The Original Code is libguac-client-ssh.
 *
 * The Initial Developer of the Original Code is
 * Michael Jumper.
 * Portions created by the Initial Developer are Copyright (C) 2011
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * James Muehlner <dagger10k@users.sourceforge.net>
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
#include <string.h>
#include <pthread.h>

#include <cairo/cairo.h>
#include <pango/pangocairo.h>

#include <guacamole/socket.h>
#include <guacamole/protocol.h>
#include <guacamole/client.h>

#include "types.h"
#include "buffer.h"
#include "delta.h"
#include "terminal.h"
#include "terminal_handlers.h"

guac_terminal* guac_terminal_create(guac_client* client,
        int width, int height) {

    guac_terminal_attributes default_attributes = {
        .foreground = 7,
        .background = 0,
        .bold       = false,
        .reverse    = false,
        .underscore = false
    };

    guac_terminal* term = malloc(sizeof(guac_terminal));
    term->client = client;

    /* Init buffer */
    term->buffer = guac_terminal_buffer_alloc(1000);
    term->scroll_offset = 0;

    /* Init delta */
    term->delta = guac_terminal_delta_alloc(client,
            term->term_width, term->term_height,
            default_attributes.foreground,
            default_attributes.background);

    /* Init terminal state */
    term->current_attributes = 
    term->default_attributes = default_attributes;

    term->cursor_row = 0;
    term->cursor_col = 0;

    term->term_width   = width  / term->delta->char_width;
    term->term_height  = height / term->delta->char_height;
    term->char_handler = guac_terminal_echo; 

    term->scroll_start = 0;
    term->scroll_end = term->term_height - 1;

    term->text_selected = false;

    /* Clear with background color */
    guac_terminal_clear(term,
            0, 0, term->term_height, term->term_width);

    /* Init terminal lock */
    pthread_mutex_init(&(term->lock), NULL);

    return term;

}

void guac_terminal_free(guac_terminal* term) {
    
    /* Free delta */
    guac_terminal_delta_free(term->delta);

    /* Free buffer */
    guac_terminal_buffer_free(term->buffer);

}

int guac_terminal_set(guac_terminal* term, int row, int col, char c) {

    int scrolled_row = row + term->scroll_offset;

    /* Build character with current attributes */
    guac_terminal_char guac_char;
    guac_char.value = c;
    guac_char.attributes = term->current_attributes;

    /* Set delta */
    if (scrolled_row < term->delta->height)
        guac_terminal_delta_set_columns(term->delta, scrolled_row, col, col, &guac_char);

    /* Set buffer */
    guac_terminal_buffer_set_columns(term->buffer, row, col, col, &guac_char);

    return 0;

}

int guac_terminal_toggle_reverse(guac_terminal* term, int row, int col) {

    int scrolled_row = row + term->scroll_offset;

    /* Get character from buffer */
    guac_terminal_char* guac_char =
        &(term->buffer->characters[row*term->buffer->width + col]);

    /* Toggle reverse */
    guac_char->attributes.reverse = !(guac_char->attributes.reverse);

    /* Set delta */
    if (scrolled_row < term->delta->height)
        guac_terminal_delta_set(term->delta, scrolled_row, col, guac_char);

    return 0;

}

int guac_terminal_write(guac_terminal* term, const char* c, int size) {

    while (size > 0) {
        term->char_handler(term, *(c++));
        size--;
    }

    return 0;

}

int guac_terminal_copy(guac_terminal* term,
        int src_row, int src_col, int rows, int cols,
        int dst_row, int dst_col) {

    int scrolled_src_row = src_row + term->scroll_offset;
    int scrolled_dst_row = dst_row + term->scroll_offset;

    int scrolled_rows = rows;

    /* FIXME: If source (but not dest) is partially scrolled out of view, then
     *        the delta will not be updated properly. We need to pull the data
     *        from the buffer in such a case.
     */

    if (scrolled_src_row < term->delta->height &&
            scrolled_dst_row < term->delta->height) {

        /* Adjust delta rect height if scrolled out of view */
        if (scrolled_src_row + scrolled_rows > term->delta->height)
            scrolled_rows = term->delta->height - scrolled_src_row;

        if (scrolled_dst_row + scrolled_rows > term->delta->height)
            scrolled_rows = term->delta->height - scrolled_dst_row;

        /* Update delta */
        guac_terminal_delta_copy(term->delta,
            scrolled_dst_row, dst_col,
            scrolled_src_row, src_col,
            cols, rows);

    }

    /* Update buffer */
    guac_terminal_buffer_copy(term->buffer,
        dst_row, dst_col,
        src_row, src_col,
        cols, rows);

    return 0;

}


int guac_terminal_clear(guac_terminal* term,
        int row, int col, int rows, int cols) {

    int scrolled_row = row + term->scroll_offset;
    int scrolled_rows = rows;

    /* Build space */
    guac_terminal_char character;
    character.value = ' ';
    character.attributes = term->current_attributes;

    if (scrolled_row < term->delta->height) {

        /* Adjust delta rect height if scrolled out of view */
        if (scrolled_row + scrolled_rows > term->delta->height)
            scrolled_rows = term->delta->height - scrolled_row;

        /* Fill with color */
        guac_terminal_delta_set_rect(term->delta,
            scrolled_row, col, cols, scrolled_rows, &character);

    }

    guac_terminal_buffer_set_rect(term->buffer,
        row, col, cols, rows, &character);

    return 0;

}

int guac_terminal_scroll_up(guac_terminal* term,
        int start_row, int end_row, int amount) {

    /* Calculate height of scroll region */
    int height = end_row - start_row + 1;
    
    /* If scroll region is entire screen, push rows into scrollback */
    if (start_row == 0 && end_row == term->term_height-1)
        guac_terminal_scrollback_buffer_append(term->scrollback, term, amount);

    return 

        /* Move rows within scroll region up by the given amount */
        guac_terminal_copy(term,
                start_row + amount, 0,
                height - amount, term->term_width,
                start_row, 0)

        /* Fill new rows with background */
        || guac_terminal_clear(term,
                end_row - amount + 1, 0, amount, term->term_width);

}

int guac_terminal_scroll_down(guac_terminal* term,
        int start_row, int end_row, int amount) {

    /* Calculate height of scroll region */
    int height = end_row - start_row + 1;
    
    return 

        /* Move rows within scroll region down by the given amount */
        guac_terminal_copy(term,
                start_row, 0,
                height - amount, term->term_width,
                start_row + amount, 0)

        /* Fill new rows with background */
        || guac_terminal_clear(term,
                start_row, 0, amount, term->term_width);

}

int guac_terminal_clear_range(guac_terminal* term,
        int start_row, int start_col,
        int end_row, int end_col) {

    /* If not at far left, must clear sub-region to far right */
    if (start_col > 0) {

        /* Clear from start_col to far right */
        if (guac_terminal_clear(term,
                start_row, start_col, 1, term->term_width - start_col))
            return 1;

        /* One less row to clear */
        start_row++;
    }

    /* If not at far right, must clear sub-region to far left */
    if (end_col < term->term_width - 1) {

        /* Clear from far left to end_col */
        if (guac_terminal_clear(term,
                end_row, 0, 1, end_col + 1))
            return 1;

        /* One less row to clear */
        end_row--;

    }

    /* Remaining region now guaranteed rectangular. Clear, if possible */
    if (start_row <= end_row) {

        if (guac_terminal_clear(term,
                start_row, 0, end_row - start_row + 1, term->term_width))
            return 1;

    }

    return 0;

}


void guac_terminal_delta_set(guac_terminal_delta* delta, int r, int c,
        guac_terminal_char* character) {

    /* Get operation at coordinate */
    guac_terminal_operation* op = &(delta->operations[r*delta->width + c]);

    /* Store operation */
    op->type = GUAC_CHAR_SET;
    op->character = *character;

}

void guac_terminal_delta_copy(guac_terminal_delta* delta,
        int dst_row, int dst_column,
        int src_row, int src_column,
        int w, int h) {

    int row, column;

    /* FIXME: Handle intersections between src and dst rects */

    memcpy(delta->scratch, delta->operations, 
            sizeof(guac_terminal_operation) * delta->width * delta->height);

    guac_terminal_operation* current_row =
        &(delta->operations[dst_row*delta->width + dst_column]);

    guac_terminal_operation* src_current_row =
        &(delta->scratch[src_row*delta->width + src_column]);

    /* Set rectangle to copy operations */
    for (row=0; row<h; row++) {

        guac_terminal_operation* current = current_row;
        guac_terminal_operation* src_current = src_current_row;

        for (column=0; column<w; column++) {

            /* If copying existing delta operation, just copy that rather
             * than create a new copy op */
            if (src_current->type != GUAC_CHAR_NOP)
                *current = *src_current;

            /* Store operation */
            else {
                current->type = GUAC_CHAR_COPY;
                current->row = src_row + row;
                current->column = src_column + column;
            }

            /* Next column */
            current++;
            src_current++;

        }

        /* Next row */
        current_row += delta->width;
        src_current_row += delta->width;

    }



}

void guac_terminal_delta_set_rect(guac_terminal_delta* delta,
        int row, int column, int w, int h,
        guac_terminal_char* character) {

    guac_terminal_operation* current_row =
        &(delta->operations[row*delta->width + column]);

    /* Set rectangle contents to given character */
    for (row=0; row<h; row++) {

        guac_terminal_operation* current = current_row;

        for (column=0; column<w; column++) {

            /* Store operation */
            current->type = GUAC_CHAR_SET;
            current->character = *character;

            /* Next column */
            current++;

        }

        /* Next row */
        current_row += delta->width;

    }

}

void guac_terminal_scrollback_buffer_append(
    guac_terminal_scrollback_buffer* buffer,
    guac_terminal* terminal, int rows) {

    int row, column;

    /* Copy data into scrollback */
    guac_terminal_scrollback_row* scrollback_row =
        &(buffer->scrollback[buffer->top]);
    guac_terminal_char* current = terminal->buffer->characters;

    for (row=0; row<rows; row++) {

        /* FIXME: Assumes scrollback row large enough */

        /* Copy character data for row */
        guac_terminal_char* dest = scrollback_row->characters;
        for (column=0; column < terminal->buffer->width; column++)
            *(dest++) = *(current++);

        scrollback_row->length = terminal->buffer->width;

        /* Next scrollback row */
        scrollback_row++;
        buffer->top++;

        /* Wrap around when bottom reached */
        if (buffer->top == buffer->rows) {
            buffer->top = 0;
            scrollback_row = buffer->scrollback;
        }

    } /* end for each row */

    /* Increment row count */
    buffer->length += rows;
    if (buffer->length > buffer->rows)
        buffer->length = buffer->rows;

}

guac_terminal_char* guac_terminal_get_row(guac_terminal* terminal, int row, int* length) {

    /* If row in past, pull from scrollback */
    if (row < 0) {
        guac_terminal_scrollback_row* scrollback_row = 
            guac_terminal_scrollback_buffer_get_row(terminal->scrollback, row);

        *length = scrollback_row->length;
        return scrollback_row->characters;
    }

    *length = terminal->buffer->width;
    return &(terminal->buffer->characters[terminal->buffer->width * row]); 

}

void guac_terminal_scroll_display_down(guac_terminal* terminal,
        int scroll_amount) {

    int start_row, end_row;
    int dest_row;
    int row, column;

    /* Limit scroll amount by size of scrollback buffer */
    if (scroll_amount > terminal->scroll_offset)
        scroll_amount = terminal->scroll_offset;

    /* If not scrolling at all, don't bother trying */
    if (scroll_amount == 0)
        return;

    /* Shift screen up */
    if (terminal->term_height > scroll_amount)
        guac_terminal_delta_copy(terminal->delta,
                0,             0, /* Destination row, col */
                scroll_amount, 0, /* source row,col */
                terminal->term_width, terminal->term_height - scroll_amount);

    /* Advance by scroll amount */
    terminal->scroll_offset -= scroll_amount;

    /* Get row range */
    end_row   = terminal->term_height - terminal->scroll_offset - 1;
    start_row = end_row - scroll_amount + 1;
    dest_row  = terminal->term_height - scroll_amount;

    /* Draw new rows from scrollback */
    for (row=start_row; row<=end_row; row++) {

        int length;
        guac_terminal_char* current = guac_terminal_get_row(terminal, row, &length);

        for (column=0; column<length; column++)
            guac_terminal_delta_set(terminal->delta, dest_row, column,
                    current++);

        /* Next row */
        dest_row++;

    }

    /* FIXME: Should flush somewhere more sensible */
    guac_terminal_delta_flush(terminal->delta, terminal);
    guac_socket_flush(terminal->client->socket);

}

void guac_terminal_scroll_display_up(guac_terminal* terminal,
        int scroll_amount) {

    int start_row, end_row;
    int dest_row;
    int row, column;


    /* Limit scroll amount by size of scrollback buffer */
    if (terminal->scroll_offset + scroll_amount > terminal->scrollback->length)
        scroll_amount = terminal->scrollback->length - terminal->scroll_offset;

    /* If not scrolling at all, don't bother trying */
    if (scroll_amount == 0)
        return;

    /* Shift screen down */
    if (terminal->term_height > scroll_amount)
        guac_terminal_delta_copy(terminal->delta,
                scroll_amount, 0, /* Destination row,col */
                0,             0, /* Source row, col */
                terminal->term_width, terminal->term_height - scroll_amount);

    /* Advance by scroll amount */
    terminal->scroll_offset += scroll_amount;

    /* Get row range */
    start_row = -terminal->scroll_offset;
    end_row   = start_row + scroll_amount - 1;
    dest_row  = 0;

    /* Draw new rows from scrollback */
    for (row=start_row; row<=end_row; row++) {

        /* Get row from scrollback */
        guac_terminal_scrollback_row* scrollback_row = 
            guac_terminal_scrollback_buffer_get_row(terminal->scrollback, row);

        /* Draw row */
        /* FIXME: Clear row first */
        guac_terminal_char* current = scrollback_row->characters;
        for (column=0; column<scrollback_row->length; column++)
            guac_terminal_delta_set(terminal->delta, dest_row, column,
                    current++);

        /* Next row */
        dest_row++;

    }

    /* FIXME: Should flush somewhere more sensible */
    guac_terminal_delta_flush(terminal->delta, terminal);
    guac_socket_flush(terminal->client->socket);

}

guac_terminal_scrollback_row* guac_terminal_scrollback_buffer_get_row(
    guac_terminal_scrollback_buffer* buffer, int row) {

    /* Calculate scrollback row index */
    int index = buffer->top + row;
    if (index < 0) index += buffer->rows;

    /* Return found row */
    return &(buffer->scrollback[index]);

}

void guac_terminal_select_start(guac_terminal* terminal, int row, int column) {

    guac_terminal_char* guac_char;
    guac_terminal_operation* guac_operation;

    /* Update selection coordinates */
    terminal->selection_start_row =
    terminal->selection_end_row   = row;
    terminal->selection_start_column =
    terminal->selection_end_column   = column;
    terminal->text_selected = true;

    /* Get char and operation */
    guac_char = &(terminal->buffer->characters[terminal->buffer->width * row + column]);
    guac_operation = &(terminal->delta->operations[terminal->delta->width * row + column]);

    /* Set character as selected */
    guac_char->attributes.selected = true;
    guac_operation->type = GUAC_CHAR_SET;
    guac_operation->character = *guac_char;

    guac_terminal_delta_flush(terminal->delta, terminal);
    guac_socket_flush(terminal->client->socket);

}

void guac_terminal_select_update(guac_terminal* terminal, int row, int column) {

    int start_index = terminal->selection_start_row * terminal->buffer->width
                    + terminal->selection_start_column;

    int old_end_index = terminal->selection_end_row * terminal->buffer->width
                      + terminal->selection_end_column;

    int new_end_index = row * terminal->buffer->width + column;

    int old_index_a, old_index_b;
    int new_index_a, new_index_b;

    int search_index_a, search_index_b;

    int i;
    guac_terminal_char* guac_char;
    guac_terminal_operation* guac_operation;

    /* If unchanged, do nothing */
    if (old_end_index == new_end_index) return;

    /* Calculate old selection range */
    if (start_index < old_end_index) {
        old_index_a = start_index;
        old_index_b = old_end_index;
    }
    else {
        old_index_a = old_end_index;
        old_index_b = start_index;
    }

    /* Calculate new selection range */
    if (start_index < new_end_index) {
        new_index_a = start_index;
        new_index_b = new_end_index;
    }
    else {
        new_index_a = new_end_index;
        new_index_b = start_index;
    }

    if (new_index_a < old_index_a)
        search_index_a = new_index_a;
    else
        search_index_a = old_index_a;

    if (new_index_b > old_index_b)
        search_index_b = new_index_b;
    else
        search_index_b = old_index_b;

    /* Get first character */
    guac_char = &(terminal->buffer->characters[search_index_a]);
    guac_operation = &(terminal->delta->operations[search_index_a]);

    /* Invert modified area */
    for (i=search_index_a; i<=search_index_b; i++) {

        /* If now selected, mark as such */
        if (i >= new_index_a && i <= new_index_b &&
                (i < old_index_a || i > old_index_b)) {

            guac_char->attributes.selected = true;
            guac_operation->type = GUAC_CHAR_SET;
            guac_operation->character = *guac_char;

        }

        /* If now unselected, mark as such */
        else if (i >= old_index_a && i <= old_index_b &&
                (i < new_index_a || i > new_index_b)) {

            guac_char->attributes.selected = false;
            guac_operation->type = GUAC_CHAR_SET;
            guac_operation->character = *guac_char;

        }

        /* Next char */
        guac_char++;
        guac_operation++;

    }

    terminal->selection_end_row = row;
    terminal->selection_end_column = column;

    guac_terminal_delta_flush(terminal->delta, terminal);
    guac_socket_flush(terminal->client->socket);

}

void guac_terminal_select_end(guac_terminal* terminal) {

    int index_a = terminal->selection_end_row * terminal->buffer->width
        + terminal->selection_end_column;

    int index_b = terminal->selection_start_row * terminal->buffer->width
        + terminal->selection_start_column;

    int i;
    guac_terminal_char* guac_char;
    guac_terminal_operation* guac_operation;

    /* The start and end indices of all characters in selection */
    int start_index;
    int end_index;

    /* Order indices such that end is after start */
    if (index_a > index_b) {
        start_index = index_b;
        end_index   = index_a;
    }
    else {
        start_index = index_a;
        end_index   = index_b;
    }

    /* Get first character */
    guac_char = &(terminal->buffer->characters[start_index]);
    guac_operation = &(terminal->delta->operations[start_index]);

    /* Restore state from buffer */
    for (i=start_index; i<=end_index; i++) {

        /* Restore state */
        guac_char->attributes.selected = false;
        guac_operation->type = GUAC_CHAR_SET;
        guac_operation->character = *guac_char;

        /* Next char */
        guac_char++;
        guac_operation++;

    }

    terminal->text_selected = false;

    guac_terminal_delta_flush(terminal->delta, terminal);
    guac_socket_flush(terminal->client->socket);

}

