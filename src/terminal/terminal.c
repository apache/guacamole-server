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

#include "common/clipboard.h"
#include "common/cursor.h"
#include "terminal/buffer.h"
#include "terminal/color-scheme.h"
#include "terminal/common.h"
#include "terminal/display.h"
#include "terminal/palette.h"
#include "terminal/select.h"
#include "terminal/terminal.h"
#include "terminal/terminal_handlers.h"
#include "terminal/types.h"
#include "terminal/typescript.h"

#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <wchar.h>

#include <guacamole/client.h>
#include <guacamole/error.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/timestamp.h>

/**
 * Sets the given range of columns to the given character.
 */
static void __guac_terminal_set_columns(guac_terminal* terminal, int row,
        int start_column, int end_column, guac_terminal_char* character) {

    guac_terminal_display_set_columns(terminal->display, row + terminal->scroll_offset,
            start_column, end_column, character);

    guac_terminal_buffer_set_columns(terminal->buffer, row,
            start_column, end_column, character);

    /* Clear selection if region is modified */
    guac_terminal_select_touch(terminal, row, start_column, row, end_column);

}

/**
 * Enforces a character break at the given edge, ensuring that the left side
 * of the edge is the final column of a character, and the right side of the
 * edge is the initial column of a DIFFERENT character.
 *
 * For a character in a column N, the left edge number is N, and the right
 * edge is N+1.
 */
static void __guac_terminal_force_break(guac_terminal* terminal, int row, int edge) {

    guac_terminal_buffer_row* buffer_row = guac_terminal_buffer_get_row(terminal->buffer, row, 0);

    /* Ensure character to left of edge is unbroken */
    if (edge > 0) {

        int end_column = edge - 1;
        int start_column = end_column;

        guac_terminal_char* start_char = &(buffer_row->characters[start_column]);

        /* Determine start column */
        while (start_column > 0 && start_char->value == GUAC_CHAR_CONTINUATION) {
            start_char--;
            start_column--;
        }

        /* Advance to start of broken character if necessary */
        if (start_char->value != GUAC_CHAR_CONTINUATION && start_char->width < end_column - start_column + 1) {
            start_column += start_char->width;
            start_char += start_char->width;
        }

        /* Clear character if broken */
        if (start_char->value == GUAC_CHAR_CONTINUATION || start_char->width != end_column - start_column + 1) {

            guac_terminal_char cleared_char;
            cleared_char.value = ' ';
            cleared_char.attributes = start_char->attributes;
            cleared_char.width = 1;

            __guac_terminal_set_columns(terminal, row, start_column, end_column, &cleared_char);

        }

    }

    /* Ensure character to right of edge is unbroken */
    if (edge >= 0 && edge < buffer_row->length) {

        int start_column = edge;
        int end_column = start_column;

        guac_terminal_char* start_char = &(buffer_row->characters[start_column]);
        guac_terminal_char* end_char = &(buffer_row->characters[end_column]);

        /* Determine end column */
        while (end_column+1 < buffer_row->length && (end_char+1)->value == GUAC_CHAR_CONTINUATION) {
            end_char++;
            end_column++;
        }

        /* Advance to start of broken character if necessary */
        if (start_char->value != GUAC_CHAR_CONTINUATION && start_char->width < end_column - start_column + 1) {
            start_column += start_char->width;
            start_char += start_char->width;
        }

        /* Clear character if broken */
        if (start_char->value == GUAC_CHAR_CONTINUATION || start_char->width != end_column - start_column + 1) {

            guac_terminal_char cleared_char;
            cleared_char.value = ' ';
            cleared_char.attributes = start_char->attributes;
            cleared_char.width = 1;

            __guac_terminal_set_columns(terminal, row, start_column, end_column, &cleared_char);

        }

    }

}

/**
 * Returns the number of rows available within the terminal buffer, taking
 * changes to the desired scrollback size into account. Regardless of the
 * true buffer length, only the number of rows that should be made available
 * will be returned.
 *
 * @param term
 *     The terminal whose effective buffer length should be retrieved.
 *
 * @return
 *     The number of rows effectively available within the terminal buffer,
 *     taking changes to the desired scrollback size into account.
 */
static int guac_terminal_effective_buffer_length(guac_terminal* term) {

    int scrollback = term->requested_scrollback;

    /* Limit available scrollback to defined maximum */
    if (scrollback > term->max_scrollback)
        scrollback = term->max_scrollback;

    /* There must always be at least enough scrollback to cover the visible
     * terminal display */
    else if (scrollback < term->term_height)
        scrollback = term->term_height;

    /* If the buffer contains more rows than requested, pretend it only
     * contains the requested number of rows */
    int effective_length = term->buffer->length;
    if (effective_length > scrollback)
        effective_length = scrollback;

    return effective_length;

}

int guac_terminal_available_scroll(guac_terminal* term) {
    return guac_terminal_effective_buffer_length(term) - term->term_height;
}

void guac_terminal_reset(guac_terminal* term) {

    int row;

    /* Set current state */
    term->char_handler = guac_terminal_echo; 
    term->active_char_set = 0;
    term->char_mapping[0] =
    term->char_mapping[1] = NULL;

    /* Reset cursor location */
    term->cursor_row = term->visible_cursor_row = term->saved_cursor_row = 0;
    term->cursor_col = term->visible_cursor_col = term->saved_cursor_col = 0;
    term->cursor_visible = true;

    /* Clear scrollback, buffer, and scroll region */
    term->buffer->top = 0;
    term->buffer->length = 0;
    term->scroll_start = 0;
    term->scroll_end = term->term_height - 1;
    term->scroll_offset = 0;

    /* Reset scrollbar bounds */
    guac_terminal_scrollbar_set_bounds(term->scrollbar, 0, 0);
    guac_terminal_scrollbar_set_value(term->scrollbar, -term->scroll_offset);

    /* Reset flags */
    term->text_selected = false;
    term->selection_committed = false;
    term->application_cursor_keys = false;
    term->automatic_carriage_return = false;
    term->insert_mode = false;

    /* Reset tabs */
    term->tab_interval = 8;
    memset(term->custom_tabs, 0, sizeof(term->custom_tabs));

    /* Reset character attributes */
    term->current_attributes = term->default_char.attributes;

    /* Reset display palette */
    guac_terminal_display_reset_palette(term->display);

    /* Clear terminal */
    for (row=0; row<term->term_height; row++)
        guac_terminal_set_columns(term, row, 0, term->term_width, &(term->default_char));

}

/**
 * Paints or repaints the background of the terminal display. This painting
 * occurs beneath the actual terminal and scrollbar layers, and thus will not
 * overwrite any text or other content currently on the screen. This is only
 * necessary to paint over parts of the terminal background which may otherwise
 * be transparent (the default layer background).
 *
 * @param terminal
 *     The terminal whose background should be painted or repainted.
 *
 * @param socket
 *     The socket over which instructions required to paint / repaint the
 *     terminal background should be send.
 */
static void guac_terminal_repaint_default_layer(guac_terminal* terminal,
        guac_socket* socket) {

    int width = terminal->width;
    int height = terminal->height;
    guac_terminal_display* display = terminal->display;

    /* Get background color */
    const guac_terminal_color* color = &display->default_background;

    /* Reset size */
    guac_protocol_send_size(socket, GUAC_DEFAULT_LAYER, width, height);

    /* Paint background color */
    guac_protocol_send_rect(socket, GUAC_DEFAULT_LAYER, 0, 0, width, height);
    guac_protocol_send_cfill(socket, GUAC_COMP_OVER, GUAC_DEFAULT_LAYER,
            color->red, color->green, color->blue, 0xFF);

}

/**
 * Automatically and continuously renders frames of terminal data while the
 * associated guac_client is running.
 *
 * @param data
 *     A pointer to the guac_terminal that should be continuously rendered
 *     while its associated guac_client is running.
 *
 * @return
 *     Always NULL.
 */
void* guac_terminal_thread(void* data) {

    guac_terminal* terminal = (guac_terminal*) data;
    guac_client* client = terminal->client;

    /* Render frames only while client is running */
    while (client->state == GUAC_CLIENT_RUNNING) {

        /* Stop rendering if an error occurs */
        if (guac_terminal_render_frame(terminal))
            break;

        /* Signal end of frame */
        guac_client_end_frame(client);
        guac_socket_flush(client->socket);

    }

    /* The client has stopped or an error has occurred */
    return NULL;

}

guac_terminal* guac_terminal_create(guac_client* client,
        guac_common_clipboard* clipboard, bool disable_copy,
        int max_scrollback, const char* font_name, int font_size, int dpi,
        int width, int height, const char* color_scheme,
        const int backspace) {

    /* Build default character using default colors */
    guac_terminal_char default_char = {
        .value = 0,
        .attributes = {
            .bold        = false,
            .half_bright = false,
            .reverse     = false,
            .underscore  = false
        },
        .width = 1
    };

    /* Initialized by guac_terminal_parse_color_scheme. */
    guac_terminal_color (*default_palette)[256] = (guac_terminal_color(*)[256])
            malloc(sizeof(guac_terminal_color[256]));

    guac_terminal_parse_color_scheme(client, color_scheme,
                                     &default_char.attributes.foreground,
                                     &default_char.attributes.background,
                                     default_palette);

    /* Calculate available display area */
    int available_width = width - GUAC_TERMINAL_SCROLLBAR_WIDTH;
    if (available_width < 0)
        available_width = 0;

    guac_terminal* term = malloc(sizeof(guac_terminal));
    term->started = false;
    term->client = client;
    term->upload_path_handler = NULL;
    term->file_download_handler = NULL;

    /* Copy initially-provided color scheme and font details */
    term->color_scheme = strdup(color_scheme);
    term->font_name = strdup(font_name);
    term->font_size = font_size;

    /* Set size of available screen area */
    term->outer_width = width;
    term->outer_height = height;

    /* Init modified flag and conditional */
    term->modified = 0;
    pthread_cond_init(&(term->modified_cond), NULL);
    pthread_mutex_init(&(term->modified_lock), NULL);

    /* Maximum and requested scrollback are initially the same */
    term->max_scrollback = max_scrollback;
    term->requested_scrollback = max_scrollback;

    /* Allocate enough space for maximum scrollback, bumping up internal
     * storage as necessary to allow screen to be resized to maximum height */
    int initial_scrollback = max_scrollback;
    if (initial_scrollback < GUAC_TERMINAL_MAX_ROWS)
        initial_scrollback = GUAC_TERMINAL_MAX_ROWS;

    /* Init buffer */
    term->buffer = guac_terminal_buffer_alloc(initial_scrollback,
            &default_char);

    /* Init display */
    term->display = guac_terminal_display_alloc(client,
            font_name, font_size, dpi,
            &default_char.attributes.foreground,
            &default_char.attributes.background,
            (guac_terminal_color(*)[256]) default_palette);

    /* Fail if display init failed */
    if (term->display == NULL) {
        guac_client_log(client, GUAC_LOG_DEBUG, "Display initialization failed");
        free(term);
        return NULL;
    }

    /* Init common cursor */
    term->cursor = guac_common_cursor_alloc(client);

    /* Init terminal state */
    term->current_attributes = default_char.attributes;
    term->default_char = default_char;
    term->clipboard = clipboard;
    term->disable_copy = disable_copy;

    /* Calculate character size */
    int rows    = height / term->display->char_height;
    int columns = available_width / term->display->char_width;

    /* Keep height within predefined maximum */
    if (rows > GUAC_TERMINAL_MAX_ROWS) {
        rows = GUAC_TERMINAL_MAX_ROWS;
        height = rows * term->display->char_height;
    }

    /* Keep width within predefined maximum */
    if (columns > GUAC_TERMINAL_MAX_COLUMNS) {
        columns = GUAC_TERMINAL_MAX_COLUMNS;
        available_width = columns * term->display->char_width;
        width = available_width + GUAC_TERMINAL_SCROLLBAR_WIDTH;
    }

    /* Set pixel size */
    term->width = width;
    term->height = height;

    term->term_width  = columns;
    term->term_height = rows;

    /* Open STDIN pipe */
    if (pipe(term->stdin_pipe_fd)) {
        guac_error = GUAC_STATUS_SEE_ERRNO;
        guac_error_message = "Unable to open pipe for STDIN";
        free(term);
        return NULL;
    }

    /* Read input from keyboard by default */
    term->input_stream = NULL;

    /* Init pipe stream (output to display by default) */
    term->pipe_stream = NULL;

    /* No typescript by default */
    term->typescript = NULL;

    /* Init terminal lock */
    pthread_mutex_init(&(term->lock), NULL);

    /* Repaint and resize overall display */
    guac_terminal_repaint_default_layer(term, client->socket);
    guac_terminal_display_resize(term->display,
            term->term_width, term->term_height);

    /* Allocate scrollbar */
    term->scrollbar = guac_terminal_scrollbar_alloc(client, GUAC_DEFAULT_LAYER,
            width, height, term->term_height);

    /* Associate scrollbar with this terminal */
    term->scrollbar->data = term;
    term->scrollbar->scroll_handler = guac_terminal_scroll_handler;

    /* Init terminal */
    guac_terminal_reset(term);

    term->mod_alt   =
    term->mod_ctrl  =
    term->mod_shift = 0;

    /* Initialize mouse cursor */
    term->current_cursor = GUAC_TERMINAL_CURSOR_BLANK;
    guac_common_cursor_set_blank(term->cursor);

    /* Start terminal thread */
    if (pthread_create(&(term->thread), NULL,
                guac_terminal_thread, (void*) term)) {
        guac_terminal_free(term);
        return NULL;
    }

    /* Configure backspace */
    term->backspace = backspace;

    return term;

}

void guac_terminal_start(guac_terminal* term) {
    term->started = true;
    guac_terminal_notify(term);
}

void guac_terminal_stop(guac_terminal* term) {

    /* Close input pipe and set fds to invalid */
    if (term->stdin_pipe_fd[1] != -1) {
        close(term->stdin_pipe_fd[1]);
        term->stdin_pipe_fd[1] = -1;
    }
    if (term->stdin_pipe_fd[0] != -1) {
        close(term->stdin_pipe_fd[0]);
        term->stdin_pipe_fd[0] = -1;
    }
}

void guac_terminal_free(guac_terminal* term) {

    /* Close user input pipe */
    guac_terminal_stop(term);

    /* Wait for render thread to finish */
    pthread_join(term->thread, NULL);

    /* Close and flush any open pipe stream */
    guac_terminal_pipe_stream_close(term);

    /* Close and flush any active typescript */
    guac_terminal_typescript_free(term->typescript);

    /* Free display */
    guac_terminal_display_free(term->display);

    /* Free buffer */
    guac_terminal_buffer_free(term->buffer);

    /* Free scrollbar */
    guac_terminal_scrollbar_free(term->scrollbar);

    /* Free copies of font and color scheme information */
    free((char*) term->color_scheme);
    free((char*) term->font_name);

    /* Free the terminal itself */
    free(term);

}

/**
 * Populate the given timespec with the current time, plus the given offset.
 *
 * @param ts
 *     The timespec structure to populate.
 *
 * @param offset_sec
 *     The offset from the current time to use when populating the given
 *     timespec, in seconds.
 *
 * @param offset_usec
 *     The offset from the current time to use when populating the given
 *     timespec, in microseconds.
 */
static void guac_terminal_get_absolute_time(struct timespec* ts,
        int offset_sec, int offset_usec) {

    /* Get timeval */
    struct timeval tv;
    gettimeofday(&tv, NULL);

    /* Update with offset */
    tv.tv_sec  += offset_sec;
    tv.tv_usec += offset_usec;

    /* Wrap to next second if necessary */
    if (tv.tv_usec >= 1000000) {
        tv.tv_sec++;
        tv.tv_usec -= 1000000;
    }

    /* Convert to timespec */
    ts->tv_sec  = tv.tv_sec;
    ts->tv_nsec = tv.tv_usec * 1000;

}

/**
 * Waits for the terminal state to be modified, returning only when the
 * specified timeout has elapsed or a frame flush is desired. Note that the
 * modified flag of the terminal will only be reset if no data remains to be
 * read from STDOUT.
 *
 * @param terminal
 *    The terminal to wait on.
 *
 * @param msec_timeout
 *    The maximum amount of time to wait, in milliseconds.
 *
 * @return
 *    Non-zero if the terminal has been modified, zero if the timeout has
 *    elapsed without the terminal being modified.
 */
static int guac_terminal_wait(guac_terminal* terminal, int msec_timeout) {

    int retval = 1;

    pthread_mutex_t* mod_lock = &(terminal->modified_lock);
    pthread_cond_t* mod_cond = &(terminal->modified_cond);

    /* Split provided milliseconds into microseconds and whole seconds */
    int secs  =  msec_timeout / 1000;
    int usecs = (msec_timeout % 1000) * 1000;

    /* Calculate absolute timestamp from provided relative timeout */
    struct timespec timeout;
    guac_terminal_get_absolute_time(&timeout, secs, usecs);

    /* Test for terminal modification */
    pthread_mutex_lock(mod_lock);
    if (terminal->modified)
        goto wait_complete;

    /* If not yet modified, wait for modification condition to be signaled */
    retval = pthread_cond_timedwait(mod_cond, mod_lock, &timeout) != ETIMEDOUT;

wait_complete:

    /* Terminal is no longer modified */
    terminal->modified = 0;
    pthread_mutex_unlock(mod_lock);
    return retval;

}

int guac_terminal_render_frame(guac_terminal* terminal) {

    guac_client* client = terminal->client;

    int wait_result;

    /* Wait for data to be available */
    wait_result = guac_terminal_wait(terminal, 1000);
    if (wait_result || !terminal->started) {

        guac_timestamp frame_start = guac_timestamp_current();

        do {

            /* Calculate time remaining in frame */
            guac_timestamp frame_end = guac_timestamp_current();
            int frame_remaining = frame_start + GUAC_TERMINAL_FRAME_DURATION
                                - frame_end;

            /* Wait again if frame remaining */
            if (frame_remaining > 0 || !terminal->started)
                wait_result = guac_terminal_wait(terminal,
                        GUAC_TERMINAL_FRAME_TIMEOUT);
            else
                break;

        } while (client->state == GUAC_CLIENT_RUNNING
                && (wait_result > 0 || !terminal->started));

        /* Flush terminal */
        guac_terminal_lock(terminal);
        guac_terminal_flush(terminal);
        guac_terminal_unlock(terminal);

    }

    return 0;

}

int guac_terminal_read_stdin(guac_terminal* terminal, char* c, int size) {
    int stdin_fd = terminal->stdin_pipe_fd[0];
    return read(stdin_fd, c, size);
}

void guac_terminal_notify(guac_terminal* terminal) {

    pthread_mutex_t* mod_lock = &(terminal->modified_lock);
    pthread_cond_t* mod_cond = &(terminal->modified_cond);

    pthread_mutex_lock(mod_lock);

    /* Signal modification */
    terminal->modified = 1;
    pthread_cond_signal(mod_cond);

    pthread_mutex_unlock(mod_lock);

}

int guac_terminal_printf(guac_terminal* terminal, const char* format, ...) {

    int written;

    va_list ap;
    char buffer[1024];

    /* Print to buffer */
    va_start(ap, format);
    written = vsnprintf(buffer, sizeof(buffer)-1, format, ap);
    va_end(ap);

    if (written < 0)
        return written;

    /* Write to STDOUT */
    return guac_terminal_write(terminal, buffer, written);

}

char* guac_terminal_prompt(guac_terminal* terminal, const char* title,
        bool echo) {

    char buffer[1024];

    int pos;
    char in_byte;

    /* Prompting implicitly requires user input */
    guac_terminal_start(terminal);

    /* Print title */
    guac_terminal_printf(terminal, "%s", title);

    /* Read bytes until newline */
    pos = 0;
    while (guac_terminal_read_stdin(terminal, &in_byte, 1) == 1) {

        /* Backspace */
        if (in_byte == 0x7F) {
            if (pos > 0) {
                guac_terminal_printf(terminal, "\b \b");
                pos--;
            }
        }

        /* CR (end of input */
        else if (in_byte == 0x0D) {
            guac_terminal_printf(terminal, "\r\n");
            break;
        }

        /* Otherwise, store byte if there is room */
        else if (pos < sizeof(buffer) - 1) {

            /* Store character, update buffers */
            buffer[pos++] = in_byte;

            /* Print character if echoing */
            if (echo)
                guac_terminal_printf(terminal, "%c", in_byte);
            else
                guac_terminal_printf(terminal, "*");

        }

        /* Ignore all other input */

    }

    /* Terminate string */
    buffer[pos] = 0;

    /* Return newly-allocated string containing result */
    return strdup(buffer);

}

int guac_terminal_set(guac_terminal* term, int row, int col, int codepoint) {

    /* Calculate width in columns */
    int width = wcwidth(codepoint);
    if (width < 0)
        width = 1;

    /* Do nothing if glyph is empty */
    else if (width == 0)
        return 0;

    /* Build character with current attributes */
    guac_terminal_char guac_char = {
        .value      = codepoint,
        .attributes = term->current_attributes,
        .width      = width
    };

    guac_terminal_set_columns(term, row, col, col + width - 1, &guac_char);

    return 0;

}

void guac_terminal_commit_cursor(guac_terminal* term) {

    guac_terminal_char* guac_char;

    guac_terminal_buffer_row* row;

    /* If no change, done */
    if (term->cursor_visible && term->visible_cursor_row == term->cursor_row && term->visible_cursor_col == term->cursor_col)
        return;

    /* Clear cursor if it was visible */
    if (term->visible_cursor_row != -1 && term->visible_cursor_col != -1) {
        /* Get old row with cursor */
        row = guac_terminal_buffer_get_row(term->buffer, term->visible_cursor_row, term->visible_cursor_col+1);

        guac_char = &(row->characters[term->visible_cursor_col]);
        guac_char->attributes.cursor = false;
        guac_terminal_display_set_columns(term->display, term->visible_cursor_row + term->scroll_offset,
                term->visible_cursor_col, term->visible_cursor_col, guac_char);
    }

    /* Set cursor if should be visible */
    if (term->cursor_visible) {
        /* Get new row with cursor */
        row = guac_terminal_buffer_get_row(term->buffer, term->cursor_row, term->cursor_col+1);

        guac_char = &(row->characters[term->cursor_col]);
        guac_char->attributes.cursor = true;
        guac_terminal_display_set_columns(term->display, term->cursor_row + term->scroll_offset,
                term->cursor_col, term->cursor_col, guac_char);

        term->visible_cursor_row = term->cursor_row;
        term->visible_cursor_col = term->cursor_col;
    }

    /* Otherwise set visible position to a sentinel value */
    else {
        term->visible_cursor_row = -1;
        term->visible_cursor_col = -1;
    }

    return;

}

int guac_terminal_write(guac_terminal* term, const char* c, int size) {

    guac_terminal_lock(term);
    while (size > 0) {

        /* Read and advance to next character */
        char current = *(c++);
        size--;

        /* Write character to typescript, if any */
        if (term->typescript != NULL)
            guac_terminal_typescript_write(term->typescript, current);

        /* Handle character and its meaning */
        term->char_handler(term, current);

    }
    guac_terminal_unlock(term);

    guac_terminal_notify(term);
    return 0;

}

int guac_terminal_scroll_up(guac_terminal* term,
        int start_row, int end_row, int amount) {

    /* If scrolling entire display, update scroll offset */
    if (start_row == 0 && end_row == term->term_height - 1) {

        /* Scroll up visibly */
        guac_terminal_display_copy_rows(term->display, start_row + amount, end_row, -amount);

        /* Advance by scroll amount */
        term->buffer->top += amount;
        if (term->buffer->top >= term->buffer->available)
            term->buffer->top -= term->buffer->available;

        term->buffer->length += amount;
        if (term->buffer->length > term->buffer->available)
            term->buffer->length = term->buffer->available;

        /* Reset scrollbar bounds */
        guac_terminal_scrollbar_set_bounds(term->scrollbar,
                -guac_terminal_available_scroll(term), 0);

        /* Update cursor location if within region */
        if (term->visible_cursor_row >= start_row &&
            term->visible_cursor_row <= end_row)
            term->visible_cursor_row -= amount;

        /* Update selected region */
        if (term->text_selected) {
            term->selection_start_row -= amount;
            term->selection_end_row -= amount;
        }

    }

    /* Otherwise, just copy row data upwards */
    else
        guac_terminal_copy_rows(term, start_row + amount, end_row, -amount);

    /* Clear new area */
    guac_terminal_clear_range(term,
            end_row - amount + 1, 0,
            end_row, term->term_width - 1);

    return 0;
}

int guac_terminal_scroll_down(guac_terminal* term,
        int start_row, int end_row, int amount) {

    guac_terminal_copy_rows(term, start_row, end_row - amount, amount);

    /* Clear new area */
    guac_terminal_clear_range(term,
            start_row, 0,
            start_row + amount - 1, term->term_width - 1);

    return 0;
}

int guac_terminal_clear_columns(guac_terminal* term,
        int row, int start_col, int end_col) {

    /* Build space */
    guac_terminal_char blank;
    blank.value = 0;
    blank.attributes = term->current_attributes;
    blank.width = 1;

    /* Clear */
    guac_terminal_set_columns(term,
        row, start_col, end_col, &blank);

    return 0;

}

int guac_terminal_clear_range(guac_terminal* term,
        int start_row, int start_col,
        int end_row, int end_col) {

    /* If not at far left, must clear sub-region to far right */
    if (start_col > 0) {

        /* Clear from start_col to far right */
        guac_terminal_clear_columns(term,
            start_row, start_col, term->term_width - 1);

        /* One less row to clear */
        start_row++;
    }

    /* If not at far right, must clear sub-region to far left */
    if (end_col < term->term_width - 1) {

        /* Clear from far left to end_col */
        guac_terminal_clear_columns(term, end_row, 0, end_col);

        /* One less row to clear */
        end_row--;

    }

    /* Remaining region now guaranteed rectangular. Clear, if possible */
    if (start_row <= end_row) {

        int row;
        for (row=start_row; row<=end_row; row++) {

            /* Clear entire row */
            guac_terminal_clear_columns(term, row, 0, term->term_width - 1);

        }

    }

    return 0;

}

/**
 * Returns whether the given character would be visible relative to the
 * background of the given terminal.
 *
 * @param term
 *     The guac_terminal to test the character against.
 *
 * @param c
 *     The character being tested.
 *
 * @return
 *     true if the given character is different from the terminal background,
 *     false otherwise.
 */
static bool guac_terminal_is_visible(guac_terminal* term,
        guac_terminal_char* c) {

    /* Continuation characters are NEVER visible */
    if (c->value == GUAC_CHAR_CONTINUATION)
        return false;

    /* Characters with glyphs are ALWAYS visible */
    if (guac_terminal_has_glyph(c->value))
        return true;

    const guac_terminal_color* background;

    /* Determine actual background color of character */
    if (c->attributes.reverse != c->attributes.cursor)
        background = &c->attributes.foreground;
    else
        background = &c->attributes.background;

    /* Blank characters are visible if their background color differs from that
     * of the terminal */
    return guac_terminal_colorcmp(background,
            &term->default_char.attributes.background) != 0;

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
    if (scroll_amount <= 0)
        return;

    /* Shift screen up */
    if (terminal->term_height > scroll_amount)
        guac_terminal_display_copy_rows(terminal->display,
                scroll_amount, terminal->term_height - 1,
                -scroll_amount);

    /* Advance by scroll amount */
    terminal->scroll_offset -= scroll_amount;
    guac_terminal_scrollbar_set_value(terminal->scrollbar, -terminal->scroll_offset);

    /* Get row range */
    end_row   = terminal->term_height - terminal->scroll_offset - 1;
    start_row = end_row - scroll_amount + 1;
    dest_row  = terminal->term_height - scroll_amount;

    /* Draw new rows from scrollback */
    for (row=start_row; row<=end_row; row++) {

        /* Get row from scrollback */
        guac_terminal_buffer_row* buffer_row =
            guac_terminal_buffer_get_row(terminal->buffer, row, 0);

        /* Clear row */
        guac_terminal_display_set_columns(terminal->display,
                dest_row, 0, terminal->display->width, &(terminal->default_char));

        /* Draw row */
        guac_terminal_char* current = buffer_row->characters;
        for (column=0; column<buffer_row->length; column++) {

            /* Only draw if not blank */
            if (guac_terminal_is_visible(terminal, current))
                guac_terminal_display_set_columns(terminal->display, dest_row, column, column, current);

            current++;

        }

        /* Next row */
        dest_row++;

    }

    guac_terminal_notify(terminal);

}

void guac_terminal_scroll_display_up(guac_terminal* terminal,
        int scroll_amount) {

    int start_row, end_row;
    int dest_row;
    int row, column;

    /* Limit scroll amount by size of scrollback buffer */
    int available_scroll = guac_terminal_available_scroll(terminal);
    if (terminal->scroll_offset + scroll_amount > available_scroll)
        scroll_amount = available_scroll - terminal->scroll_offset;

    /* If not scrolling at all, don't bother trying */
    if (scroll_amount <= 0)
        return;

    /* Shift screen down */
    if (terminal->term_height > scroll_amount)
        guac_terminal_display_copy_rows(terminal->display,
                0, terminal->term_height - scroll_amount - 1,
                scroll_amount);

    /* Advance by scroll amount */
    terminal->scroll_offset += scroll_amount;
    guac_terminal_scrollbar_set_value(terminal->scrollbar, -terminal->scroll_offset);

    /* Get row range */
    start_row = -terminal->scroll_offset;
    end_row   = start_row + scroll_amount - 1;
    dest_row  = 0;

    /* Draw new rows from scrollback */
    for (row=start_row; row<=end_row; row++) {

        /* Get row from scrollback */
        guac_terminal_buffer_row* buffer_row = 
            guac_terminal_buffer_get_row(terminal->buffer, row, 0);

        /* Clear row */
        guac_terminal_display_set_columns(terminal->display,
                dest_row, 0, terminal->display->width, &(terminal->default_char));

        /* Draw row */
        guac_terminal_char* current = buffer_row->characters;
        for (column=0; column<buffer_row->length; column++) {

            /* Only draw if not blank */
            if (guac_terminal_is_visible(terminal, current))
                guac_terminal_display_set_columns(terminal->display, dest_row, column, column, current);

            current++;

        }

        /* Next row */
        dest_row++;

    }

    guac_terminal_notify(terminal);

}

void guac_terminal_copy_columns(guac_terminal* terminal, int row,
        int start_column, int end_column, int offset) {

    guac_terminal_display_copy_columns(terminal->display, row + terminal->scroll_offset,
            start_column, end_column, offset);

    guac_terminal_buffer_copy_columns(terminal->buffer, row,
            start_column, end_column, offset);

    /* Clear selection if region is modified */
    guac_terminal_select_touch(terminal, row, start_column, row, end_column);

    /* Update cursor location if within region */
    if (row == terminal->visible_cursor_row &&
            terminal->visible_cursor_col >= start_column &&
            terminal->visible_cursor_col <= end_column)
        terminal->visible_cursor_col += offset;

    /* Force breaks around destination region */
    __guac_terminal_force_break(terminal, row, start_column + offset);
    __guac_terminal_force_break(terminal, row, end_column + offset + 1);

}

void guac_terminal_copy_rows(guac_terminal* terminal,
        int start_row, int end_row, int offset) {

    guac_terminal_display_copy_rows(terminal->display,
            start_row + terminal->scroll_offset, end_row + terminal->scroll_offset, offset);

    guac_terminal_buffer_copy_rows(terminal->buffer,
            start_row, end_row, offset);

    /* Clear selection if region is modified */
    guac_terminal_select_touch(terminal, start_row, 0, end_row,
            terminal->term_width);

    /* Update cursor location if within region */
    if (terminal->visible_cursor_row >= start_row &&
        terminal->visible_cursor_row <= end_row)
        terminal->visible_cursor_row += offset;

}

void guac_terminal_set_columns(guac_terminal* terminal, int row,
        int start_column, int end_column, guac_terminal_char* character) {

    __guac_terminal_set_columns(terminal, row, start_column, end_column, character);

    /* If visible cursor in current row, preserve state */
    if (row == terminal->visible_cursor_row
            && terminal->visible_cursor_col >= start_column
            && terminal->visible_cursor_col <= end_column) {

        /* Create copy of character with cursor attribute set */
        guac_terminal_char cursor_character = *character;
        cursor_character.attributes.cursor = true;

        __guac_terminal_set_columns(terminal, row,
                terminal->visible_cursor_col, terminal->visible_cursor_col, &cursor_character);

    }

    /* Force breaks around destination region */
    __guac_terminal_force_break(terminal, row, start_column);
    __guac_terminal_force_break(terminal, row, end_column + 1);

}

static void __guac_terminal_redraw_rect(guac_terminal* term, int start_row, int start_col, int end_row, int end_col) {

    int row, col;

    /* Redraw region */
    for (row=start_row; row<=end_row; row++) {

        guac_terminal_buffer_row* buffer_row =
            guac_terminal_buffer_get_row(term->buffer, row - term->scroll_offset, 0);

        /* Clear row */
        guac_terminal_display_set_columns(term->display,
                row, start_col, end_col, &(term->default_char));

        /* Copy characters */
        for (col=start_col; col <= end_col && col < buffer_row->length; col++) {

            /* Only redraw if not blank */
            guac_terminal_char* c = &(buffer_row->characters[col]);
            if (guac_terminal_is_visible(term, c))
                guac_terminal_display_set_columns(term->display, row, col, col, c);

        }

    }

}

/**
 * Internal terminal resize routine. Accepts width/height in CHARACTERS
 * (not pixels like the public function).
 */
static void __guac_terminal_resize(guac_terminal* term, int width, int height) {

    /* If height is decreasing, shift display up */
    if (height < term->term_height) {

        int shift_amount;

        /* Get number of rows actually occupying terminal space */
        int used_height = guac_terminal_effective_buffer_length(term);
        if (used_height > term->term_height)
            used_height = term->term_height;

        shift_amount = used_height - height;

        /* If the new terminal bottom covers N rows, shift up N rows */
        if (shift_amount > 0) {

            guac_terminal_display_copy_rows(term->display,
                    shift_amount, term->display->height - 1, -shift_amount);

            /* Update buffer top and cursor row based on shift */
            term->buffer->top += shift_amount;
            term->cursor_row  -= shift_amount;
            if (term->visible_cursor_row != -1)
                term->visible_cursor_row -= shift_amount;

            /* Redraw characters within old region */
            __guac_terminal_redraw_rect(term, height - shift_amount, 0, height-1, width-1);

        }

    }

    /* Resize display */
    guac_terminal_display_flush(term->display);
    guac_terminal_display_resize(term->display, width, height);

    /* Reraw any characters on right if widening */
    if (width > term->term_width)
        __guac_terminal_redraw_rect(term, 0, term->term_width-1, height-1, width-1);

    /* If height is increasing, shift display down */
    if (height > term->term_height) {

        /* If undisplayed rows exist in the buffer, shift them into view */
        int available_scroll = guac_terminal_available_scroll(term);
        if (available_scroll > 0) {

            /* If the new terminal bottom reveals N rows, shift down N rows */
            int shift_amount = height - term->term_height;

            /* The maximum amount we can shift is the number of undisplayed rows */
            if (shift_amount > available_scroll)
                shift_amount = available_scroll;

            /* Update buffer top and cursor row based on shift */
            term->buffer->top -= shift_amount;
            term->cursor_row  += shift_amount;
            if (term->visible_cursor_row != -1)
                term->visible_cursor_row += shift_amount;

            /* If scrolled enough, use scroll to fulfill entire resize */
            if (term->scroll_offset >= shift_amount) {

                term->scroll_offset -= shift_amount;
                guac_terminal_scrollbar_set_value(term->scrollbar, -term->scroll_offset);

                /* Draw characters from scroll at bottom */
                __guac_terminal_redraw_rect(term, term->term_height, 0, term->term_height + shift_amount - 1, width-1);

            }

            /* Otherwise, fulfill with as much scroll as possible */
            else {

                /* Draw characters from scroll at bottom */
                __guac_terminal_redraw_rect(term, term->term_height, 0, term->term_height + term->scroll_offset - 1, width-1);

                /* Update shift_amount and scroll based on new rows */
                shift_amount -= term->scroll_offset;
                term->scroll_offset = 0;
                guac_terminal_scrollbar_set_value(term->scrollbar, -term->scroll_offset);

                /* If anything remains, move screen as necessary */
                if (shift_amount > 0) {

                    guac_terminal_display_copy_rows(term->display,
                            0, term->display->height - shift_amount - 1, shift_amount);

                    /* Draw characters at top from scroll */
                    __guac_terminal_redraw_rect(term, 0, 0, shift_amount - 1, width-1);

                }

            }

        } /* end if undisplayed rows exist */

    }

    /* Keep cursor on screen */
    if (term->cursor_row < 0)       term->cursor_row = 0;
    if (term->cursor_row >= height) term->cursor_row = height-1;
    if (term->cursor_col < 0)       term->cursor_col = 0;
    if (term->cursor_col >= width)  term->cursor_col = width-1;

    /* Commit new dimensions */
    term->term_width = width;
    term->term_height = height;

}

int guac_terminal_resize(guac_terminal* terminal, int width, int height) {

    guac_terminal_display* display = terminal->display;
    guac_client* client = display->client;

    /* Acquire exclusive access to terminal */
    guac_terminal_lock(terminal);

    /* Set size of available screen area */
    terminal->outer_width = width;
    terminal->outer_height = height;

    /* Calculate available display area */
    int available_width = width - GUAC_TERMINAL_SCROLLBAR_WIDTH;
    if (available_width < 0)
        available_width = 0;

    /* Calculate dimensions */
    int rows    = height / display->char_height;
    int columns = available_width / display->char_width;

    /* Keep height within predefined maximum */
    if (rows > GUAC_TERMINAL_MAX_ROWS) {
        rows = GUAC_TERMINAL_MAX_ROWS;
        height = rows * display->char_height;
    }

    /* Keep width within predefined maximum */
    if (columns > GUAC_TERMINAL_MAX_COLUMNS) {
        columns = GUAC_TERMINAL_MAX_COLUMNS;
        available_width = columns * display->char_width;
        width = available_width + GUAC_TERMINAL_SCROLLBAR_WIDTH;
    }

    /* Set pixel sizes */
    terminal->width = width;
    terminal->height = height;

    /* Resize default layer to given pixel dimensions */
    guac_terminal_repaint_default_layer(terminal, client->socket);

    /* Resize terminal if row/column dimensions have changed */
    if (columns != terminal->term_width || rows != terminal->term_height) {

        guac_client_log(client, GUAC_LOG_DEBUG,
                "Resizing terminal to %ix%i", rows, columns);

        /* Resize terminal */
        __guac_terminal_resize(terminal, columns, rows);

        /* Reset scroll region */
        terminal->scroll_end = rows - 1;

    }

    /* Notify scrollbar of resize */
    guac_terminal_scrollbar_parent_resized(terminal->scrollbar, width, height, rows);
    guac_terminal_scrollbar_set_bounds(terminal->scrollbar,
            -guac_terminal_available_scroll(terminal), 0);


    /* Release terminal */
    guac_terminal_unlock(terminal);

    guac_terminal_notify(terminal);
    return 0;

}

void guac_terminal_flush(guac_terminal* terminal) {

    /* Flush typescript if in use */
    if (terminal->typescript != NULL)
        guac_terminal_typescript_flush(terminal->typescript);

    /* Flush pipe stream if automatic flushing is enabled */
    if (terminal->pipe_stream_flags & GUAC_TERMINAL_PIPE_AUTOFLUSH)
        guac_terminal_pipe_stream_flush(terminal);

    /* Flush display state */
    guac_terminal_select_redraw(terminal);
    guac_terminal_commit_cursor(terminal);
    guac_terminal_display_flush(terminal->display);
    guac_terminal_scrollbar_flush(terminal->scrollbar);

}

void guac_terminal_lock(guac_terminal* terminal) {
    pthread_mutex_lock(&(terminal->lock));
}

void guac_terminal_unlock(guac_terminal* terminal) {
    pthread_mutex_unlock(&(terminal->lock));
}

int guac_terminal_send_data(guac_terminal* term, const char* data, int length) {

    /* Block all other sources of input if input is coming from a stream */
    if (term->input_stream != NULL)
        return 0;

    return guac_terminal_write_all(term->stdin_pipe_fd[1], data, length);

}

int guac_terminal_send_string(guac_terminal* term, const char* data) {

    /* Block all other sources of input if input is coming from a stream */
    if (term->input_stream != NULL)
        return 0;

    return guac_terminal_write_all(term->stdin_pipe_fd[1], data, strlen(data));

}

static int __guac_terminal_send_key(guac_terminal* term, int keysym, int pressed) {

    /* Ignore user input if terminal is not started */
    if (!term->started) {
        guac_client_log(term->client, GUAC_LOG_DEBUG, "Ignoring user input "
                "while terminal has not yet started.");
        return 0;
    }

    /* Hide mouse cursor if not already hidden */
    if (term->current_cursor != GUAC_TERMINAL_CURSOR_BLANK) {
        term->current_cursor = GUAC_TERMINAL_CURSOR_BLANK;
        guac_common_cursor_set_blank(term->cursor);
        guac_terminal_notify(term);
    }

    /* Track modifiers */
    if (keysym == 0xFFE3)
        term->mod_ctrl = pressed;
    else if (keysym == 0xFFE9)
        term->mod_alt = pressed;
    else if (keysym == 0xFFE1)
        term->mod_shift = pressed;
        
    /* If key pressed */
    else if (pressed) {

        /* Ctrl+Shift+V shortcut for paste */
        if (keysym == 'V' && term->mod_ctrl)
            return guac_terminal_send_data(term, term->clipboard->buffer, term->clipboard->length);

        /* Shift+PgUp / Shift+PgDown shortcuts for scrolling */
        if (term->mod_shift) {

            /* Page up */
            if (keysym == 0xFF55) {
                guac_terminal_scroll_display_up(term, term->term_height);
                return 0;
            }

            /* Page down */
            if (keysym == 0xFF56) {
                guac_terminal_scroll_display_down(term, term->term_height);
                return 0;
            }

        }

        /* Reset scroll */
        if (term->scroll_offset != 0)
            guac_terminal_scroll_display_down(term, term->scroll_offset);

        /* If alt being held, also send escape character */
        if (term->mod_alt)
            guac_terminal_send_string(term, "\x1B");

        /* Translate Ctrl+letter to control code */ 
        if (term->mod_ctrl) {

            char data;

            /* Keysyms for '@' through '_' are all conveniently in C0 order */
            if (keysym >= '@' && keysym <= '_')
                data = (char) (keysym - '@');

            /* Handle lowercase as well */
            else if (keysym >= 'a' && keysym <= 'z')
                data = (char) (keysym - 'a' + 1);

            /* Ctrl+? is DEL (0x7f) */
            else if (keysym == '?')
                data = 0x7F;

            /* Map Ctrl+2 to same result as Ctrl+@ */
            else if (keysym == '2')
                data = 0x00;

            /* Map Ctrl+3 through Ctrl-7 to the remaining C0 characters such that Ctrl+6 is the same as Ctrl+^ */
            else if (keysym >= '3' && keysym <= '7')
                data = (char) (keysym - '3' + 0x1B);

            /* Otherwise ignore */
            else
                return 0;

            return guac_terminal_send_data(term, &data, 1);

        }

        /* Translate Unicode to UTF-8 */
        else if ((keysym >= 0x00 && keysym <= 0xFF) || ((keysym & 0xFFFF0000) == 0x01000000)) {

            int length;
            char data[5];

            length = guac_terminal_encode_utf8(keysym & 0xFFFF, data);
            return guac_terminal_send_data(term, data, length);

        }

        /* Typeable keys of number pad */
        else if (keysym >= 0xFFAA && keysym <= 0xFFB9) {
            char value = keysym - 0xFF80;
            guac_terminal_send_data(term, &value, sizeof(value));
        }

        /* Non-printable keys */
        else {

            /* Backspace can vary based on configuration of terminal by client. */
            if (keysym == 0xFF08) {
                char backspace_str[] = { term->backspace, '\0' };
                return guac_terminal_send_string(term, backspace_str);
            }
            if (keysym == 0xFF09 || keysym == 0xFF89) return guac_terminal_send_string(term, "\x09"); /* Tab */
            if (keysym == 0xFF0D || keysym == 0xFF8D) return guac_terminal_send_string(term, "\x0D"); /* Enter */
            if (keysym == 0xFF1B) return guac_terminal_send_string(term, "\x1B"); /* Esc */

            if (keysym == 0xFF50 || keysym == 0xFF95) return guac_terminal_send_string(term, "\x1B[1~"); /* Home */

            /* Arrow keys w/ application cursor */
            if (term->application_cursor_keys) {
                if (keysym == 0xFF51 || keysym == 0xFF96) return guac_terminal_send_string(term, "\x1BOD"); /* Left */
                if (keysym == 0xFF52 || keysym == 0xFF97) return guac_terminal_send_string(term, "\x1BOA"); /* Up */
                if (keysym == 0xFF53 || keysym == 0xFF98) return guac_terminal_send_string(term, "\x1BOC"); /* Right */
                if (keysym == 0xFF54 || keysym == 0xFF99) return guac_terminal_send_string(term, "\x1BOB"); /* Down */
            }
            else {
                if (keysym == 0xFF51 || keysym == 0xFF96) return guac_terminal_send_string(term, "\x1B[D"); /* Left */
                if (keysym == 0xFF52 || keysym == 0xFF97) return guac_terminal_send_string(term, "\x1B[A"); /* Up */
                if (keysym == 0xFF53 || keysym == 0xFF98) return guac_terminal_send_string(term, "\x1B[C"); /* Right */
                if (keysym == 0xFF54 || keysym == 0xFF99) return guac_terminal_send_string(term, "\x1B[B"); /* Down */
            }

            if (keysym == 0xFF55 || keysym == 0xFF9A) return guac_terminal_send_string(term, "\x1B[5~"); /* Page up */
            if (keysym == 0xFF56 || keysym == 0xFF9B) return guac_terminal_send_string(term, "\x1B[6~"); /* Page down */
            if (keysym == 0xFF57 || keysym == 0xFF9C) return guac_terminal_send_string(term, "\x1B[4~"); /* End */

            if (keysym == 0xFF63 || keysym == 0xFF9E) return guac_terminal_send_string(term, "\x1B[2~"); /* Insert */

            if (keysym == 0xFFBE || keysym == 0xFF91) return guac_terminal_send_string(term, "\x1B[[A"); /* F1  */
            if (keysym == 0xFFBF || keysym == 0xFF92) return guac_terminal_send_string(term, "\x1B[[B"); /* F2  */
            if (keysym == 0xFFC0 || keysym == 0xFF93) return guac_terminal_send_string(term, "\x1B[[C"); /* F3  */
            if (keysym == 0xFFC1 || keysym == 0xFF94) return guac_terminal_send_string(term, "\x1B[[D"); /* F4  */
            if (keysym == 0xFFC2) return guac_terminal_send_string(term, "\x1B[[E"); /* F5  */

            if (keysym == 0xFFC3) return guac_terminal_send_string(term, "\x1B[17~"); /* F6  */
            if (keysym == 0xFFC4) return guac_terminal_send_string(term, "\x1B[18~"); /* F7  */
            if (keysym == 0xFFC5) return guac_terminal_send_string(term, "\x1B[19~"); /* F8  */
            if (keysym == 0xFFC6) return guac_terminal_send_string(term, "\x1B[20~"); /* F9  */
            if (keysym == 0xFFC7) return guac_terminal_send_string(term, "\x1B[21~"); /* F10 */
            if (keysym == 0xFFC8) return guac_terminal_send_string(term, "\x1B[22~"); /* F11 */
            if (keysym == 0xFFC9) return guac_terminal_send_string(term, "\x1B[23~"); /* F12 */

            if (keysym == 0xFFFF || keysym == 0xFF9F) return guac_terminal_send_string(term, "\x1B[3~"); /* Delete */

            /* Ignore unknown keys */
            guac_client_log(term->client, GUAC_LOG_DEBUG,
                    "Ignoring unknown keysym: 0x%X", keysym);
        }

    }

    return 0;

}

int guac_terminal_send_key(guac_terminal* term, int keysym, int pressed) {

    int result;

    guac_terminal_lock(term);
    result = __guac_terminal_send_key(term, keysym, pressed);
    guac_terminal_unlock(term);

    return result;

}

static int __guac_terminal_send_mouse(guac_terminal* term, guac_user* user,
        int x, int y, int mask) {

    /* Ignore user input if terminal is not started */
    if (!term->started) {
        guac_client_log(term->client, GUAC_LOG_DEBUG, "Ignoring user input "
                "while terminal has not yet started.");
        return 0;
    }

    /* Determine which buttons were just released and pressed */
    int released_mask =  term->mouse_mask & ~mask;
    int pressed_mask  = ~term->mouse_mask &  mask;

    /* Store current mouse location/state */
    guac_common_cursor_update(term->cursor, user, x, y, mask);

    /* Notify scrollbar, do not handle anything handled by scrollbar */
    if (guac_terminal_scrollbar_handle_mouse(term->scrollbar, x, y, mask)) {

        /* Set pointer cursor if mouse is over scrollbar */
        if (term->current_cursor != GUAC_TERMINAL_CURSOR_POINTER) {
            term->current_cursor = GUAC_TERMINAL_CURSOR_POINTER;
            guac_common_cursor_set_pointer(term->cursor);
            guac_terminal_notify(term);
        }

        guac_terminal_notify(term);
        return 0;

    }

    term->mouse_mask = mask;

    /* Show mouse cursor if not already shown */
    if (term->current_cursor != GUAC_TERMINAL_CURSOR_IBAR) {
        term->current_cursor = GUAC_TERMINAL_CURSOR_IBAR;
        guac_common_cursor_set_ibar(term->cursor);
        guac_terminal_notify(term);
    }

    /* Paste contents of clipboard on right or middle mouse button up */
    if ((released_mask & GUAC_CLIENT_MOUSE_RIGHT) || (released_mask & GUAC_CLIENT_MOUSE_MIDDLE))
        return guac_terminal_send_data(term, term->clipboard->buffer, term->clipboard->length);

    /* If left mouse button was just released, stop selection */
    if (released_mask & GUAC_CLIENT_MOUSE_LEFT)
        guac_terminal_select_end(term);

    /* Update selection state contextually while the left mouse button is
     * pressed */
    else if (mask & GUAC_CLIENT_MOUSE_LEFT) {

        int row = y / term->display->char_height - term->scroll_offset;
        int col = x / term->display->char_width;

        /* If mouse button was already just pressed, start a new selection or
         * resume the existing selection depending on whether shift is held */
        if (pressed_mask & GUAC_CLIENT_MOUSE_LEFT) {
            if (term->mod_shift)
                guac_terminal_select_resume(term, row, col);
            else
                guac_terminal_select_start(term, row, col);
        }

        /* In all other cases, simply update the existing selection as long as
         * the mouse button is pressed */
        else
            guac_terminal_select_update(term, row, col);

    }

    /* Scroll up if wheel moved up */
    if (released_mask & GUAC_CLIENT_MOUSE_SCROLL_UP)
        guac_terminal_scroll_display_up(term, GUAC_TERMINAL_WHEEL_SCROLL_AMOUNT);

    /* Scroll down if wheel moved down */
    if (released_mask & GUAC_CLIENT_MOUSE_SCROLL_DOWN)
        guac_terminal_scroll_display_down(term, GUAC_TERMINAL_WHEEL_SCROLL_AMOUNT);

    return 0;

}

int guac_terminal_send_mouse(guac_terminal* term, guac_user* user,
        int x, int y, int mask) {

    int result;

    guac_terminal_lock(term);
    result = __guac_terminal_send_mouse(term, user, x, y, mask);
    guac_terminal_unlock(term);

    return result;

}

void guac_terminal_scroll_handler(guac_terminal_scrollbar* scrollbar, int value) {

    guac_terminal* terminal = (guac_terminal*) scrollbar->data;

    /* Calculate change in scroll offset */
    int delta = -value - terminal->scroll_offset;

    /* Update terminal based on change in scroll offset */
    if (delta < 0)
        guac_terminal_scroll_display_down(terminal, -delta);
    else if (delta > 0)
        guac_terminal_scroll_display_up(terminal, delta);

    /* Update scrollbar value */
    guac_terminal_scrollbar_set_value(scrollbar, value);

}

int guac_terminal_sendf(guac_terminal* term, const char* format, ...) {

    int written;

    va_list ap;
    char buffer[1024];

    /* Block all other sources of input if input is coming from a stream */
    if (term->input_stream != NULL)
        return 0;

    /* Print to buffer */
    va_start(ap, format);
    written = vsnprintf(buffer, sizeof(buffer)-1, format, ap);
    va_end(ap);

    if (written < 0)
        return written;

    /* Write to STDIN */
    return guac_terminal_write_all(term->stdin_pipe_fd[1], buffer, written);

}

void guac_terminal_set_tab(guac_terminal* term, int column) {

    int i;

    /* Search for available space, set if available */
    for (i=0; i<GUAC_TERMINAL_MAX_TABS; i++) {

        /* Set tab if space free */
        if (term->custom_tabs[i] == 0) {
            term->custom_tabs[i] = column+1;
            break;
        }

    }

}

void guac_terminal_unset_tab(guac_terminal* term, int column) {

    int i;

    /* Search for given tab, unset if found */
    for (i=0; i<GUAC_TERMINAL_MAX_TABS; i++) {

        /* Unset tab if found */
        if (term->custom_tabs[i] == column+1) {
            term->custom_tabs[i] = 0;
            break;
        }

    }

}

void guac_terminal_clear_tabs(guac_terminal* term) {
    term->tab_interval = 0;
    memset(term->custom_tabs, 0, sizeof(term->custom_tabs));
}

int guac_terminal_next_tab(guac_terminal* term, int column) {

    int i;

    /* Determine tab stop from interval */
    int tabstop;
    if (term->tab_interval != 0)
        tabstop = (column / term->tab_interval + 1) * term->tab_interval;
    else
        tabstop = term->term_width - 1;

    /* Walk custom tabs, trying to find an earlier occurrence */
    for (i=0; i<GUAC_TERMINAL_MAX_TABS; i++) {

        int custom_tabstop = term->custom_tabs[i] - 1;
        if (custom_tabstop != -1 && custom_tabstop > column && custom_tabstop < tabstop)
            tabstop = custom_tabstop;

    }

    return tabstop;
}

void guac_terminal_pipe_stream_open(guac_terminal* term, const char* name,
        int flags) {

    guac_client* client = term->client;
    guac_socket* socket = client->socket;

    /* Close existing stream, if any */
    guac_terminal_pipe_stream_close(term);

    /* Allocate and assign new pipe stream */
    term->pipe_stream = guac_client_alloc_stream(client);
    term->pipe_buffer_length = 0;
    term->pipe_stream_flags = flags;

    /* Open new pipe stream */
    guac_protocol_send_pipe(socket, term->pipe_stream, "text/plain", name);

    /* Log redirect at debug level */
    guac_client_log(client, GUAC_LOG_DEBUG, "Terminal output now directed to "
            "pipe \"%s\" (flags=%i).", name, flags);

}

void guac_terminal_pipe_stream_write(guac_terminal* term, char c) {

    /* Append byte to buffer only if pipe is open */
    if (term->pipe_stream != NULL) {

        /* Flush buffer if no space is available */
        if (term->pipe_buffer_length == sizeof(term->pipe_buffer))
            guac_terminal_pipe_stream_flush(term);

        /* Append single byte to buffer */
        term->pipe_buffer[term->pipe_buffer_length++] = c;

    }

}

void guac_terminal_pipe_stream_flush(guac_terminal* term) {

    guac_client* client = term->client;
    guac_socket* socket = client->socket;
    guac_stream* pipe_stream = term->pipe_stream;

    /* Write blob if data exists in buffer */
    if (pipe_stream != NULL && term->pipe_buffer_length > 0) {
        guac_protocol_send_blob(socket, pipe_stream,
                term->pipe_buffer, term->pipe_buffer_length);
        term->pipe_buffer_length = 0;
    }

}

void guac_terminal_pipe_stream_close(guac_terminal* term) {

    guac_client* client = term->client;
    guac_socket* socket = client->socket;
    guac_stream* pipe_stream = term->pipe_stream;

    /* Close any existing pipe */
    if (pipe_stream != NULL) {

        /* Write end of stream */
        guac_terminal_pipe_stream_flush(term);
        guac_protocol_send_end(socket, pipe_stream);

        /* Destroy stream */
        guac_client_free_stream(client, pipe_stream);
        term->pipe_stream = NULL;

        /* Log redirect at debug level */
        guac_client_log(client, GUAC_LOG_DEBUG,
                "Terminal output now redirected to display.");

    }

}

int guac_terminal_create_typescript(guac_terminal* term, const char* path,
        const char* name, int create_path) {

    /* Create typescript */
    term->typescript = guac_terminal_typescript_alloc(path, name, create_path);

    /* Log failure */
    if (term->typescript == NULL) {
        guac_client_log(term->client, GUAC_LOG_ERROR,
                "Creation of typescript failed: %s", strerror(errno));
        return 1;
    }

    /* If typescript was successfully created, log filenames */
    guac_client_log(term->client, GUAC_LOG_INFO,
            "Typescript of terminal session will be saved to \"%s\". "
            "Timing file is \"%s\".",
            term->typescript->data_filename,
            term->typescript->timing_filename);

    /* Typescript creation succeeded */
    return 0;

}

void guac_terminal_dup(guac_terminal* term, guac_user* user,
        guac_socket* socket) {

    /* Synchronize display state with new user */
    guac_terminal_repaint_default_layer(term, socket);
    guac_terminal_display_dup(term->display, user, socket);

    /* Synchronize mouse cursor */
    guac_common_cursor_dup(term->cursor, user, socket);

    /* Paint scrollbar for joining user */
    guac_terminal_scrollbar_dup(term->scrollbar, user, socket);

}

void guac_terminal_apply_color_scheme(guac_terminal* terminal,
        const char* color_scheme) {

    guac_client* client = terminal->client;
    guac_terminal_char* default_char = &terminal->default_char;
    guac_terminal_display* display = terminal->display;

    /* Reinitialize default terminal colors with values from color scheme */
    guac_terminal_parse_color_scheme(client, color_scheme,
        &default_char->attributes.foreground,
        &default_char->attributes.background,
        display->default_palette);

    /* Reinitialize default attributes of buffer and display */
    guac_terminal_display_reset_palette(display);
    display->default_foreground = default_char->attributes.foreground;
    display->default_background = default_char->attributes.background;

    /* Redraw terminal text and background */
    guac_terminal_repaint_default_layer(terminal, client->socket);
    __guac_terminal_redraw_rect(terminal, 0, 0,
            terminal->term_height - 1,
            terminal->term_width - 1);

    /* Acquire exclusive access to terminal */
    guac_terminal_lock(terminal);

    /* Update stored copy of color scheme */
    free((char*) terminal->color_scheme);
    terminal->color_scheme = strdup(color_scheme);

    /* Release terminal */
    guac_terminal_unlock(terminal);

    guac_terminal_notify(terminal);

}

void guac_terminal_apply_font(guac_terminal* terminal, const char* font_name,
        int font_size, int dpi) {

    guac_client* client = terminal->client;
    guac_terminal_display* display = terminal->display;

    if (guac_terminal_display_set_font(display, font_name, font_size, dpi))
        return;

    /* Resize terminal to fit available region, now that font metrics may be
     * different */
    guac_terminal_resize(terminal, terminal->outer_width,
            terminal->outer_height);

    /* Redraw terminal text and background */
    guac_terminal_repaint_default_layer(terminal, client->socket);
    __guac_terminal_redraw_rect(terminal, 0, 0,
            terminal->term_height - 1,
            terminal->term_width - 1);

    /* Acquire exclusive access to terminal */
    guac_terminal_lock(terminal);

    /* Update stored copy of font name, if changed */
    if (font_name != NULL)
        terminal->font_name = strdup(font_name);

    /* Update stored copy of font size, if changed */
    if (font_size != -1)
        terminal->font_size = font_size;

    /* Release terminal */
    guac_terminal_unlock(terminal);

    guac_terminal_notify(terminal);

}

