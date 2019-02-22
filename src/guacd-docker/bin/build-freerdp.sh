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
## @fn build-freerdp
##
## Builds the source of FreeRDP for supporting the guacd
## build.
##
## @param BUILD_DIR
##     The directory which currently contains the freerdp source and in which
##     the build should be performed.
##
## @param PREFIX_DIR
##     The directory prefix into which the build artifacts should be installed 
##     in which the build should be performed. This is passed to the --prefix
##     option of `configure`.
##

BUILD_DIR="$1"
PREFIX_DIR="$2"

#
# Build freerdp
#

mkdir ${BUILD_DIR}
cd "$BUILD_DIR"
git clone https://github.com/FreeRDP/FreeRDP FreeRDP
cd FreeRDP
git checkout 1.2.0-beta1+android9
curl https://github.com/FreeRDP/FreeRDP/commit/1b663ceffe51008af7ae9749e5b7999b2f7d6698.patch | patch -p1
cmake -DCMAKE_INSTALL_PREFIX=${PREFIX_DIR} -DWITH_DIRECTFB=OFF -DWITH_PULSE=ON -DWITH_CUPS=ON -DWITH_FFMPEG=OFF -DWITH_JPEG=ON -DWITH_OPENH264=OFF -DWITH_GSM=ON -DWITH_X11=OFF .
make
make install
ldconfig
