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

#include "keymap.h"

#include <CUnit/CUnit.h>

/**
 * Test which verifies that guac_spice_keymap_find() returns the requested
 * keymap for known layout names and NULL for names that do not exist.
 */
void test_keymap__find(void) {

    /* The default keymap MUST exist and be named accordingly */
    const guac_spice_keymap* def =
        guac_spice_keymap_find(GUAC_SPICE_DEFAULT_KEYMAP);
    CU_ASSERT_PTR_NOT_NULL_FATAL(def);
    CU_ASSERT_STRING_EQUAL(def->name, GUAC_SPICE_DEFAULT_KEYMAP);

    /* A known, non-default keymap should also be found and correctly named */
    const guac_spice_keymap* de = guac_spice_keymap_find("de-de-qwertz");
    CU_ASSERT_PTR_NOT_NULL_FATAL(de);
    CU_ASSERT_STRING_EQUAL(de->name, "de-de-qwertz");

    /* Names which do not correspond to any keymap must return NULL */
    CU_ASSERT_PTR_NULL(guac_spice_keymap_find("no-such-keymap"));
    CU_ASSERT_PTR_NULL(guac_spice_keymap_find(""));

}

/**
 * Test which verifies that the global keymap registry is well-formed: it is
 * NULL-terminated, every entry has a name and mapping, and every registered
 * keymap can be retrieved by its own name via guac_spice_keymap_find().
 */
void test_keymap__registry(void) {

    int count = 0;

    for (const guac_spice_keymap** current = GUAC_SPICE_KEYMAPS;
            *current != NULL; current++) {

        const guac_spice_keymap* map = *current;

        /* Each registered keymap must have a name and a mapping */
        CU_ASSERT_PTR_NOT_NULL_FATAL(map->name);
        CU_ASSERT_PTR_NOT_NULL(map->mapping);

        /* ... and must be retrievable by that name */
        CU_ASSERT_PTR_EQUAL(guac_spice_keymap_find(map->name), map);

        count++;

    }

    /* At least the default keymap must be registered */
    CU_ASSERT_TRUE(count >= 1);

}

/**
 * Test which verifies that the mapping array of the default keymap is a valid,
 * zero-terminated array of key descriptors and defines at least one key.
 */
void test_keymap__mapping(void) {

    const guac_spice_keymap* def =
        guac_spice_keymap_find(GUAC_SPICE_DEFAULT_KEYMAP);
    CU_ASSERT_PTR_NOT_NULL_FATAL(def);
    CU_ASSERT_PTR_NOT_NULL_FATAL(def->mapping);

    /* Walk to the terminating (keysym == 0) entry, bounded to guard against a
     * malformed, non-terminated array */
    int entries = 0;
    const guac_spice_keysym_desc* desc = def->mapping;
    while (desc->keysym != 0 && entries < 100000) {
        desc++;
        entries++;
    }

    /* The default keymap must define at least one key and be terminated */
    CU_ASSERT_TRUE(entries > 0);
    CU_ASSERT_EQUAL(desc->keysym, 0);

}
