
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

#include <cairo/cairo.h>
#include <pango/pangocairo.h>

#include <guacamole/log.h>
#include <guacamole/guacio.h>
#include <guacamole/protocol.h>
#include <guacamole/client.h>

#include "ssh_terminal.h"

ssh_guac_terminal* ssh_guac_terminal_create(guac_client* client) {

    PangoFontMap* font_map;
    PangoFont* font;
    PangoFontMetrics* metrics;
    PangoContext* context;

    ssh_guac_terminal* term = malloc(sizeof(ssh_guac_terminal));
    term->client = client;

    term->cursor_row = 0;
    term->cursor_col = 0;

    term->term_width = 160;
    term->term_height = 50;
    term->term_state = SSH_TERM_STATE_ECHO;

    /* Get font */
    term->font_desc = pango_font_description_new();
    pango_font_description_set_family(term->font_desc, "monospace");
    pango_font_description_set_weight(term->font_desc, PANGO_WEIGHT_NORMAL);
    pango_font_description_set_size(term->font_desc, 8*PANGO_SCALE);

    font_map = pango_cairo_font_map_get_default();
    context = pango_font_map_create_context(font_map);

    font = pango_font_map_load_font(font_map, context, term->font_desc);
    if (font == NULL) {
        guac_log_error("Unable to get font.");
        return NULL;
    }

    metrics = pango_font_get_metrics(font, NULL);
    if (metrics == NULL) {
        guac_log_error("Unable to get font metrics.");
        return NULL;
    }

    /* Calculate character dimensions */
    term->char_width =
        pango_font_metrics_get_approximate_digit_width(metrics) / PANGO_SCALE;
    term->char_height =
        (pango_font_metrics_get_descent(metrics)
            + pango_font_metrics_get_ascent(metrics)) / PANGO_SCALE;

    return term;

}

void ssh_guac_terminal_free(ssh_guac_terminal* term) {
    /* STUB */
}

guac_layer* __ssh_guac_terminal_get_glyph(ssh_guac_terminal* term, char c) {

    GUACIO* io = term->client->io;
    guac_layer* glyph;

    cairo_surface_t* surface;
    cairo_t* cairo;
   
    PangoLayout* layout;

    /* Return glyph if exists */
    if (term->glyphs[(int) c])
        return term->glyphs[(int) c];

    /* Otherwise, draw glyph */
    surface = cairo_image_surface_create(
            CAIRO_FORMAT_ARGB32,
            term->char_width, term->char_height);
    cairo = cairo_create(surface);

    /* Get layout */
    layout = pango_cairo_create_layout(cairo);
    pango_layout_set_font_description(layout, term->font_desc);
    pango_layout_set_text(layout, &c, 1);

    /* Draw */
    cairo_set_source_rgba(cairo, 1.0, 1.0, 1.0, 1.0);
    cairo_move_to(cairo, 0.0, 0.0);
    pango_cairo_show_layout(cairo, layout);

    /* Free all */
    g_object_unref(layout);
    cairo_destroy(cairo);

    /* Send glyph and save */
    glyph = guac_client_alloc_buffer(term->client);
    guac_send_png(io, GUAC_COMP_OVER, glyph, 0, 0, surface);
    term->glyphs[(int) c] = glyph;

    guac_flush(io);
    cairo_surface_destroy(surface);

    /* Return glyph */
    return glyph;

}

int __ssh_guac_terminal_send_glyph(ssh_guac_terminal* term, int row, int col, char c) {

    GUACIO* io = term->client->io;
    guac_layer* glyph = __ssh_guac_terminal_get_glyph(term, c);

    return guac_send_copy(io,
            glyph, 0, 0, term->char_width, term->char_height,
            GUAC_COMP_SRC, GUAC_DEFAULT_LAYER,
            term->char_width * col,
            term->char_height * row);

}

int ssh_guac_terminal_write(ssh_guac_terminal* term, const char* c, int size) {

    GUACIO* io = term->client->io;

    while (size > 0) {

        switch (term->term_state) {

            case SSH_TERM_STATE_NULL:
                break;

            case SSH_TERM_STATE_ECHO:

                /* Wrap if necessary */
                if (term->cursor_col >= term->term_width) {
                    term->cursor_col = 0;
                    term->cursor_row++;
                }

                /* Scroll up if necessary */
                if (term->cursor_row >= term->term_height) {
                    term->cursor_row = term->term_height - 1;
                    
                    /* Copy screen up by one row */
                    guac_send_copy(io,
                            GUAC_DEFAULT_LAYER, 0, term->char_height,
                            term->char_width * term->term_width,
                            term->char_height * (term->term_height - 1),
                            GUAC_COMP_SRC, GUAC_DEFAULT_LAYER, 0, 0);

                    /* Fill bottom row with background */
                    guac_send_rect(io,
                            GUAC_COMP_SRC, GUAC_DEFAULT_LAYER,
                            0, term->char_height * (term->term_height - 1),
                            term->char_width * term->term_width,
                            term->char_height * term->term_height,
                            0, 0, 0, 255);

                }



                switch (*c) {

                    /* Bell */
                    case 0x07:
                        break;

                    /* Backspace */
                    case 0x08:
                        if (term->cursor_col >= 1)
                            term->cursor_col--;
                        break;

                    /* Carriage return */
                    case '\r':
                        term->cursor_col = 0;
                        break;

                    /* Line feed */
                    case '\n':
                        term->cursor_row++;
                        break;

                    /* ESC */
                    case 0x1B:
                        term->term_state = SSH_TERM_STATE_ESC;
                        break;

                    /* Displayable chars */
                    default:
                        __ssh_guac_terminal_send_glyph(term,
                                term->cursor_row,
                                term->cursor_col,
                                *c);

                        /* Advance cursor */
                        term->cursor_col++;
                }

                /* End of SSH_TERM_STATE_ECHO */
                break;

            case SSH_TERM_STATE_CHARSET:
                term->term_state = SSH_TERM_STATE_ECHO;
                break;

            case SSH_TERM_STATE_ESC:

                switch (*c) {

                    case '(':
                        term->term_state = SSH_TERM_STATE_CHARSET;
                        break;

                    case ']':
                        term->term_state = SSH_TERM_STATE_OSC;
                        term->term_seq_argc = 0;
                        term->term_seq_argv_buffer_current = 0;
                        break;

                    case '[':
                        term->term_state = SSH_TERM_STATE_CSI;
                        term->term_seq_argc = 0;
                        term->term_seq_argv_buffer_current = 0;
                        break;

                    default:
                        guac_log_info("Unhandled ESC sequence: %c", *c);
                        term->term_state = SSH_TERM_STATE_ECHO;

                }

                /* End of SSH_TERM_STATE_ESC */
                break;

            case SSH_TERM_STATE_OSC:
  
                /* TODO: Implement OSC */
                if (*c == 0x9C || *c == 0x5C || *c == 0x07) /* ECMA-48 ST (String Terminator */
                   term->term_state = SSH_TERM_STATE_ECHO; 

                /* End of SSH_TERM_STATE_OSC */
                break;

            case SSH_TERM_STATE_CSI:

                /* FIXME: "The sequence of parameters may be preceded by a single question mark. */
                if (*c == '?')
                    break; /* Ignore question marks for now... */

                /* Digits get concatenated into argv */
                if (*c >= '0' && *c <= '9') {

                    /* Concatenate digit if there is space in buffer */
                    if (term->term_seq_argv_buffer_current <
                            sizeof(term->term_seq_argv_buffer)) {

                        term->term_seq_argv_buffer[
                            term->term_seq_argv_buffer_current++
                            ] = *c;
                    }

                }

                /* Any non-digit stops the parameter, and possibly the sequence */
                else {

                    /* At most 16 parameters */
                    if (term->term_seq_argc < 16) {
                        /* Finish parameter */
                        term->term_seq_argv_buffer[term->term_seq_argv_buffer_current] = 0;
                        term->term_seq_argv[term->term_seq_argc++] =
                            atoi(term->term_seq_argv_buffer);

                        /* Prepare for next parameter */
                        term->term_seq_argv_buffer_current = 0;
                    }

                    /* Handle CSI functions */ 
                    switch (*c) {

                        /* H: Move cursor */
                        case 'H':
                            term->cursor_row = term->term_seq_argv[0] - 1;
                            term->cursor_col = term->term_seq_argv[1] - 1;
                            break;

                        /* J: Erase display */
                        case 'J':

                            /* Erase from cursor to end of display */
                            if (term->term_seq_argv[0] == 0) {

                                /* Until end of line */
                                guac_send_rect(io,
                                        GUAC_COMP_SRC, GUAC_DEFAULT_LAYER,
                                        term->cursor_col * term->char_width,
                                        term->cursor_row * term->char_height,
                                        (term->term_width - term->cursor_col) * term->char_width,
                                        term->char_height,
                                        0, 0, 0, 255); /* Background color */

                                /* Until end of display */
                                if (term->cursor_row < term->term_height - 1) {
                                    guac_send_rect(io,
                                            GUAC_COMP_SRC, GUAC_DEFAULT_LAYER,
                                            0,
                                            (term->cursor_row+1) * term->char_height,
                                            term->term_width * term->char_width,
                                            term->term_height * term->char_height,
                                            0, 0, 0, 255); /* Background color */
                                }

                            }
                            
                            /* Erase from start to cursor */
                            else if (term->term_seq_argv[0] == 1) {

                                /* Until start of line */
                                guac_send_rect(io,
                                        GUAC_COMP_SRC, GUAC_DEFAULT_LAYER,
                                        0,
                                        term->cursor_row * term->char_height,
                                        term->cursor_col * term->char_width,
                                        term->char_height,
                                        0, 0, 0, 255); /* Background color */

                                /* From start */
                                if (term->cursor_row >= 1) {
                                    guac_send_rect(io,
                                            GUAC_COMP_SRC, GUAC_DEFAULT_LAYER,
                                            0,
                                            0,
                                            term->term_width * term->char_width,
                                            (term->cursor_row-1) * term->char_height,
                                            0, 0, 0, 255); /* Background color */
                                }

                            }

                            /* Entire screen */
                            else if (term->term_seq_argv[0] == 2) {
                                guac_send_rect(io,
                                        GUAC_COMP_SRC, GUAC_DEFAULT_LAYER,
                                        0,
                                        0,
                                        term->term_width * term->char_width,
                                        term->term_height * term->char_height,
                                        0, 0, 0, 255); /* Background color */
                            }

                            break;

                        /* K: Erase line */
                        case 'K':

                            /* Erase from cursor to end of line */
                            if (term->term_seq_argv[0] == 0) {
                                guac_send_rect(io,
                                        GUAC_COMP_SRC, GUAC_DEFAULT_LAYER,
                                        term->cursor_col * term->char_width,
                                        term->cursor_row * term->char_height,
                                        (term->term_width - term->cursor_col) * term->char_width,
                                        term->char_height,
                                        0, 0, 0, 255); /* Background color */
                            }

                            /* Erase from start to cursor */
                            else if (term->term_seq_argv[0] == 1) {
                                guac_send_rect(io,
                                        GUAC_COMP_SRC, GUAC_DEFAULT_LAYER,
                                        0,
                                        term->cursor_row * term->char_height,
                                        term->cursor_col * term->char_width,
                                        term->char_height,
                                        0, 0, 0, 255); /* Background color */
                            }

                            /* Erase line */
                            else if (term->term_seq_argv[0] == 2) {
                                guac_send_rect(io,
                                        GUAC_COMP_SRC, GUAC_DEFAULT_LAYER,
                                        0,
                                        term->cursor_row * term->char_height,
                                        term->term_width * term->char_width,
                                        term->char_height,
                                        0, 0, 0, 255); /* Background color */
                            }

                            break;

                        /* Warn of unhandled codes */
                        default:
                            if (*c != ';')
                                guac_log_info("Unhandled CSI sequence: %c", *c);

                    }

                    /* If not a semicolon, end of CSI sequence */
                    if (*c != ';')
                        term->term_state = SSH_TERM_STATE_ECHO;

                }

                /* End of SSH_TERM_STATE_CSI */
                break;

        }

        c++;
        size--;
    }

    return 0;

}

