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
## Given the full path to a FreeRDP plugin, locates the base directory of the
## associated FreeRDP installation (where the FreeRDP library .so files are
## located), printing the result to STDOUT. If the directory cannot be
## determined, an error is printed.
##
## @param PLUGIN_FILE
##     The full path to the FreeRDP plugin to check.
##
where_is_freerdp() {

    PLUGIN_FILE="$1"

    # Determine the location of all libfreerdp* libraries explicitly linked
    # to given file
    PATHS="$(ldd "$PLUGIN_FILE"              \
                 | awk '/=>/{print $(NF-1)}' \
                 | grep 'libfreerdp'         \
                 | xargs -r dirname          \
                 | xargs -r realpath         \
                 | sort -u)"

    # Verify that exactly one location was found
    if [ "$(echo "$PATHS" | wc -l)" != 1 ]; then
        echo "$1: Unable to locate FreeRDP install location." >&2
        return 1
    fi

    echo "$PATHS"

}

#
# Create symbolic links as necessary to include all given plugins within the
# search path of FreeRDP
#

while [ -n "$1" ]; do

    # Determine correct install location for FreeRDP plugins
    FREERDP_DIR="$(where_is_freerdp "$1")"
    FREERDP_PLUGIN_DIR="${FREERDP_DIR}/freerdp2"

    # Add symbolic link if necessary
    if [ ! -e "$FREERDP_PLUGIN_DIR/$(basename "$1")" ]; then
        mkdir -p "$FREERDP_PLUGIN_DIR"
        ln -s "$1" "$FREERDP_PLUGIN_DIR"
    else
        echo "$1: Already in correct directory." >&2
    fi

    shift

done

