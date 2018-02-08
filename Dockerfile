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

#
# Dockerfile for guacamole-server
#


# Use Ubuntu as base for the build
ARG UBUNTU_VERSION=xenial
FROM ubuntu:${UBUNTU_VERSION} AS builder

# Base directory for installed build artifacts.
# Due to limitations of the Docker image build process, this value is
# duplicated in an ARG in the second stage of the build.
#
ARG PREFIX_DIR=/usr/local/guacamole

# Build arguments
ARG BUILD_DIR=/tmp/guacd-docker-BUILD
ARG BUILD_DEPENDENCIES="              \
        autoconf                      \
        automake                      \
        gcc                           \
        libcairo2-dev                 \
        libfreerdp-dev                \
        libjpeg-turbo8-dev            \
        libossp-uuid-dev              \
        libpango1.0-dev               \
        libpulse-dev                  \
        libssh2-1-dev                 \
        libssl-dev                    \
        libtelnet-dev                 \
        libtool                       \
        libvncserver-dev              \
        libwebp-dev                   \
        make"

# Build time environment
ENV LC_ALL=en_US.UTF-8

# Bring build environment up to date and install build dependencies
RUN apt-get update                         && \
    apt-get install -y $BUILD_DEPENDENCIES && \
    rm -rf /var/lib/apt/lists/*

# Add configuration scripts
COPY src/guacd-docker/bin /opt/guacd/bin/

# Copy source to container for sake of build
COPY . "$BUILD_DIR"

# Build guacamole-server from local source
RUN /opt/guacd/bin/build-guacd.sh "$BUILD_DIR" "$PREFIX_DIR"

# Use same Ubuntu as the base for the runtime image
FROM ubuntu:${UBUNTU_VERSION}

# Base directory for installed build artifacts.
# Due to limitations of the Docker image build process, this value is
# duplicated in an ARG in the first stage of the build. See also the
# CMD directive at the end of this build stage.
#
ARG PREFIX_DIR=/usr/local/guacamole

# Runtime environment
ENV LC_ALL=en_US.UTF-8
ENV GUACD_LOG_LEVEL=info

ARG RUNTIME_DEPENDENCIES="            \
        ghostscript                   \
        libcairo2                     \
        fonts-liberation              \
        fonts-dejavu                  \
        libfreerdp-cache1.1           \
        libfreerdp-client1.1          \
        libfreerdp-codec1.1           \
        libfreerdp-common1.1.0        \
        libfreerdp-core1.1            \
        libfreerdp-crypto1.1          \
        libfreerdp-gdi1.1             \
        libfreerdp-locale1.1          \
        libfreerdp-plugins-standard   \
        libfreerdp-primitives1.1      \
        libfreerdp-rail1.1            \
        libfreerdp-utils1.1           \
        libjpeg-turbo8                \
        libossp-uuid16                \
        libpango1.0                   \
        libpulse0                     \
        libssh2-1                     \
        libssl1.0.0                   \
        libtelnet2                    \
        libvncclient1                 \
        libwebp5                      \
        libwinpr-asn1-0.1             \
        libwinpr-bcrypt0.1            \
        libwinpr-credentials0.1       \
        libwinpr-credui0.1            \
        libwinpr-crt0.1               \
        libwinpr-crypto0.1            \
        libwinpr-dsparse0.1           \
        libwinpr-environment0.1       \
        libwinpr-error0.1             \
        libwinpr-file0.1              \
        libwinpr-handle0.1            \
        libwinpr-heap0.1              \
        libwinpr-input0.1             \
        libwinpr-interlocked0.1       \
        libwinpr-io0.1                \
        libwinpr-library0.1           \
        libwinpr-path0.1              \
        libwinpr-pipe0.1              \
        libwinpr-pool0.1              \
        libwinpr-registry0.1          \
        libwinpr-rpc0.1               \
        libwinpr-sspi0.1              \
        libwinpr-sspicli0.1           \
        libwinpr-synch0.1             \
        libwinpr-sysinfo0.1           \
        libwinpr-thread0.1            \
        libwinpr-timezone0.1          \
        libwinpr-utils0.1             \
        libwinpr-winhttp0.1           \
        libwinpr-winsock0.1           \
        xfonts-terminus"

# Bring runtime environment up to date and install runtime dependencies
RUN apt-get update                           && \
    apt-get install -y $RUNTIME_DEPENDENCIES && \
    rm -rf /var/lib/apt/lists/*

# Copy build artifacts into this stage
COPY --from=builder ${PREFIX_DIR} ${PREFIX_DIR}

# Link FreeRDP plugins into proper path
RUN FREERDP_DIR=$(dirname \
        $(dpkg-query -L libfreerdp | grep 'libfreerdp.*\.so' | head -n1)) && \
    FREERDP_PLUGIN_DIR="${FREERDP_DIR}/freerdp" && \
    mkdir -p "$FREERDP_PLUGIN_DIR" && \
    ln -s "$PREFIX_DIR"/lib/freerdp/*.so "$FREERDP_PLUGIN_DIR"

# Expose the default listener port
EXPOSE 4822

# Start guacd, listening on port 0.0.0.0:4822
#
# Note the path here MUST correspond to the value specified in the 
# PREFIX_DIR build argument.
#
CMD /usr/local/guacamole/sbin/guacd -b 0.0.0.0 -L $GUACD_LOG_LEVEL -f

