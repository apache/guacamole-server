
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

#ifndef _SSH_GUAC_DELTA_H
#define _SSH_GUAC_DELTA_H

#include <guacamole/client.h>
#include "terminal.h"

/**
 * All available terminal operations which affect character cells.
 */
typedef enum guac_terminal_operation_type {

    /**
     * Operation which does nothing.
     */
    GUAC_CHAR_NOP,

    /**
     * Operation which copies a character from a given row/column coordinate.
     */
    GUAC_CHAR_COPY,

    /**
     * Operation which sets the character and attributes.
     */
    GUAC_CHAR_SET

} guac_terminal_operation_type;

/**
 * A pairing of a guac_terminal_operation_type and all parameters required by
 * that operation type.
 */
typedef struct guac_terminal_operation {

    /**
     * The type of operation to perform.
     */
    guac_terminal_operation_type type;

    /**
     * The character (and attributes) to set the current location to. This is
     * only applicable to GUAC_CHAR_SET.
     */
    guac_terminal_char character;

    /**
     * The row to copy a character from. This is only applicable to
     * GUAC_CHAR_COPY.
     */
    int row;

    /**
     * The column to copy a character from. This is only applicable to
     * GUAC_CHAR_COPY.
     */
    int column;

} guac_terminal_operation;

/**
 * Set of all pending operations for the currently-visible screen area.
 */
typedef struct guac_terminal_delta {

    /**
     * Array of all operations pending for the visible screen area.
     */
    guac_terminal_operation* operations;

    /**
     * The width of the screen, in characters.
     */
    int width;

    /**
     * The height of the screen, in characters.
     */
    int height;

} guac_terminal_delta;

/**
 * Allocates a new guac_terminal_delta.
 */
guac_terminal_delta* guac_terminal_delta_alloc(int width, int height);

/**
 * Frees the given guac_terminal_delta.
 */
void guac_terminal_delta_free(guac_terminal_delta* delta);

/**
 * Resizes the given guac_terminal_delta to the given dimensions.
 */
void guac_terminal_delta_resize(guac_terminal_delta* delta,
    int width, int height);

/**
 * Stores a set operation at the given location.
 */
void guac_terminal_delta_set(guac_terminal_delta* delta, int r, int c,
        guac_terminal_char* character);

/**
 * Stores a rectangle of copy operations, copying existing operations as
 * necessary.
 */
void guac_terminal_delta_copy(guac_terminal_delta* delta,
        int dst_row, int dst_column,
        int src_row, int src_column,
        int w, int h);

/**
 * Flushes all pending operations within the given guac_client_delta to the
 * given guac_terminal.
 */
void guac_terminal_delta_flush(guac_terminal_delta* delta,
        guac_terminal* terminal);

#endif

