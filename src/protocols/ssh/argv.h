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


#ifndef GUAC_SSH_ARGV_H
#define GUAC_SSH_ARGV_H

#include "config.h"

#include <guacamole/user.h>

/**
 * The maximum number of bytes to allow for any argument value received via an
 * argv stream, including null terminator.
 */
#define GUAC_SSH_ARGV_MAX_LENGTH 16384

/**
 * Handles an incoming stream from a Guacamole "argv" instruction, updating the
 * given connection parameter if that parameter is allowed to be updated.
 */
guac_user_argv_handler guac_ssh_argv_handler;

/**
 * Sends the current values of all non-sensitive parameters which may be set
 * while the connection is running to the given user. Note that the user
 * receiving these values will not necessarily be able to set new values
 * themselves if their connection is read-only. This function can be used as
 * the callback for guac_client_foreach_user() and guac_client_for_owner()
 *
 * @param user
 *     The user that should receive the values of all non-sensitive parameters
 *     which may be set while the connection is running.
 *
 * @param data
 *     The guac_ssh_client instance associated with the current connection.
 *
 * @return
 *     Always NULL.
 */
void* guac_ssh_send_current_argv(guac_user* user, void* data);

#endif

