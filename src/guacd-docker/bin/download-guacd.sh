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
## @fn download-guacd.sh
##
## Downloads and builds the given version of guacamole-server, automatically
## creating any required symbolic links for the proper loading of FreeRDP
## plugins.
##
## @param VERSION
##     The version of guacamole-server to download, such as "0.9.6".
##

VERSION="$1"
BUILD_DIR="/tmp"

##
## Locates the directory in which the FreeRDP libraries (.so files) are
## located, printing the result to STDOUT.
##
where_is_freerdp() {
    dirname `rpm -ql freerdp-devel | grep 'libfreerdp.*\.so' | head -n1`
}

#
# Download latest guacamole-server
#

curl -L "http://sourceforge.net/projects/guacamole/files/current/source/guacamole-server-$VERSION.tar.gz" | tar -xz -C "$BUILD_DIR"

#
# Build guacamole-server
#

cd "$BUILD_DIR/guacamole-server-$VERSION"
./configure
make
make install
ldconfig

#
# Clean up after build
#

rm -Rf "$BUILD_DIR/guacamole-server-$VERSION"

#
# Add FreeRDP plugins to proper path
#

FREERDP_DIR=`where_is_freerdp`
FREERDP_PLUGIN_DIR="$FREERDP_DIR/freerdp"

mkdir -p "$FREERDP_PLUGIN_DIR"
ln -s /usr/local/lib/freerdp/*.so "$FREERDP_PLUGIN_DIR"

