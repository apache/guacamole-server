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

#ifndef GUACCLIP_H
#define GUACCLIP_H

/**
 * The default log level below which no messages should be logged.
 */
#define GUACCLIP_DEFAULT_LOG_LEVEL GUAC_LOG_INFO

/**
 * The default maximum number of bytes to accumulate for any single clipboard
 * item when the user has not explicitly specified "--max-item-bytes". This
 * bounds memory growth against a hostile or malformed recording which
 * streams an unbounded number of blobs into a single clipboard stream. Users
 * may explicitly opt out of any limit by specifying "--max-item-bytes 0".
 */
#define GUACCLIP_DEFAULT_MAX_ITEM_BYTES (64 * 1024 * 1024)

#endif
