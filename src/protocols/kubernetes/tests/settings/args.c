/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "settings.h"

#include <CUnit/CUnit.h>

/**
 * Verifies that terminal argument names follow the indexes used by the
 * Kubernetes settings parser.
 */
void test_settings__terminal_args_order(void) {

    CU_ASSERT_STRING_EQUAL(GUAC_KUBERNETES_CLIENT_ARGS[28], "func-keys-and-keypad");
    CU_ASSERT_STRING_EQUAL(GUAC_KUBERNETES_CLIENT_ARGS[29], "scrollback");

}
