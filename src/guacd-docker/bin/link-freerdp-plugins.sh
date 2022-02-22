#!/bin/sh -e
#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#

##
## @fn link-freerdp-plugins.sh
##
## Automatically creates any required symbolic links for the proper loading of
## the given FreeRDP plugins. If a given plugin is already in the correct
## directory, no link is created for that plugin.
##
## @param ...
##     The FreeRDP plugins to add links for.
##

##
## Locates the base directory of the FreeRDP installation (where the FreeRDP library
## .so files are located), printing the result to STDOUT. If the directory cannot be
## determined, an error is printed.
##
where_is_freerdp() {

    # Determine the location of any freerdp2 .so files
    PATHS="$(find / -iname '*libfreerdp2.so.*' \
             | xargs -r dirname                \
             | xargs -r realpath               \
             | sort -u)"

    # Verify that exactly one location was found
    if [ -z "$PATHS" -o "$(echo "$PATHS" | wc -l)" != 1 ]; then
        echo "$1: Unable to locate FreeRDP install location." >&2
        return 1
    fi

    echo "$PATHS"

}

#
# Create symbolic links as necessary to include all given plugins within the
# search path of FreeRDP
#

# Determine correct install location for FreeRDP plugins
FREERDP_DIR="$(where_is_freerdp)"
FREERDP_PLUGIN_DIR="${FREERDP_DIR}/freerdp2"

while [ -n "$1" ]; do

    # Add symbolic link if necessary
    if [ ! -e "$FREERDP_PLUGIN_DIR/$(basename "$1")" ]; then
        mkdir -p "$FREERDP_PLUGIN_DIR"
        ln -s "$1" "$FREERDP_PLUGIN_DIR"
    else
        echo "$1: Already in correct directory." >&2
    fi

    shift

done
