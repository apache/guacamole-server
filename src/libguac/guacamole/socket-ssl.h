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

#ifndef __GUACD_SOCKET_SSL_H
#define __GUACD_SOCKET_SSL_H

/**
 * Provides an SSL/TLS implementation of guac_socket. This header will only be
 * available if libguac was built with SSL support.
 *
 * @file socket-ssl.h
 */

#include "socket-types.h"

#include <openssl/ssl.h>

/**
 * Creates a new guac_socket which will use SSL for all communication. Freeing
 * this guac_socket will automatically close the associated file descriptor.
 *
 * @param context
 *     The SSL_CTX structure describing the desired SSL configuration.
 *
 * @param fd
 *     The file descriptor to use for the SSL connection underlying the
 *     created guac_socket.
 *
 * @return
 *     A newly-allocated guac_socket which will transparently use SSL for
 *     all communication.
 */
guac_socket* guac_socket_open_secure(SSL_CTX* context, int fd);

#endif

