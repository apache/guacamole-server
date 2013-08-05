
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

#include <CUnit/Basic.h>

#include <guacamole/unicode.h>
#include "util_suite.h"

void test_guac_unicode() {

    /* Test character length */
    CU_ASSERT_EQUAL(1, guac_utf8_charsize(UTF8_1[1]));
    CU_ASSERT_EQUAL(2, guac_utf8_charsize(UTF8_2[1]));
    CU_ASSERT_EQUAL(3, guac_utf8_charsize(UTF8_3[1]));
    CU_ASSERT_EQUAL(4, guac_utf8_charsize(UTF8_4[1]));

    /* Test string length */
    CU_ASSERT_EQUAL(0,  guac_utf8_strlen(""));
    CU_ASSERT_EQUAL(4,  guac_utf8_strlen(UTF8_4));
    CU_ASSERT_EQUAL(5,  guac_utf8_strlen(UTF8_1 UTF8_3 UTF8_1));
    CU_ASSERT_EQUAL(5,  guac_utf8_strlen("hello"));
    CU_ASSERT_EQUAL(6,  guac_utf8_strlen(UTF8_2 UTF8_1 UTF8_3));
    CU_ASSERT_EQUAL(8,  guac_utf8_strlen(UTF8_8));
    CU_ASSERT_EQUAL(9,  guac_utf8_strlen("guacamole"));
    CU_ASSERT_EQUAL(11, guac_utf8_strlen(UTF8_2 UTF8_1 UTF8_8));

    /*int guac_utf8_write(int codepoint, char* utf8, int length);
    int guac_utf8_read(const char* utf8, int length, int* codepoint);*/

}

