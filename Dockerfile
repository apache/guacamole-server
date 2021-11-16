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

ARG UBUNTU_VERSION=bionic
FROM ubuntu:${UBUNTU_VERSION} AS builder

#
# The Debian repository that should be preferred for dependencies (this will be
# added to /etc/apt/sources.list if not already present)
#
# NOTE: Due to limitations of the Docker image build process, this value is
# duplicated in an ARG in the second stage of the build.
#
ARG DEBIAN_RELEASE=bionic

# Copy latest freerdp build deb
COPY "freerdp-debs/freerdp-nightly_2.0.0_amd64.deb" "/tmp/freerdp-debs/"
COPY "freerdp-debs/freerdp-nightly-dev_2.0.0_amd64.deb" "/tmp/freerdp-debs/"

#
# Base directory for installed build artifacts.
#
# NOTE: Due to limitations of the Docker image build process, this value is
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
        libjpeg-dev                   \
        libjpeg8-dev                  \
        libjpeg-turbo8-dev            \
        libossp-uuid-dev              \
        libpango1.0-dev               \
        libpulse-dev                  \
        libssh2-1-dev                 \
        libssl-dev                    \
        libtelnet-dev                 \
        libtool                       \
        libvncserver-dev              \
        libwebsockets-dev             \
        libwebp-dev                   \
        make"

# Do not require interaction during build
ARG DEBIAN_FRONTEND=noninteractive

# Bring build environment up to date and install build dependencies
RUN apt-get update                                              && \
    apt-get install -t ${DEBIAN_RELEASE} -y $BUILD_DEPENDENCIES && \
    apt-get install -y -f /tmp/freerdp-debs/*.deb               && \
    rm -rf /var/lib/apt/lists/*

# Add configuration scripts
COPY src/guacd-docker/bin "${PREFIX_DIR}/bin/"

# Copy source to container for sake of build
COPY . "$BUILD_DIR"

# Set compile and linker options for nightly FreeRDP
ENV LDFLAGS="-fsanitize=address"
ENV RDP_CFLAGS="-I/opt/freerdp-nightly/include/freerdp2 -I/opt/freerdp-nightly/include/winpr2"
ENV RDP_LIBS="-L/opt/freerdp-nightly/lib -lfreerdp-client2 -lfreerdp2 -lwinpr2 -fsanitize=address"

# Build guacamole-server from local source
RUN ${PREFIX_DIR}/bin/build-guacd.sh "$BUILD_DIR" "$PREFIX_DIR"

# Record the packages of all runtime library dependencies
RUN ${PREFIX_DIR}/bin/list-dependencies.sh     \
        ${PREFIX_DIR}/sbin/guacd               \
        ${PREFIX_DIR}/lib/libguac-client-*.so  \
        ${PREFIX_DIR}/lib/freerdp2/*guac*.so   \
        > ${PREFIX_DIR}/DEPENDENCIES

FROM ubuntu:${UBUNTU_VERSION}

#
# The Debian repository that should be preferred for dependencies (this will be
# added to /etc/apt/sources.list if not already present)
#
# NOTE: Due to limitations of the Docker image build process, this value is
# duplicated in an ARG in the first stage of the build.
#
ARG DEBIAN_RELEASE=bionic

#
# Base directory for installed build artifacts. See also the
# CMD directive at the end of this build stage.
#
# NOTE: Due to limitations of the Docker image build process, this value is
# duplicated in an ARG in the first stage of the build.
#
ARG PREFIX_DIR=/usr/local/guacamole

# Runtime environment
ENV LC_ALL=C.UTF-8
ENV LD_LIBRARY_PATH=/opt/freerdp-nightly/lib:${PREFIX_DIR}/lib
ENV GUACD_LOG_LEVEL=info

ARG RUNTIME_DEPENDENCIES="            \
        netcat-openbsd                \
        ca-certificates               \
        ghostscript                   \
        fonts-liberation              \
        fonts-dejavu                  \
        xfonts-terminus"

# Do not require interaction during build
ARG DEBIAN_FRONTEND=noninteractive

# Copy build artifacts into this stage
COPY --from=builder ${PREFIX_DIR} ${PREFIX_DIR}

# Copy latest freerdp build deb
COPY  "freerdp-debs/freerdp-nightly_2.0.0_amd64.deb" "/tmp/freerdp-debs/"

# Bring runtime environment up to date and install runtime dependencies
RUN apt-get update                                                                                       && \
    apt-get install -t ${DEBIAN_RELEASE} -y --no-install-recommends $RUNTIME_DEPENDENCIES                && \
    apt-get install -t ${DEBIAN_RELEASE} -y --no-install-recommends $(cat "${PREFIX_DIR}"/DEPENDENCIES)  && \
    apt-get install -y --no-install-recommends -f /tmp/freerdp-debs/*.deb                                && \
    rm -rf /var/lib/apt/lists/*

# Link FreeRDP plugins into proper path
RUN ${PREFIX_DIR}/bin/link-freerdp-plugins.sh \
        ${PREFIX_DIR}/lib/freerdp2/libguac*.so

# Checks the operating status every 5 minutes with a timeout of 5 seconds
HEALTHCHECK --interval=5m --timeout=5s CMD nc -z 127.0.0.1 4822 || exit 1

# Create a new user guacd
ARG UID=1000
ARG GID=1000
RUN groupadd --gid $GID guacd
RUN useradd --system --create-home --shell /usr/sbin/nologin --uid $UID --gid $GID guacd

# Link Guac freerdp plugins into freerdp-nightly
RUN ln -s /freerdp2 /opt/freerdp-nightly/lib/freerdp2

# Run with user guacd
USER guacd

# Expose the default listener port
EXPOSE 4822

# Start guacd, listening on port 0.0.0.0:4822
#
# Note the path here MUST correspond to the value specified in the 
# PREFIX_DIR build argument.
#
CMD /usr/local/guacamole/sbin/guacd -b 0.0.0.0 -L $GUACD_LOG_LEVEL -f

