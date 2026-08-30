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

#ifndef GUAC_DBSHELL_HISTORY_H
#define GUAC_DBSHELL_HISTORY_H

/**
 * Declarations for the bounded, in-memory command history of the dbshell
 * REPL. History entries are stored only in memory and are freed when the
 * session ends; they are never written to disk.
 *
 * @file history.h
 */

/**
 * A bounded, in-memory ring of previously-submitted command lines, ordered
 * from oldest to newest. Once the ring is full, adding a new entry discards
 * the oldest entry.
 */
typedef struct guac_dbshell_history {

    /**
     * The circular buffer of history entries. Each entry is a
     * null-terminated, dynamically-allocated string, or NULL if the slot is
     * unused.
     */
    char** entries;

    /**
     * The maximum number of entries which may be stored within the ring.
     */
    int capacity;

    /**
     * The number of entries currently stored within the ring.
     */
    int length;

    /**
     * The index within entries of the slot which will receive the next
     * added entry.
     */
    int next;

} guac_dbshell_history;

/**
 * Allocates a new, empty history ring with the given capacity. The returned
 * history must eventually be freed with guac_dbshell_history_free().
 *
 * @param capacity
 *     The maximum number of entries the ring may hold. Must be positive.
 *
 * @return
 *     A newly-allocated, empty history ring.
 */
guac_dbshell_history* guac_dbshell_history_alloc(int capacity);

/**
 * Frees the given history ring and all entries stored within it.
 *
 * @param history
 *     The history ring to free.
 */
void guac_dbshell_history_free(guac_dbshell_history* history);

/**
 * Adds a copy of the given line to the given history ring as its newest
 * entry, discarding the oldest entry if the ring is full. If the given line
 * is empty or identical to the newest existing entry, the ring is not
 * modified.
 *
 * @param history
 *     The history ring to add the line to.
 *
 * @param line
 *     The line to add.
 */
void guac_dbshell_history_add(guac_dbshell_history* history,
        const char* line);

/**
 * Returns the history entry which is the given number of steps back from the
 * position following the newest entry. An offset of 1 returns the newest
 * entry, an offset of 2 the entry before it, and so on.
 *
 * @param history
 *     The history ring to retrieve the entry from.
 *
 * @param offset
 *     The number of steps back from the position following the newest
 *     entry.
 *
 * @return
 *     The requested history entry, or NULL if the offset does not denote a
 *     stored entry.
 */
const char* guac_dbshell_history_get(guac_dbshell_history* history,
        int offset);

#endif
