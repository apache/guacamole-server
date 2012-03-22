
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

#include "rdp_keymap.h"

const int GUAC_KEYSYMS_SHIFT[] = {0xFFE1, 0};
const int GUAC_KEYSYMS_ALL_SHIFT[] = {0xFFE1, 0xFFE2, 0};

const int GUAC_KEYSYMS_CTRL[] = {0xFFE3, 0};
const int GUAC_KEYSYMS_ALL_CTRL[] = {0xFFE3, 0xFFE4, 0};

const int GUAC_KEYSYMS_ALT[] = {0xFFE9, 0};
const int GUAC_KEYSYMS_ALL_ALT[] = {0xFFE9, 0xFFEA, 0};

const int GUAC_KEYSYMS_ALL_MODIFIERS[] = {
    0xFFE1, 0xFFE2, /* Left and right shift */
    0xFFE3, 0xFFE4, /* Left and right control */
    0xFFE9, 0xFFEA, /* Left and right alt */
    0
};

