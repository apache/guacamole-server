
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

#include <libssh/libssh.h>

#include <cairo/cairo.h>
#include <pango/pangocairo.h>

#include <guacamole/log.h>
#include <guacamole/guacio.h>
#include <guacamole/protocol.h>
#include <guacamole/client.h>

#define SSH_TERM_STATE_NULL 0
#define SSH_TERM_STATE_ECHO 1
#define SSH_TERM_STATE_ESC  2
#define SSH_TERM_STATE_CSI  3
#define SSH_TERM_STATE_OSC  4
#define SSH_TERM_STATE_CHARSET 5

/* Client plugin arguments */
const char* GUAC_CLIENT_ARGS[] = {
    "hostname",
    "user",
    "password",
    NULL
};

typedef struct ssh_guac_client_data {

    ssh_session session;
    ssh_channel term_channel;

    PangoFontDescription* font_desc;

    guac_layer* glyphs[256];

    int char_width;
    int char_height;

    int term_width;
    int term_height;
    int term_state;

    int term_seq_argc;
    int term_seq_argv[16];
    char term_seq_argv_buffer[16];
    int term_seq_argv_buffer_current;

    int cursor_row;
    int cursor_col;

} ssh_guac_client_data;

int ssh_guac_client_handle_messages(guac_client* client);
int ssh_guac_client_key_handler(guac_client* client, int keysym, int pressed);
int ssh_guac_client_send_glyph(guac_client* client, int row, int col, char c);
int ssh_guac_client_write(guac_client* client, const char* c, int size);

int guac_client_init(guac_client* client, int argc, char** argv) {

    GUACIO* io = client->io;

    PangoFontMap* font_map;
    PangoFont* font;
    PangoFontMetrics* metrics;
    PangoContext* context;

    ssh_guac_client_data* client_data = malloc(sizeof(ssh_guac_client_data));

    client_data->cursor_row = 0;
    client_data->cursor_col = 0;

    client_data->term_width = 160;
    client_data->term_height = 50;
    client_data->term_state = SSH_TERM_STATE_ECHO;

    /* Get font */
    client_data->font_desc = pango_font_description_new();
    pango_font_description_set_family(client_data->font_desc, "monospace");
    pango_font_description_set_weight(client_data->font_desc, PANGO_WEIGHT_NORMAL);
    pango_font_description_set_size(client_data->font_desc, 8*PANGO_SCALE);

    font_map = pango_cairo_font_map_get_default();
    context = pango_font_map_create_context(font_map);

    font = pango_font_map_load_font(font_map, context, client_data->font_desc);
    if (font == NULL) {
        guac_log_error("Unable to get font.");
        return 1;
    }

    metrics = pango_font_get_metrics(font, NULL);
    if (metrics == NULL) {
        guac_log_error("Unable to get font metrics.");
        return 1;
    }

    /* Calculate character dimensions */
    client_data->char_width =
        pango_font_metrics_get_approximate_digit_width(metrics) / PANGO_SCALE;
    client_data->char_height =
        (pango_font_metrics_get_descent(metrics)
            + pango_font_metrics_get_ascent(metrics)) / PANGO_SCALE;

    client->data = client_data;

    /* Send name and dimensions */
    guac_send_name(io, "SSH TEST");
    guac_send_size(io,
            client_data->char_width  * client_data->term_width,
            client_data->char_height * client_data->term_height);

    guac_flush(io);

    /* Open SSH session */
    client_data->session = ssh_new();
    if (client_data->session == NULL) {
        guac_send_error(io, "Unable to create SSH session.");
        guac_flush(io);
        return 1;
    }

    /* Set session options */
    ssh_options_set(client_data->session, SSH_OPTIONS_HOST, argv[0]);
    ssh_options_set(client_data->session, SSH_OPTIONS_USER, argv[1]);

    /* Connect */
    if (ssh_connect(client_data->session) != SSH_OK) {
        guac_send_error(io, "Unable to connect via SSH.");
        guac_flush(io);
        return 1;
    }

    /* Authenticate */
    if (ssh_userauth_password(client_data->session, NULL, argv[2]) != SSH_AUTH_SUCCESS) {
        guac_send_error(io, "SSH auth failed.");
        guac_flush(io);
        return 1;
    }

    /* Open channel for terminal */
    client_data->term_channel = channel_new(client_data->session);
    if (client_data->term_channel == NULL) {
        guac_send_error(io, "Unable to open channel.");
        guac_flush(io);
        return 1;
    }

    /* Open session for channel */
    if (channel_open_session(client_data->term_channel) != SSH_OK) {
        guac_send_error(io, "Unable to open channel session.");
        guac_flush(io);
        return 1;
    }

    /* Request PTY */
    if (channel_request_pty(client_data->term_channel) != SSH_OK) {
        guac_send_error(io, "Unable to allocate PTY for channel.");
        guac_flush(io);
        return 1;
    }

    /* Request PTY size */
    if (channel_change_pty_size(client_data->term_channel, client_data->term_width, client_data->term_height) != SSH_OK) {
        guac_send_error(io, "Unable to change PTY size.");
        guac_flush(io);
        return 1;
    }

    /* Request shell */
    if (channel_request_shell(client_data->term_channel) != SSH_OK) {
        guac_send_error(io, "Unable to associate shell with PTY.");
        guac_flush(io);
        return 1;
    }

    guac_log_info("SSH connection successful.");

    /* Set handlers */
    client->handle_messages = ssh_guac_client_handle_messages;
    client->key_handler = ssh_guac_client_key_handler;

    /* Success */
    return 0;

}

guac_layer* ssh_guac_client_get_glyph(guac_client* client, char c) {

    GUACIO* io = client->io;
    ssh_guac_client_data* client_data = (ssh_guac_client_data*) client->data;
    guac_layer* glyph;

    cairo_surface_t* surface;
    cairo_t* cairo;
   
    PangoLayout* layout;

    /* Return glyph if exists */
    if (client_data->glyphs[(int) c])
        return client_data->glyphs[(int) c];

    /* Otherwise, draw glyph */
    surface = cairo_image_surface_create(
            CAIRO_FORMAT_ARGB32,
            client_data->char_width, client_data->char_height);
    cairo = cairo_create(surface);

    /* Get layout */
    layout = pango_cairo_create_layout(cairo);
    pango_layout_set_font_description(layout, client_data->font_desc);
    pango_layout_set_text(layout, &c, 1);

    /* Draw */
    cairo_set_source_rgba(cairo, 1.0, 1.0, 1.0, 1.0);
    cairo_move_to(cairo, 0.0, 0.0);
    pango_cairo_show_layout(cairo, layout);

    /* Free all */
    g_object_unref(layout);
    cairo_destroy(cairo);

    /* Send glyph and save */
    glyph = guac_client_alloc_buffer(client);
    guac_send_png(io, GUAC_COMP_OVER, glyph, 0, 0, surface);
    client_data->glyphs[(int) c] = glyph;

    guac_flush(io);
    cairo_surface_destroy(surface);

    /* Return glyph */
    return glyph;

}

int ssh_guac_client_handle_messages(guac_client* client) {

    GUACIO* io = client->io;
    ssh_guac_client_data* client_data = (ssh_guac_client_data*) client->data;
    char buffer[8192];

    /* While data available, write to terminal */
    int bytes_read = 0;
    while (channel_is_open(client_data->term_channel)
            && !channel_is_eof(client_data->term_channel)
            && (bytes_read = channel_read_nonblocking(client_data->term_channel, buffer, sizeof(buffer), 0)) > 0) {

        ssh_guac_client_write(client, buffer, bytes_read);
        guac_flush(io);

    }

    /* Notify on error */
    if (bytes_read < 0) {
        guac_send_error(io, "Error reading data.");
        guac_flush(io);
        return 1;
    }

    return 0;

}

int ssh_guac_client_send_glyph(guac_client* client, int row, int col, char c) {

    GUACIO* io = client->io;
    ssh_guac_client_data* client_data = (ssh_guac_client_data*) client->data;
    guac_layer* glyph = ssh_guac_client_get_glyph(client, c);

    return guac_send_copy(io,
            glyph, 0, 0, client_data->char_width, client_data->char_height,
            GUAC_COMP_SRC, GUAC_DEFAULT_LAYER,
            client_data->char_width * col,
            client_data->char_height * row);

}

int ssh_guac_client_write(guac_client* client, const char* c, int size) {

    GUACIO* io = client->io;
    ssh_guac_client_data* client_data = (ssh_guac_client_data*) client->data;

    while (size > 0) {

        switch (client_data->term_state) {

            case SSH_TERM_STATE_NULL:
                break;

            case SSH_TERM_STATE_ECHO:

                /* Wrap if necessary */
                if (client_data->cursor_col >= client_data->term_width) {
                    client_data->cursor_col = 0;
                    client_data->cursor_row++;
                }

                /* Scroll up if necessary */
                if (client_data->cursor_row >= client_data->term_height) {
                    client_data->cursor_row = client_data->term_height - 1;
                    
                    /* Copy screen up by one row */
                    guac_send_copy(io,
                            GUAC_DEFAULT_LAYER, 0, client_data->char_height,
                            client_data->char_width * client_data->term_width,
                            client_data->char_height * (client_data->term_height - 1),
                            GUAC_COMP_SRC, GUAC_DEFAULT_LAYER, 0, 0);

                    /* Fill bottom row with background */
                    guac_send_rect(io,
                            GUAC_COMP_SRC, GUAC_DEFAULT_LAYER,
                            0, client_data->char_height * (client_data->term_height - 1),
                            client_data->char_width * client_data->term_width,
                            client_data->char_height * client_data->term_height,
                            0, 0, 0, 255);

                }



                switch (*c) {

                    /* Bell */
                    case 0x07:
                        break;

                    /* Backspace */
                    case 0x08:
                        if (client_data->cursor_col >= 1)
                            client_data->cursor_col--;
                        break;

                    /* Carriage return */
                    case '\r':
                        client_data->cursor_col = 0;
                        break;

                    /* Line feed */
                    case '\n':
                        client_data->cursor_row++;
                        break;

                    /* ESC */
                    case 0x1B:
                        client_data->term_state = SSH_TERM_STATE_ESC;
                        break;

                    /* Displayable chars */
                    default:
                        ssh_guac_client_send_glyph(client,
                                client_data->cursor_row,
                                client_data->cursor_col,
                                *c);

                        /* Advance cursor */
                        client_data->cursor_col++;
                }

                /* End of SSH_TERM_STATE_ECHO */
                break;

            case SSH_TERM_STATE_CHARSET:
                client_data->term_state = SSH_TERM_STATE_ECHO;
                break;

            case SSH_TERM_STATE_ESC:

                switch (*c) {

                    case '(':
                        client_data->term_state = SSH_TERM_STATE_CHARSET;
                        break;

                    case ']':
                        client_data->term_state = SSH_TERM_STATE_OSC;
                        client_data->term_seq_argc = 0;
                        client_data->term_seq_argv_buffer_current = 0;
                        break;

                    case '[':
                        client_data->term_state = SSH_TERM_STATE_CSI;
                        client_data->term_seq_argc = 0;
                        client_data->term_seq_argv_buffer_current = 0;
                        break;

                    default:
                        guac_log_info("Unhandled ESC sequence: %c", *c);
                        client_data->term_state = SSH_TERM_STATE_ECHO;

                }

                /* End of SSH_TERM_STATE_ESC */
                break;

            case SSH_TERM_STATE_OSC:
  
                /* TODO: Implement OSC */
                if (*c == 0x9C || *c == 0x5C || *c == 0x07) /* ECMA-48 ST (String Terminator */
                   client_data->term_state = SSH_TERM_STATE_ECHO; 

                /* End of SSH_TERM_STATE_OSC */
                break;

            case SSH_TERM_STATE_CSI:

                /* FIXME: "The sequence of parameters may be preceded by a single question mark. */
                if (*c == '?')
                    break; /* Ignore question marks for now... */

                /* Digits get concatenated into argv */
                if (*c >= '0' && *c <= '9') {

                    /* Concatenate digit if there is space in buffer */
                    if (client_data->term_seq_argv_buffer_current <
                            sizeof(client_data->term_seq_argv_buffer)) {

                        client_data->term_seq_argv_buffer[
                            client_data->term_seq_argv_buffer_current++
                            ] = *c;
                    }

                }

                /* Any non-digit stops the parameter, and possibly the sequence */
                else {

                    /* At most 16 parameters */
                    if (client_data->term_seq_argc < 16) {
                        /* Finish parameter */
                        client_data->term_seq_argv_buffer[client_data->term_seq_argv_buffer_current] = 0;
                        client_data->term_seq_argv[client_data->term_seq_argc++] =
                            atoi(client_data->term_seq_argv_buffer);

                        /* Prepare for next parameter */
                        client_data->term_seq_argv_buffer_current = 0;
                    }

                    /* Handle CSI functions */ 
                    switch (*c) {

                        /* H: Move cursor */
                        case 'H':
                            client_data->cursor_row = client_data->term_seq_argv[0] - 1;
                            client_data->cursor_col = client_data->term_seq_argv[1] - 1;
                            break;

                        /* J: Erase display */
                        case 'J':

                            /* Erase from cursor to end of display */
                            if (client_data->term_seq_argv[0] == 0) {

                                /* Until end of line */
                                guac_send_rect(io,
                                        GUAC_COMP_SRC, GUAC_DEFAULT_LAYER,
                                        client_data->cursor_col * client_data->char_width,
                                        client_data->cursor_row * client_data->char_height,
                                        (client_data->term_width - client_data->cursor_col) * client_data->char_width,
                                        client_data->char_height,
                                        0, 0, 0, 255); /* Background color */

                                /* Until end of display */
                                if (client_data->cursor_row < client_data->term_height - 1) {
                                    guac_send_rect(io,
                                            GUAC_COMP_SRC, GUAC_DEFAULT_LAYER,
                                            0,
                                            (client_data->cursor_row+1) * client_data->char_height,
                                            client_data->term_width * client_data->char_width,
                                            client_data->term_height * client_data->char_height,
                                            0, 0, 0, 255); /* Background color */
                                }

                            }
                            
                            /* Erase from start to cursor */
                            else if (client_data->term_seq_argv[0] == 1) {

                                /* Until start of line */
                                guac_send_rect(io,
                                        GUAC_COMP_SRC, GUAC_DEFAULT_LAYER,
                                        0,
                                        client_data->cursor_row * client_data->char_height,
                                        client_data->cursor_col * client_data->char_width,
                                        client_data->char_height,
                                        0, 0, 0, 255); /* Background color */

                                /* From start */
                                if (client_data->cursor_row >= 1) {
                                    guac_send_rect(io,
                                            GUAC_COMP_SRC, GUAC_DEFAULT_LAYER,
                                            0,
                                            0,
                                            client_data->term_width * client_data->char_width,
                                            (client_data->cursor_row-1) * client_data->char_height,
                                            0, 0, 0, 255); /* Background color */
                                }

                            }

                            /* Entire screen */
                            else if (client_data->term_seq_argv[0] == 2) {
                                guac_send_rect(io,
                                        GUAC_COMP_SRC, GUAC_DEFAULT_LAYER,
                                        0,
                                        0,
                                        client_data->term_width * client_data->char_width,
                                        client_data->term_height * client_data->char_height,
                                        0, 0, 0, 255); /* Background color */
                            }

                            break;

                        /* K: Erase line */
                        case 'K':

                            /* Erase from cursor to end of line */
                            if (client_data->term_seq_argv[0] == 0) {
                                guac_send_rect(io,
                                        GUAC_COMP_SRC, GUAC_DEFAULT_LAYER,
                                        client_data->cursor_col * client_data->char_width,
                                        client_data->cursor_row * client_data->char_height,
                                        (client_data->term_width - client_data->cursor_col) * client_data->char_width,
                                        client_data->char_height,
                                        0, 0, 0, 255); /* Background color */
                            }

                            /* Erase from start to cursor */
                            else if (client_data->term_seq_argv[0] == 1) {
                                guac_send_rect(io,
                                        GUAC_COMP_SRC, GUAC_DEFAULT_LAYER,
                                        0,
                                        client_data->cursor_row * client_data->char_height,
                                        client_data->cursor_col * client_data->char_width,
                                        client_data->char_height,
                                        0, 0, 0, 255); /* Background color */
                            }

                            /* Erase line */
                            else if (client_data->term_seq_argv[0] == 2) {
                                guac_send_rect(io,
                                        GUAC_COMP_SRC, GUAC_DEFAULT_LAYER,
                                        0,
                                        client_data->cursor_row * client_data->char_height,
                                        client_data->term_width * client_data->char_width,
                                        client_data->char_height,
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
                        client_data->term_state = SSH_TERM_STATE_ECHO;

                }

                /* End of SSH_TERM_STATE_CSI */
                break;

        }

        c++;
        size--;
    }

    return 0;

}

int ssh_guac_client_key_handler(guac_client* client, int keysym, int pressed) {

    ssh_guac_client_data* client_data = (ssh_guac_client_data*) client->data;

    /* If key pressed */
    if (pressed) {

        char data;

        /* If simple ASCII key */
        if (keysym >= 0x00 && keysym <= 0xFF)
            data = (char) keysym;

        else if (keysym == 0xFF08) data = 0x08;
        else if (keysym == 0xFF09) data = 0x09;
        else if (keysym == 0xFF0D) data = 0x0D;

        else
            return 0;

        return channel_write(client_data->term_channel, &data, 1);

    }

    return 0;

}
