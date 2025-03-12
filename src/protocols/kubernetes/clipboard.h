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

#ifndef GUAC_KUBERNETES_CLIPBOARD_H
#define GUAC_KUBERNETES_CLIPBOARD_H

#include <guacamole/user.h>

/**
 * Handler for inbound clipboard streams.
 */
guac_user_clipboard_handler guac_kubernetes_clipboard_handler;

/**
 * Handler for data received along clipboard streams.
 */
guac_user_blob_handler guac_kubernetes_clipboard_blob_handler;

/**
 * Handler for end-of-stream related to clipboard.
 */
guac_user_end_handler guac_kubernetes_clipboard_end_handler;

#endif

