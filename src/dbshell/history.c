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

#include "config.h"

#include "dbshell/history.h"

#include <guacamole/mem.h>
#include <guacamole/string.h>

#include <string.h>

guac_dbshell_history* guac_dbshell_history_alloc(int capacity) {

    guac_dbshell_history* history = guac_mem_zalloc(sizeof(guac_dbshell_history));

    history->entries = guac_mem_zalloc(guac_mem_ckd_mul_or_die(capacity,
                sizeof(char*)));
    history->capacity = capacity;

    return history;

}

void guac_dbshell_history_free(guac_dbshell_history* history) {

    /* Free each stored entry */
    for (int i = 0; i < history->capacity; i++)
        guac_mem_free(history->entries[i]);

    guac_mem_free(history->entries);
    guac_mem_free(history);

}

void guac_dbshell_history_add(guac_dbshell_history* history,
        const char* line) {

    /* Never store empty lines */
    if (line == NULL || *line == '\0')
        return;

    /* Skip consecutive duplicates */
    const char* newest = guac_dbshell_history_get(history, 1);
    if (newest != NULL && strcmp(newest, line) == 0)
        return;

    /* Replace the oldest entry (if any) with the new entry */
    guac_mem_free(history->entries[history->next]);
    history->entries[history->next] = guac_strdup(line);

    /* Advance ring position */
    history->next = (history->next + 1) % history->capacity;
    if (history->length < history->capacity)
        history->length++;

}

const char* guac_dbshell_history_get(guac_dbshell_history* history,
        int offset) {

    /* Only offsets denoting stored entries are valid */
    if (offset < 1 || offset > history->length)
        return NULL;

    /* Walk backwards from the slot following the newest entry */
    int index = (history->next - offset + history->capacity)
        % history->capacity;

    return history->entries[index];

}
