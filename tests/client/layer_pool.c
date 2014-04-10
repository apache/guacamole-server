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

#include "client_suite.h"

#include <CUnit/Basic.h>
#include <guacamole/client.h>
#include <guacamole/layer.h>

void test_layer_pool() {

    guac_client* client;

    int i;
    int seen[GUAC_BUFFER_POOL_INITIAL_SIZE] = {0};

    guac_layer* layer;

    /* Get client */
    client = guac_client_alloc();
    CU_ASSERT_PTR_NOT_NULL_FATAL(client);

    /* Fill pool */
    for (i=0; i<GUAC_BUFFER_POOL_INITIAL_SIZE; i++) {

        /* Allocate and throw away a buffer (should not disturb layer alloc) */
        CU_ASSERT_PTR_NOT_NULL_FATAL(guac_client_alloc_buffer(client));

        layer = guac_client_alloc_layer(client);

        /* Index should be within pool size */
        CU_ASSERT_PTR_NOT_NULL_FATAL(layer);
        CU_ASSERT_FATAL(layer->index > 0);
        CU_ASSERT_FATAL(layer->index <= GUAC_BUFFER_POOL_INITIAL_SIZE);

        /* This should be a layer we have not seen yet */
        CU_ASSERT_FALSE(seen[layer->index - 1]);
        seen[layer->index - 1] = 1;

        guac_client_free_layer(client, layer);

    }

    /* Now that pool is filled, we should get a previously seen layer */
    layer = guac_client_alloc_layer(client);

    CU_ASSERT_FATAL(layer->index > 0);
    CU_ASSERT_FATAL(layer->index <= GUAC_BUFFER_POOL_INITIAL_SIZE);
    CU_ASSERT_TRUE(seen[layer->index - 1]);

    /* Free client */
    guac_client_free(client);

}

