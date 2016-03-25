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

noinst_LTLIBRARIES = libguac_common.la

noinst_HEADERS =          \
    guac_io.h             \
    guac_blank_cursor.h   \
    guac_clipboard.h      \
    guac_cursor.h         \
    guac_display.h        \
    guac_dot_cursor.h     \
    guac_ibar_cursor.h    \
    guac_iconv.h          \
    guac_json.h           \
    guac_list.h           \
    guac_pointer_cursor.h \
    guac_recording.h      \
    guac_rect.h           \
    guac_string.h         \
    guac_surface.h

libguac_common_la_SOURCES = \
    guac_io.c               \
    guac_blank_cursor.c     \
    guac_clipboard.c        \
    guac_cursor.c           \
    guac_display.c          \
    guac_dot_cursor.c       \
    guac_ibar_cursor.c      \
    guac_iconv.c            \
    guac_json.c             \
    guac_list.c             \
    guac_pointer_cursor.c   \
    guac_recording.c        \
    guac_rect.c             \
    guac_string.c           \
    guac_surface.c

libguac_common_la_CFLAGS =  \
    -Werror -Wall -pedantic \
    @LIBGUAC_INCLUDE@

libguac_common_la_LIBADD = \
    @LIBGUAC_LTLIB@

