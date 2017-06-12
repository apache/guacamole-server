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

lib_LTLIBRARIES = libguacd.la

libguacdincdir = $(includedir)/libguacd

libguacdinc_HEADERS = \
    libguacd/user.h

noinst_HEADERS = \
    log.h

libguacd_la_SOURCES = \
    log.c             \
    user.c

libguacd_la_CFLAGS =        \
    -Werror -Wall -pedantic \
    @LIBGUAC_INCLUDE@

libguacd_la_LIBADD = \
    @LIBGUAC_LTLIB@

libguacd_la_LDFLAGS =    \
    -version-info 0:0:0  \
    -no-undefined        \
    @PTHREAD_LIBS@

