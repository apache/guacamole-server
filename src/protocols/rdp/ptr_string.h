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

#ifndef GUAC_RDP_PTR_STRING_H
#define GUAC_RDP_PTR_STRING_H

#include "config.h"

#include <guacamole/client.h>

/**
 * Converts the given string back into a void pointer. The string MUST have
 * been produced via guac_rdp_ptr_to_string().
 *
 * @param str
 *     The string to convert back to a pointer.
 *
 * @return
 *     The pointer value of the given string, as originally passed to
 *     guac_rdp_ptr_to_string().
 */
void* guac_rdp_string_to_ptr(const char* str);

/**
 * Converts a void pointer into a string representation, safe for use with
 * parts of the FreeRDP API which provide only for passing arbitrary strings,
 * despite being within the same memory area. The returned string must
 * eventually be freed with a call to free().
 *
 * @param data
 *     The void pointer to convert to a string.
 *
 * @return
 *     A newly-allocated string containing the string representation of the
 *     given void pointer. This string must eventually be freed with a call to
 *     free().
 */
char* guac_rdp_ptr_to_string(void* data);

#endif

