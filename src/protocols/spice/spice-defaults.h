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

#ifndef SPICE_DEFAULTS_H
#define SPICE_DEFAULTS_H

/**
 * The default hostname to connect to if none is specified.
 */
#define SPICE_DEFAULT_HOST "localhost"

/**
 * The default Spice port number to connect to if none is specified.
 */
#define SPICE_DEFAULT_PORT "5900"

/**
 * The default encodings to use for the Spice clipboard.
 */
#define SPICE_DEFAULT_ENCODINGS "zrle ultra copyrect hextile zlib corre rre raw"

/**
 * The default SFTP port to connect to if SFTP is enabled.
 */
#define SPICE_DEFAULT_SFTP_PORT "22"

/**
 * The default root directory to limit SFTP access to.
 */
#define SPICE_DEFAULT_SFTP_ROOT "/"

#endif /* SPICE_DEFAULTS_H */

