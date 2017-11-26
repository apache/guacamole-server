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

#ifndef GUACLOG_STATE_H
#define GUACLOG_STATE_H

#include "config.h"

#include <stdbool.h>
#include <stdio.h>

/**
 * The current state of the Guacamole input log interpreter.
 */
typedef struct guaclog_state {

    /**
     * Output file stream.
     */
    FILE* output;

} guaclog_state;

/**
 * Allocates a new state structure for the Guacamole input log interpreter.
 * This structure serves as the representation of interpreter state as
 * input-related instructions are read and handled.
 *
 * @param path
 *     The full path to the file in which interpreted, human-readable should be
 *     written.
 *
 * @return
 *     The newly-allocated Guacamole input log interpreter state, or NULL if
 *     the state could not be allocated.
 */
guaclog_state* guaclog_state_alloc(const char* path);

/**
 * Frees all memory associated with the given Guacamole input log interpreter
 * state, and finishes any remaining interpreting process. If the given state
 * is NULL, this function has no effect.
 *
 * @param state
 *     The Guacamole input log interpreter state to free, which may be NULL.
 *
 * @return
 *     Zero if the interpreting process completed successfully, non-zero
 *     otherwise.
 */
int guaclog_state_free(guaclog_state* state);

/**
 * Updates the given Guacamole input log interpreter state, marking the given
 * key as pressed or released.
 *
 * @param state
 *     The Guacamole input log interpreter state being updated.
 *
 * @param keysym
 *     The X11 keysym of the key being pressed or released.
 *
 * @param pressed
 *     true if the key is being pressed, false if the key is being released.
 *
 * @return
 *     Zero if the interpreter state was updated successfully, non-zero
 *     otherwise.
 */
int guaclog_state_update_key(guaclog_state* state, int keysym, bool pressed);

#endif

