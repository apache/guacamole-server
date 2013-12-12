
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
 * The Original Code is libguac-client-rdp.
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

#ifndef __GUAC_RDP_DEBUG_H
#define __GUAC_RDP_DEBUG_H

#include <stdio.h>

/* Ensure GUAC_RDP_DEBUG_LEVEL is defined to a constant */
#ifndef GUAC_RDP_DEBUG_LEVEL
#define GUAC_RDP_DEBUG_LEVEL 0
#endif

/**
 * Prints a message to STDERR using the given printf format string and
 * arguments. This will only do anything if the GUAC_RDP_DEBUG_LEVEL
 * macro is defined and greater than the given log level.
 *
 * @param level The desired log level (an integer).
 * @param fmt The format to use when printing.
 * @param ... Arguments corresponding to conversion specifiers in the format
 *            string.
 */
#define GUAC_RDP_DEBUG(level, fmt, ...)                           \
    do {                                                          \
        if (GUAC_RDP_DEBUG_LEVEL >= level)                        \
            fprintf(stderr, "%s:%d: %s(): " fmt "\n",             \
                    __FILE__, __LINE__, __func__, __VA_ARGS__);   \
    } while (0);

#endif

