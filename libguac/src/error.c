
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

#include "error.h"


/* Error strings */

const char* __GUAC_STATUS_SUCCESS_STR        = "Success";
const char* __GUAC_STATUS_NO_MEMORY_STR      = "Insufficient memory";
const char* __GUAC_STATUS_NO_INPUT_STR       = "End of input stream";
const char* __GUAC_STATUS_INPUT_TIMEOUT_STR  = "Read timeout";
const char* __GUAC_STATUS_OUTPUT_ERROR_STR   = "Output error";
const char* __GUAC_STATUS_BAD_ARGUMENT_STR   = "Invalid argument";
const char* __GUAC_STATUS_BAD_STATE_STR      = "Illegal state";
const char* __GUAC_STATUS_INVALID_STATUS_STR = "UNKNOWN STATUS CODE";


const char* guac_status_string(guac_status_t status) {

    switch (status) {

        /* No error */
		case GUAC_STATUS_SUCCESS:
            return __GUAC_STATUS_SUCCESS_STR;

        /* Out of memory */
		case GUAC_STATUS_NO_MEMORY:
            return __GUAC_STATUS_NO_MEMORY_STR;

        /* End of input */
		case GUAC_STATUS_NO_INPUT:
            return __GUAC_STATUS_NO_INPUT_STR;

        /* Input timeout */
		case GUAC_STATUS_INPUT_TIMEOUT:
            return __GUAC_STATUS_INPUT_TIMEOUT_STR;

        /* Output error */
		case GUAC_STATUS_OUTPUT_ERROR:
            return __GUAC_STATUS_OUTPUT_ERROR_STR;

        /* Invalid argument */
		case GUAC_STATUS_BAD_ARGUMENT:
            return __GUAC_STATUS_BAD_ARGUMENT_STR;

        /* Illegal state */
		case GUAC_STATUS_BAD_STATE:
            return __GUAC_STATUS_BAD_STATE_STR;

        default:
            return __GUAC_STATUS_INVALID_STATUS_STR;

    }

}

