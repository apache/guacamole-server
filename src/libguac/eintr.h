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

#ifndef GUAC_EINTR_H
#define GUAC_EINTR_H

#include <errno.h>

/**
 * Execute the expression and retry if it fails with errno =
 * EINTR.
 *
 * @param retval
 *     Return value.
 *
 * @param expression
 *     Expression (function) to run.
 */
#define GUAC_EINTR_RETRY(retval, expression)      \
    do {                                          \
        do {                                      \
            (retval) = (expression);              \
        } while ((retval) < 0 && errno == EINTR); \
    } while (0)
#endif
