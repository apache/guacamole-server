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

#include "config.h"
#include "key-name.h"
#include "log.h"

#include <stdio.h>

int guaclog_key_name(char* key_name, int keysym) {

    /* Fallback to using hex keysym as name */
    int name_length = snprintf(key_name, GUACLOG_MAX_KEY_NAME_LENGTH,
            "0x%X", keysym);

    /* Truncate name if necessary */
    if (name_length >= GUACLOG_MAX_KEY_NAME_LENGTH) {
        name_length = GUACLOG_MAX_KEY_NAME_LENGTH - 1;
        key_name[name_length] = '\0';
        guaclog_log(GUAC_LOG_DEBUG, "Name for key 0x%X was "
                "truncated.", keysym);
    }

    return name_length;

}

