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
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef GUAC_THREAD_FNTYPES_H
#define GUAC_THREAD_FNTYPES_H

/**
 * Function type definitions related to threads.
 *
 * @file thread-fntypes.h
 */

/**
 * The function which runs within a guac_thread.
 *
 * @param data
 *     The arbitrary data passed to guac_thread_create().
 *
 * @return
 *     An arbitrary return value, which may be retrieved through
 *     guac_thread_join().
 */
typedef void* guac_thread_routine(void* data);

#endif
