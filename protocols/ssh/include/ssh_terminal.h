
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

#ifndef _SSH_GUAC_TERMINAL_H
#define _SSH_GUAC_TERMINAL_H

#include <stdlib.h>
#include <string.h>

#include <cairo/cairo.h>
#include <pango/pangocairo.h>

#include <guacamole/log.h>
#include <guacamole/guacio.h>
#include <guacamole/protocol.h>
#include <guacamole/client.h>

typedef struct ssh_guac_terminal ssh_guac_terminal;

typedef int ssh_guac_terminal_char_handler(ssh_guac_terminal* term, char c);

struct ssh_guac_terminal {

    guac_client* client;

    PangoFontDescription* font_desc;
    guac_layer* glyphs[256];

    int char_width;
    int char_height;

    int term_width;
    int term_height;

    int cursor_row;
    int cursor_col;

    int foreground;
    int background;
    int reverse;
    int bold;
    int underscore;

    int default_foreground;
    int default_background;

    ssh_guac_terminal_char_handler* char_handler;

};

typedef struct ssh_guac_terminal_color {
    int red;
    int green;
    int blue;
} ssh_guac_terminal_color;

extern const ssh_guac_terminal_color ssh_guac_terminal_palette[16];

ssh_guac_terminal* ssh_guac_terminal_create(guac_client* client);
void ssh_guac_terminal_free(ssh_guac_terminal* term);

int ssh_guac_terminal_write(ssh_guac_terminal* term, const char* c, int size);
int ssh_guac_terminal_set(ssh_guac_terminal* term, int row, int col,
        char c, int foreground, int background);

int ssh_guac_terminal_copy(ssh_guac_terminal* term,
        int src_row, int src_col, int rows, int cols,
        int dst_row, int dst_col);

int ssh_guac_terminal_clear(ssh_guac_terminal* term,
        int row, int col, int rows, int cols, int background_color);

int ssh_guac_terminal_scroll_up(ssh_guac_terminal* term,
        int start_row, int end_row, int amount);

int ssh_guac_terminal_clear_range(ssh_guac_terminal* term,
        int start_row, int start_col,
        int end_row, int end_col, int background_color);

#endif

