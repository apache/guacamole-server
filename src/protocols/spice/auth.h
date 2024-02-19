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

#ifndef GUAC_SPICE_AUTH_H
#define GUAC_SPICE_AUTH_H

#include "config.h"

#include <guacamole/client.h>

#include <glib-unix.h>

/**
 * Handler invoked when an authentication error is received from the Spice
 * server, which retrieves the credentials from the Guacamole Client accessing
 * the connection, if those credentials have not been explicitly set in the
 * configuration. Returns TRUE if credentials are successfully retrieved, or
 * FALSE otherwise.
 * 
 * @param client
 *     The guac_client that is attempting to connect to the Spice server and
 *     that will be asked for the credentials.
 * 
 * @return
 *     TRUE if the credentials are retrieved from the users; otherwise FALSE.
 */
gboolean guac_spice_get_credentials(guac_client* client);

#endif /* GUAC_SPICE_AUTH_H */

