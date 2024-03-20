#!/bin/bash
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
## @fn start.sh
##
## Automatically configures and starts GUACD.
##


##
## Adds SSL command-line options to guacd
##
enable_ssl() {
    if [ -z "$GUACD_CERTIFICATE_CRT" ] || [ -z "$GUACD_CERTIFICATE_KEY" ]; then
        cat <<END
FATAL: Missing required environment variables
-------------------------------------------------------------------------------
If using a SSL encryption, you must provide both GUACD_CERTIFICATE_CRT
and GUACD_CERTIFICATE_KEY environment variables.

END
        exit 1;
    fi
    
    args+=( -C "$GUACD_CERTIFICATE_CRT" )
    args+=( -K "$GUACD_CERTIFICATE_KEY" )
}


# guacd default value if not overridden
if [ -z "$GUACD_LISTEN_ADDRESS" ]; then
    export GUACD_LISTEN_ADDRESS=0.0.0.0
fi
if [ -z "$GUACD_PORT" ]; then
    export GUACD_PORT=4822
fi
if [ -z "$GUACD_LOG_LEVEL" ]; then
    export GUACD_LOG_LEVEL=info
fi

args=( -b "${GUACD_LISTEN_ADDRESS}" -L "${GUACD_LOG_LEVEL}" -l "${GUACD_PORT}" -f )

# guacd certificate files
if [ -n "$GUACD_CERTIFICATE_CRT" ] || [ -n "$GUACD_CERTIFICATE_KEY" ]; then
    enable_ssl
fi


/opt/guacamole/sbin/guacd "${args[@]}"

