
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

    int codepoint;
    char buffer[16];

    /* Test character length */
    CU_ASSERT_EQUAL(1, guac_utf8_charsize(UTF8_1b[0]));
    CU_ASSERT_EQUAL(2, guac_utf8_charsize(UTF8_2b[0]));
    CU_ASSERT_EQUAL(3, guac_utf8_charsize(UTF8_3b[0]));
    CU_ASSERT_EQUAL(4, guac_utf8_charsize(UTF8_4b[0]));

    /* Test string length */
    CU_ASSERT_EQUAL(0, guac_utf8_strlen(""));
    CU_ASSERT_EQUAL(1, guac_utf8_strlen(UTF8_4b));
    CU_ASSERT_EQUAL(2, guac_utf8_strlen(UTF8_4b UTF8_1b));
    CU_ASSERT_EQUAL(2, guac_utf8_strlen(UTF8_2b UTF8_3b));
    CU_ASSERT_EQUAL(3, guac_utf8_strlen(UTF8_1b UTF8_3b UTF8_4b));
    CU_ASSERT_EQUAL(3, guac_utf8_strlen(UTF8_2b UTF8_1b UTF8_3b));
    CU_ASSERT_EQUAL(3, guac_utf8_strlen(UTF8_4b UTF8_2b UTF8_1b));
    CU_ASSERT_EQUAL(3, guac_utf8_strlen(UTF8_3b UTF8_4b UTF8_2b));
    CU_ASSERT_EQUAL(5, guac_utf8_strlen("hello"));
    CU_ASSERT_EQUAL(9, guac_utf8_strlen("guacamole"));

    /* Test writes */
    CU_ASSERT_EQUAL(1, guac_utf8_write(0x00065, &(buffer[0]),  10));
    CU_ASSERT_EQUAL(2, guac_utf8_write(0x00654, &(buffer[1]),   9));
    CU_ASSERT_EQUAL(3, guac_utf8_write(0x00876, &(buffer[3]),   7));
    CU_ASSERT_EQUAL(4, guac_utf8_write(0x12345, &(buffer[6]),   4));
    CU_ASSERT_EQUAL(0, guac_utf8_write(0x00066, &(buffer[10]),  0));

    /* Test result of write */
    CU_ASSERT(memcmp("\x65",             &(buffer[0]), 1) == 0); /* U+0065  */
    CU_ASSERT(memcmp("\xD9\x94",         &(buffer[1]), 2) == 0); /* U+0654  */
    CU_ASSERT(memcmp("\xE0\xA1\xB6",     &(buffer[3]), 3) == 0); /* U+0876  */
    CU_ASSERT(memcmp("\xF0\x92\x8D\x85", &(buffer[6]), 4) == 0); /* U+12345 */

    /* Test reads */

    CU_ASSERT_EQUAL(1, guac_utf8_read(&(buffer[0]), 10, &codepoint));
    CU_ASSERT_EQUAL(0x0065, codepoint);

    CU_ASSERT_EQUAL(2, guac_utf8_read(&(buffer[1]),  9, &codepoint));
    CU_ASSERT_EQUAL(0x0654, codepoint);

    CU_ASSERT_EQUAL(3, guac_utf8_read(&(buffer[3]),  7, &codepoint));
    CU_ASSERT_EQUAL(0x0876, codepoint);

    CU_ASSERT_EQUAL(4, guac_utf8_read(&(buffer[6]),  4, &codepoint));
    CU_ASSERT_EQUAL(0x12345, codepoint);

    CU_ASSERT_EQUAL(0, guac_utf8_read(&(buffer[10]), 0, &codepoint));
    CU_ASSERT_EQUAL(0x12345, codepoint);

}

