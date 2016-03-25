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

TESTS = test_libguac
check_PROGRAMS = test_libguac

noinst_HEADERS =          \
    client/client_suite.h \
    common/common_suite.h \
    protocol/suite.h      \
    util/util_suite.h

test_libguac_SOURCES =           \
    test_libguac.c               \
    client/client_suite.c        \
    client/buffer_pool.c         \
    client/layer_pool.c          \
    common/common_suite.c        \
    common/guac_iconv.c          \
    common/guac_string.c         \
    common/guac_rect.c           \
    protocol/suite.c             \
    protocol/base64_decode.c     \
    protocol/instruction_parse.c \
    protocol/instruction_read.c  \
    protocol/instruction_write.c \
    protocol/nest_write.c        \
    util/util_suite.c            \
    util/guac_pool.c             \
    util/guac_unicode.c

test_libguac_CFLAGS =       \
    -Werror -Wall -pedantic \
    @COMMON_INCLUDE@        \
    @LIBGUAC_INCLUDE@

test_libguac_LDADD = \
    @COMMON_LTLIB@   \
    @CUNIT_LIBS@     \
    @LIBGUAC_LTLIB@

