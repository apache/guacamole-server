
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
 * The Original Code is libguac.
 *
 * The Initial Developer of the Original Code is
 * Michael Jumper.
 * Portions created by the Initial Developer are Copyright (C) 2010
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

#include <stddef.h>

#include "unicode.h"

size_t guac_utf8_charsize(unsigned char c) {

    /* Determine size in bytes of character */
    if ((c >>= 1) == 0x7E) return 6;
    if ((c >>= 1) == 0x3E) return 5;
    if ((c >>= 1) == 0x1E) return 4;
    if ((c >>= 1) == 0x0E) return 3;
    if ((c >>= 1) == 0x06) return 2;

    /* Default to one character */
    return 1;

}

size_t guac_utf8_strlen(const char* str) {

    /* The current length of the string */
    int length = 0;

    /* Number of characters before start of next character */
    int skip = 0;

    while (*str != 0) {

        /* If skipping, then skip */
        if (skip > 0) skip--;

        /* Otherwise, determine next skip value, and increment length */
        else {

            /* Get next character */
            unsigned char c = (unsigned char) *str;

            /* Determine skip value (size in bytes of rest of character) */
            skip = guac_utf8_charsize(c) - 1;

            length++;
        }

        str++;
    }

    return length;

}

