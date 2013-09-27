
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

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <CUnit/Basic.h>

#include <guacamole/protocol.h>

#include "suite.h"


void test_base64_decode() {

    /* Test strings */
    char test_HELLO[]     = "SEVMTE8=";
    char test_AVOCADO[]   = "QVZPQ0FETw==";
    char test_GUACAMOLE[] = "R1VBQ0FNT0xF";

    /* Invalid strings */
    char invalid1[] = "====";
    char invalid2[] = "";

    /* Test one character of padding */
    CU_ASSERT_EQUAL(guac_protocol_decode_base64(test_HELLO), 5);
    CU_ASSERT_NSTRING_EQUAL(test_HELLO, "HELLO", 5);

    /* Test two characters of padding */
    CU_ASSERT_EQUAL(guac_protocol_decode_base64(test_AVOCADO), 7);
    CU_ASSERT_NSTRING_EQUAL(test_AVOCADO, "AVOCADO", 7);

    /* Test three characters of padding */
    CU_ASSERT_EQUAL(guac_protocol_decode_base64(test_GUACAMOLE), 9);
    CU_ASSERT_NSTRING_EQUAL(test_GUACAMOLE, "GUACAMOLE", 9);

    /* Verify invalid strings stop early as expected */
    CU_ASSERT_EQUAL(guac_protocol_decode_base64(invalid1), 0);
    CU_ASSERT_EQUAL(guac_protocol_decode_base64(invalid2), 0);

}

