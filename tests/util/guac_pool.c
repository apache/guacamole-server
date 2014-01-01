/*
 * Copyright (C) 2013 Glyptodon LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "config.h"

#include "util_suite.h"

#include <CUnit/Basic.h>
#include <guacamole/pool.h>

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
        CU_ASSERT_FATAL(value >= 0);
        CU_ASSERT_FATAL(value <  POOL_SIZE);

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
        CU_ASSERT_FATAL(value >= 0);
        CU_ASSERT_FATAL(value <  POOL_SIZE);

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

