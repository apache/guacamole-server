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

#ifndef GUACD_PROC_TITLE_H
#define GUACD_PROC_TITLE_H

#include <guacamole/client-types.h>

/**
 * The portion of the process title which precedes the name of the protocol
 * served by a connection process.
 */
#define GUACD_PROC_TITLE_PREFIX "guacd("

/**
 * The portion of the process title which follows the name of the protocol
 * served by a connection process.
 */
#define GUACD_PROC_TITLE_SUFFIX ")"

/**
 * The maximum number of bytes of a process title, including the null
 * terminator.
 */
#define GUACD_PROC_TITLE_BUFSIZE 256

/**
 * Establishes the writable region used for any future changes to the process
 * title by recording the extent of the name of the current process within
 * argv[0].
 *
 * @param process_name
 *     The pointer to the process name received by main() via argv[0].
 */
void guacd_proc_title_init(char* process_name);

/**
 * Updates the title of the current process to include information about the
 * connection being served, as may be useful to an administrator with access to
 * the process listing.
 *
 * @note
 *     The username is obfuscated before being included (for example, "bbennett"
 *     becomes "bb****tt"), but this obfuscation is NOT intended as an access
 *     control. A clever user or a user with insider information may still be
 *     able to deduce the username. This obfuscation is intended purely to
 *     provide some anonymity against casual observers, without entirely
 *     removing identifying information useful to administrators.
 *
 * If guacd_proc_title_init() was not called successfully, this function has no
 * effect.
 *
 * @param protocol
 *     The name of the protocol of the current process. If the protocol name was
 *     already provided in a prior call to this function, this may be NULL to
 *     reuse that value.
 *
 * @param info
 *     The details of the connection being served, or NULL if those details are
 *     not yet known and the title should identify only the protocol.
 */
void guacd_proc_title_set_connection(const char* protocol,
        const guac_client_info* info);

#endif
