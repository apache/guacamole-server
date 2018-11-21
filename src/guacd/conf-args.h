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

#ifndef _GUACD_CONF_ARGS_H
#define _GUACD_CONF_ARGS_H

#include "config.h"

#include "conf.h"

/**
 * Parses the given arguments into the given configuration. Zero is returned on
 * success, and non-zero is returned if arguments cannot be parsed.
 */
int guacd_conf_parse_args(guacd_config* config, int argc, char** argv);

/**
 * Finds a path to a configuration file in the given arguments. The path is
 * returned on success, and NULL is returned if appropriate option was not set.
 */
const char* guacd_conf_get_path(int argc, char** argv);

#endif

