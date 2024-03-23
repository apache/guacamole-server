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

##
## Builds and installs the source at the given git repository, automatically
## switching to the version of the source at the tag/commit that matches the
## given pattern.
##
## @param URL
##     The URL of the git repository that the source should be downloaded from.
##
## @param PATTERN
##     The Perl-compatible regular expression that the tag must match. If no
##     tag matches the regular expression, the pattern is assumed to be an
##     exact reference to a commit, branch, etc. acceptable by git checkout.
##
## @param ...
##     Any additional command-line options that should be provided to CMake or
##     the configure script.
##
install_from_git() {

    URL="$1"
    PATTERN="$2"
    shift 2

    # Calculate top-level directory name of resulting repository from the
    # provided URL
    REPO_DIR="$(basename "$URL" .git)"

    # Allow dependencies to be manually omitted with the tag/commit pattern "NO"
    if [ "$PATTERN" = "NO" ]; then
        echo "NOT building $REPO_DIR (explicitly skipped)"
        return
    fi

    # Clone repository and change to top-level directory of source
    cd /tmp
    git clone "$URL"
    cd $REPO_DIR/

    # Locate tag/commit based on provided pattern
    VERSION="$(git tag -l --sort=-v:refname | grep -Px -m1 "$PATTERN" \
        || echo "$PATTERN")"

    # Switch to desired version of source
    echo "Building $REPO_DIR @ $VERSION ..."
    git -c advice.detachedHead=false checkout "$VERSION"

    # Configure build using CMake or GNU Autotools, whichever happens to be
    # used by the library being built
    if [ -e CMakeLists.txt ]; then
        cmake -DCMAKE_INSTALL_PREFIX:PATH="$PREFIX_DIR" "$@" .
    else
        [ -e configure ] || autoreconf -fi
        ./configure --prefix="$PREFIX_DIR" "$@"
    fi

    # Build and install
    make && make install

}

#
# Determine any option overrides to guarantee successful build
#

export BUILD_ARCHITECTURE="$(arch)" # Determine architecture building on
echo "Build architecture: $BUILD_ARCHITECTURE"

case $BUILD_ARCHITECTURE in
    armv6l|armv7l|aarch64)
        export FREERDP_OPTS_OVERRIDES="-DWITH_SSE2=OFF" # Disable SSE2 on ARM
        ;;
    *)
        export FREERDP_OPTS_OVERRIDES=""
        ;;
esac

#
# Build and install core protocol library dependencies
#

install_from_git "https://github.com/FreeRDP/FreeRDP" "$WITH_FREERDP" $FREERDP_OPTS $FREERDP_OPTS_OVERRIDES
install_from_git "https://github.com/libssh2/libssh2" "$WITH_LIBSSH2" $LIBSSH2_OPTS
install_from_git "https://github.com/seanmiddleditch/libtelnet" "$WITH_LIBTELNET" $LIBTELNET_OPTS
install_from_git "https://github.com/LibVNC/libvncserver" "$WITH_LIBVNCCLIENT" $LIBVNCCLIENT_OPTS
install_from_git "https://github.com/warmcat/libwebsockets" "$WITH_LIBWEBSOCKETS" $LIBWEBSOCKETS_OPTS

#
# Build guacamole-server
#

cd "$BUILD_DIR"
autoreconf -fi && ./configure --prefix="$PREFIX_DIR" $GUACAMOLE_SERVER_OPTS
make && make check && make install

