
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

#include "client.h"
#include "client_suite.h"

void test_resource_pool() {

    guac_client* client;
    guac_resource* resource0;
    guac_resource* resource1;
    guac_resource* resource2;

    /* Get client */
    client = guac_client_alloc();
    CU_ASSERT_PTR_NOT_NULL_FATAL(client);

    /*
     * POOL:   [ EMPTY         ]
     * IN USE: [ NONE          ]
     */

    /* First resource should be resource 0 */
    resource0 = guac_client_alloc_resource(client);
    CU_ASSERT_PTR_NOT_NULL_FATAL(resource0);
    CU_ASSERT_EQUAL_FATAL(0, resource0->index);

    /*
     * POOL:   [ EMPTY         ]
     * IN USE: [ 0             ]
     */

    /* Put 0 back in pool (1) */
    guac_client_free_resource(client, resource0);

    /*
     * POOL:   [ 0             ]
     * IN USE: [ NONE          ]
     */

    /* Since we free'd 0, we should get 0 again here (1) */
    resource0 = guac_client_alloc_resource(client);
    CU_ASSERT_PTR_NOT_NULL_FATAL(resource0);
    CU_ASSERT_EQUAL_FATAL(0, resource0->index);

    /*
     * POOL:   [ EMPTY         ]
     * IN USE: [ 0             ]
     */

    /* We should get a new resource here */
    resource1 = guac_client_alloc_resource(client);
    CU_ASSERT_PTR_NOT_NULL_FATAL(resource1);
    CU_ASSERT_EQUAL_FATAL(1, resource1->index);

    /*
     * POOL:   [ EMPTY         ]
     * IN USE: [ 0 1           ]
     */

    /* Put 0 back in pool (2) */
    guac_client_free_resource(client, resource0);

    /*
     * POOL:   [ 0             ]
     * IN USE: [ 1             ]
     */

    /* Since we free'd 0, we should get 0 again here (2) */
    resource0 = guac_client_alloc_resource(client);
    CU_ASSERT_PTR_NOT_NULL_FATAL(resource0);
    CU_ASSERT_EQUAL_FATAL(0, resource0->index);

    /*
     * POOL:   [ EMPTY         ]
     * IN USE: [ 0 1           ]
     */

    /* We should get a new resource here */
    resource2 = guac_client_alloc_resource(client);
    CU_ASSERT_PTR_NOT_NULL_FATAL(resource2);
    CU_ASSERT_EQUAL_FATAL(2, resource2->index);

    /*
     * POOL:   [ EMPTY         ]
     * IN USE: [ 0 1 2         ]
     */

    /* Free all resources */
    guac_client_free_resource(client, resource2);
    guac_client_free_resource(client, resource1);
    guac_client_free_resource(client, resource0);

    /*
     * POOL:   [ 0 1 2         ]
     * IN USE: [ EMPTY         ]
     */

    /* Free client */
    guac_client_free(client);

}

