
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

#ifndef _SSH_GUAC_TYPES_H
#define _SSH_GUAC_TYPES_H

#include <stdbool.h>

/**
 * An RGB color, where each component ranges from 0 to 255.
 */
typedef struct guac_terminal_color {

    /**
     * The red component of this color.
     */
    int red;

    /**
     * The green component of this color.
     */
    int green;

    /**
     * The blue component of this color.
     */
    int blue;

} guac_terminal_color;

/**
 * Terminal attributes, as can be applied to a single character.
 */
typedef struct guac_terminal_attributes {

    /**
     * Whether the character should be rendered bold.
     */
    bool bold;

    /**
     * Whether the character should be rendered with reversed colors
     * (background becomes foreground and vice-versa).
     */
    bool reverse;

    /**
     * Whether the associated character is selected.
     */
    bool selected;

    /**
     * Whether to render the character with underscore.
     */
    bool underscore;

    /**
     * The foreground color of this character, as a palette index.
     */
    int foreground;

    /**
     * The background color of this character, as a palette index.
     */
    int background;

} guac_terminal_attributes;

/**
 * Represents a single character for display in a terminal, including actual
 * character value, foreground color, and background color.
 */
typedef struct guac_terminal_char {

    /**
     * The Unicode codepoint of the character to display.
     */
    int value;

    /**
     * The attributes of the character to display.
     */
    guac_terminal_attributes attributes;

} guac_terminal_char;

#endif

