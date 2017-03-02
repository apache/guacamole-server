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

AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4

noinst_LTLIBRARIES = libguac_common_ssh.la

libguac_common_ssh_la_SOURCES = \
    buffer.c                    \
    dsa-compat.c                \
    rsa-compat.c                \
    sftp.c                      \
    ssh.c                       \
    key.c                       \
    user.c

noinst_HEADERS =            \
    common-ssh/buffer.h     \
    common-ssh/dsa-compat.h \
    common-ssh/rsa-compat.h \
    common-ssh/key.h        \
    common-ssh/sftp.h       \
    common-ssh/ssh.h        \
    common-ssh/user.h

libguac_common_ssh_la_CFLAGS = \
    -Werror -Wall -pedantic    \
    @COMMON_INCLUDE@           \
    @LIBGUAC_INCLUDE@

libguac_common_ssh_la_LIBADD = \
    @LIBGUAC_LTLIB@

libguac_common_ssh_la_LDFLAGS = \
    @PTHREAD_LIBS@              \
    @SSH_LIBS@                  \
    @SSL_LIBS@

