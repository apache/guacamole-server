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
## @fn build-all.sh
##
## Builds the source of guacamole-server and its various core protocol library
## dependencies.
##

# Pre-populate build control variables such that the custom build prefix is
# used for C headers, locating libraries, etc.
export CFLAGS="-I${PREFIX_DIR}/include"
export LDFLAGS="-L${PREFIX_DIR}/lib"
export PKG_CONFIG_PATH="${PREFIX_DIR}/lib/pkgconfig" 

# Ensure thread stack size will be 8 MB (glibc's default on Linux) rather than
# 128 KB (musl's default)
export LDFLAGS="$LDFLAGS -Wl,-z,stack-size=8388608"

#
# Build guacamole-server
#

cd "$BUILD_DIR"
autoreconf -fi && ./configure --prefix="$PREFIX_DIR" $GUACAMOLE_SERVER_OPTS
make && make install

