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
## @fn list-dependencies.sh
##
## Lists the Debian/Ubuntu package names for all library dependencies of the
## given binaries. Each package is only listed once, even if multiple binaries
## provided by the same package are given.
##
## @param ...
##     The full paths to all binaries being checked.
##

while [ -n "$1" ]; do

    # For all non-Guacamole library dependencies
    ldd "$1" | grep -v 'libguac' | awk '/=>/{print $(NF-1)}' \
        | while read LIBRARY; do

        # Determine the Debian package which is associated with that
        # library, if any
        dpkg-query -S "$LIBRARY" 2> /dev/null || true

    done

    # Next binary
    shift

done | cut -f1 -d: | sort -u

