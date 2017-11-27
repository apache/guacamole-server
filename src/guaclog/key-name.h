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

#ifndef GUACLOG_KEY_NAME_H
#define GUACLOG_KEY_NAME_H

#include "config.h"

/**
 * The maximum size of the name of any key, in bytes.
 */
#define GUACLOG_MAX_KEY_NAME_LENGTH 64

/**
 * Copies the name of the key having the given keysym into the given buffer,
 * which must be at least GUACLOG_MAX_KEY_NAME_LENGTH bytes long. This function
 * always succeeds, ultimately resorting to using the hex value of the keysym
 * as the name if no other human-readable name is known.
 *
 * @param key_name
 *     The buffer to copy the key name into, which must be at least
 *     GUACLOG_MAX_KEY_NAME_LENGTH.
 *
 * @param keysym
 *     The X11 keysym of the key whose name should be stored in
 *     key_name.
 *
 * @return
 *     The length of the name, in bytes, excluding null terminator.
 */
int guaclog_key_name(char* key_name, int keysym);

#endif

