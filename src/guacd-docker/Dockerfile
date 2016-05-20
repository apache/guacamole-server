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
MAINTAINER Michael Jumper <mike.jumper@guac-dev.org>

# Environment variables
ENV \
    GUAC_VERSION=0.9.9 \
    LC_ALL=en_US.UTF-8

# Bring environment up-to-date, install guacamole-server build dependencies
RUN yum -y update i             && \
    yum -y install epel-release && \
    yum -y install             \
        cairo-devel            \
        dejavu-sans-mono-fonts \
        freerdp-devel          \
        freerdp-plugins        \
        gcc                    \
        ghostscript            \
        libjpeg-turbo-devel    \
        libssh2-devel          \
        liberation-mono-fonts  \
        libtelnet-devel        \
        libvorbis-devel        \
        libvncserver-devel     \
        libwebp-devel          \
        make                   \
        pango-devel            \
        pulseaudio-libs-devel  \
        tar                    \
        terminus-fonts         \
        uuid-devel              && \
    yum clean all

# Add configuration scripts
COPY bin /opt/guacd/bin/

# Download and install latest guacamole-server
RUN /opt/guacd/bin/download-guacd.sh "$GUAC_VERSION"

# Start guacd, listening on port 0.0.0.0:4822
EXPOSE 4822
CMD [ "/usr/local/sbin/guacd", "-b", "0.0.0.0", "-f" ]

