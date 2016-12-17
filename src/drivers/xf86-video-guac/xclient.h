
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

#ifndef GUAC_DRV_XCLIENT_H
#define GUAC_DRV_XCLIENT_H

#include <xcb/xcb.h>

/**
 * Creates a new client connection to display associated with the Guacamole
 * X.Org driver using XCB.
 *
 * @return
 *     A new XCB connection to the display associated with the Guacamole
 *     X.Org driver, or NULL if the connection cannot be established.
 */
xcb_connection_t* guac_drv_get_connection();

#endif

