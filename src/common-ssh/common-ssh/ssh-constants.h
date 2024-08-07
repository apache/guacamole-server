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

#ifndef GUAC_COMMON_SSH_CONSTANTS_H
#define GUAC_COMMON_SSH_CONSTANTS_H

/**
 * The default port to use for SSH and SFTP connections.
 */
#define GUAC_COMMON_SSH_DEFAULT_PORT "22"

/**
 * The default interval at which to send keepalives, which is zero, where
 * keepalives will not be sent.
 */
#define GUAC_COMMON_SSH_DEFAULT_ALIVE_INTERVAL 0

/**
 * For SFTP connections, the default root directory at which to start
 * the session.
 */
#define GUAC_COMMON_SSH_SFTP_DEFAULT_ROOT "/"

#endif