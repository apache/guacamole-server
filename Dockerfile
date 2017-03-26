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

# Start from CentOS base image
FROM centos:centos7

# Environment variables
ENV \
    BUILD_DIR=/tmp/guacd-docker-BUILD \
    LC_ALL=en_US.UTF-8                \
    RUNTIME_DEPENDENCIES="            \
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
        uuid"                         \
    BUILD_DEPENDENCIES="              \
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

# Bring environment up-to-date and install guacamole-server dependencies
RUN yum -y update                        && \
    yum -y install epel-release          && \
    yum -y install $RUNTIME_DEPENDENCIES && \
    yum clean all

# Add configuration scripts
COPY src/guacd-docker/bin /opt/guacd/bin/

# Copy source to container for sake of build
COPY . "$BUILD_DIR"

# Build guacamole-server from local source
RUN yum -y install $BUILD_DEPENDENCIES         && \
    /opt/guacd/bin/build-guacd.sh "$BUILD_DIR" && \
    rm -Rf "$BUILD_DIR"                        && \
    yum -y autoremove $BUILD_DEPENDENCIES      && \
    yum clean all

# Start guacd, listening on port 0.0.0.0:4822
EXPOSE 4822
CMD [ "/usr/local/sbin/guacd", "-b", "0.0.0.0", "-f" ]

