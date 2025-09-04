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

#ifndef GUAC_FILE_CONSTANTS_H
#define GUAC_FILE_CONSTANTS_H

/**
 * Constants relevant to the functions provided by "file.h".
 *
 * @file file-constants.h
 */

/**
 * The maximum numeric value allowed for the .1, .2, .3, etc. suffix appended
 * to the end of a filename if a file having the requested name already exists
 * and the GUAC_O_UNIQUE_SUFFIX flag is set.
 */
#define GUAC_FILE_UNIQUE_SUFFIX_MAX 255

#endif
