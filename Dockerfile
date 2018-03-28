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


# Use CentOS as base for the build
ARG CENTOS_VERSION=centos7
FROM centos:${CENTOS_VERSION} AS builder

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
        cairo-devel                   \
        freerdp-devel                 \
        gcc                           \
        libjpeg-turbo-devel           \
        libssh2-devel                 \
        libtool                       \
        libtelnet-devel               \
        libvorbis-devel               \
        libvncserver-devel            \
        libwebp-devel                 \
        make                          \
        pango-devel                   \
        pulseaudio-libs-devel         \
        uuid-devel"

# Build time environment
ENV LC_ALL=en_US.UTF-8

# Bring build environment up to date and install build dependencies
RUN yum -y update                        && \
    yum -y install epel-release          && \
    yum -y install $BUILD_DEPENDENCIES   && \
    yum clean all

# Add configuration scripts
COPY src/guacd-docker/bin /opt/guacd/bin/

# Copy source to container for sake of build
COPY . "$BUILD_DIR"

# Build guacamole-server from local source
RUN /opt/guacd/bin/build-guacd.sh "$BUILD_DIR" "$PREFIX_DIR"

# Use same CentOS as the base for the runtime image
FROM centos:${CENTOS_VERSION}

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
        cairo                         \
        dejavu-sans-mono-fonts        \
        freerdp                       \
        freerdp-plugins               \
        ghostscript                   \
        libjpeg-turbo                 \
        libssh2                       \
        liberation-mono-fonts         \
        libtelnet                     \
        libvorbis                     \
        libvncserver                  \
        libwebp                       \
        pango                         \
        pulseaudio-libs               \
        terminus-fonts                \
        uuid"

# Bring runtime environment up to date and install runtime dependencies
RUN yum -y update                          && \
    yum -y install epel-release            && \
    yum -y install $RUNTIME_DEPENDENCIES   && \
    yum clean all                          && \
    rm -rf /var/cache/yum

# Copy build artifacts into this stage
COPY --from=builder ${PREFIX_DIR} ${PREFIX_DIR}

# Link FreeRDP plugins into proper path
RUN FREERDP_DIR=$(dirname \
        $(rpm -ql freerdp-libs | grep 'libfreerdp.*\.so' | head -n1)) && \
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
CMD [ "/usr/local/guacamole/sbin/guacd", "-b", "0.0.0.0", "-L", "$GUACD_LOG_LEVEL", "-f" ]

