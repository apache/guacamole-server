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


#ifndef _GUAC_TEST_PROTOCOL_SUITE_H
#define _GUAC_TEST_PROTOCOL_SUITE_H

#include "config.h"

/* Unicode (UTF-8) strings */

#define UTF8_1 "\xe7\x8a\xac"            /* One character    */
#define UTF8_2 UTF8_1 "\xf0\x90\xac\x80" /* Two characters   */
#define UTF8_3 UTF8_2 "z"                /* Three characters */
#define UTF8_4 UTF8_3 "\xc3\xa1"         /* Four characters  */
#define UTF8_8 UTF8_4 UTF8_4             /* Eight characters */

int register_protocol_suite();

void test_base64_decode();
void test_instruction_parse();
void test_instruction_read();
void test_instruction_write();
void test_nest_write();

#endif

