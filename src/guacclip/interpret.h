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

#ifndef GUACCLIP_INTERPRET_H
#define GUACCLIP_INTERPRET_H

#include "state.h"

#include <stdbool.h>

/**
 * Extracts all clipboard artifacts (text and images) from the given Guacamole
 * protocol dump, writing per-item files and a manifest.json into the output
 * directory specified by the given options. A read lock will be acquired on the
 * input file to ensure that in-progress recordings are not interpreted. This
 * behavior can be overridden by specifying true for the force parameter.
 *
 * @param path
 *     The path to the file containing the raw Guacamole protocol dump.
 *
 * @param options
 *     The options controlling extraction, including the output directory.
 *
 * @param force
 *     Interpret even if the input file appears to be an in-progress recording
 *     (has an associated lock).
 *
 * @return
 *     Zero on success, non-zero if an error prevented successful extraction.
 */
int guacclip_interpret(const char* path, const guacclip_options* options,
        bool force);

#endif
