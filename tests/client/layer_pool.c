
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

