
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

#include "pool.h"
#include "util_suite.h"

#define UNSEEN          0 
#define SEEN_PHASE_1    1
#define SEEN_PHASE_2    2

#define POOL_SIZE 128

void test_guac_pool() {

    guac_pool* pool;

    int i;
    int seen[POOL_SIZE] = {0};
    int value;

    /* Get pool */
    pool = guac_pool_alloc(POOL_SIZE);
    CU_ASSERT_PTR_NOT_NULL_FATAL(pool);

    /* Fill pool */
    for (i=0; i<POOL_SIZE; i++) {

        /* Get value from pool */
        value = guac_pool_next_int(pool);

        /* Value should be within pool size */
        CU_ASSERT(value >= 0);
        CU_ASSERT(value <  POOL_SIZE);

        /* This should be an integer we have not seen yet */
        CU_ASSERT_EQUAL(UNSEEN, seen[value]);
        seen[value] = SEEN_PHASE_1;

        /* Return value to pool */
        guac_pool_free_int(pool, value);

    }

    /* Now that pool is filled, we should get ONLY previously seen integers */
    for (i=0; i<POOL_SIZE; i++) {

        /* Get value from pool */
        value = guac_pool_next_int(pool);

        /* Value should be within pool size */
        CU_ASSERT(value >= 0);
        CU_ASSERT(value <  POOL_SIZE);

        /* This should be an integer we have seen already */
        CU_ASSERT_EQUAL(SEEN_PHASE_1, seen[value]);
        seen[value] = SEEN_PHASE_2;

    }

    /* Pool is filled to minimum now. Next value should be equal to size. */
    value = guac_pool_next_int(pool);

    CU_ASSERT_EQUAL(POOL_SIZE, value);

    /* Free pool */
    guac_pool_free(pool);

}

