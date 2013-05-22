
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

#include <stdbool.h>
#include <unistd.h>

int guac_terminal_fit_to_range(int value, int min, int max) {

    if (value < min) return min;
    if (value > max) return max;

    return value;

}

int guac_terminal_encode_utf8(int codepoint, char* utf8) {

    int i;
    int mask, bytes;

    /* Determine size and initial byte mask */
    if (codepoint <= 0x007F) {
        mask  = 0x00;
        bytes = 1;
    }
    else if (codepoint <= 0x7FF) {
        mask  = 0xC0;
        bytes = 2;
    }
    else if (codepoint <= 0xFFFF) {
        mask  = 0xE0;
        bytes = 3;
    }
    else if (codepoint <= 0x1FFFFF) {
        mask  = 0xF0;
        bytes = 4;
    }

    /* Otherwise, invalid codepoint */
    else {
        *(utf8++) = '?';
        return 1;
    }

    /* Offset buffer by size */
    utf8 += bytes - 1;

    /* Add trailing bytes, if any */
    for (i=1; i<bytes; i++) {
        *(utf8--) = 0x80 | (codepoint & 0x3F);
        codepoint >>= 6;
    }

    /* Set initial byte */
    *utf8 = mask | codepoint;

    /* Done */
    return bytes;

}

bool guac_terminal_has_glyph(int codepoint) {
    return
           codepoint != 0
        && codepoint != ' ';
}

int guac_terminal_write_all(int fd, const char* buffer, int size) {

    int remaining = size;
    while (remaining > 0) {

        /* Attempt to write data */
        int ret_val = write(fd, buffer, remaining);
        if (ret_val <= 0)
            return -1;

        /* If successful, contine with what data remains (if any) */
        remaining -= ret_val;
        buffer += ret_val;

    }

    return size;

}

