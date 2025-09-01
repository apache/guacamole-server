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

#ifndef GUAC_FILE_PRIVATE_H
#define GUAC_FILE_PRIVATE_H

/**
 * Functions used by the internals of "file.h".
 *
 * @file file-private.h
 */

/**
 * Tests whether the given string is a filename with no other path components
 * present.
 *
 * @param filename
 *     The filename to check.
 *
 * @return
 *     Non-zero if the provided string is a filename without any other path
 *     components, zero if at least one path component is present.
 */
int guac_is_filename(const char* filename);

#endif
