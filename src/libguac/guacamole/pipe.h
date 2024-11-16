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

#ifndef _GUAC_PIPE_H
#define _GUAC_PIPE_H

#include <fcntl.h>
#include <io.h>

/**
 * The default amount of memory to dedicate to the pipe. For more info, see
 * https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/pipe.
 */
#define DEFAULT_PIPE_MEMORY 8092

/**
 * Replicate the behavior of the default posix pipe() function by deferring to
 * the windows _pipe() API, with some sensible defaults.
 */
#define pipe(fds) (_pipe((fds), DEFAULT_PIPE_MEMORY, O_BINARY))

#endif
