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
 * The name of the parameter that Guacamole uses to collect the username from
 * the Guacamole client and send it to the SSH server.
 */
#define GUAC_SSH_PARAMETER_NAME_USERNAME "username"

/**
 * The name of the parameter that Guacamole uses to collect the password from
 * the Guacamole client and send it to the SSH server.
 */
#define GUAC_SSH_PARAMETER_NAME_PASSWORD "password"

/**
 * The name of the parameter that Guacamole uses to collect a SSH key passphrase
 * from the Guacamole client in order to unlock a private key.
 */
#define GUAC_SSH_PARAMETER_NAME_PASSPHRASE "passphrase"

/**
 * The name of the parameter that Guacamole uses to collect the terminal
 * color scheme from the Guacamole client.
 */
#define GUAC_SSH_PARAMETER_NAME_COLOR_SCHEME "color-scheme"

/**
 * The name of the parameter that Guacamole uses to collect the font name
 * from the Guacamole client.
 */
#define GUAC_SSH_PARAMETER_NAME_FONT_NAME "font-name"

/**
 * The name of the parameter that Guacamole uses to collect the font size
 * from the Guacamole client
 */
#define GUAC_SSH_PARAMETER_NAME_FONT_SIZE "font-size"

#endif /* GUAC_COMMON_SSH_CONSTANTS_H */

