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

noinst_LTLIBRARIES = libguac_terminal.la

noinst_HEADERS =                 \
    terminal/buffer.h            \
    terminal/char_mappings.h     \
    terminal/common.h            \
    terminal/display.h           \
    terminal/named-colors.h      \
    terminal/palette.h           \
    terminal/scrollbar.h         \
    terminal/select.h            \
    terminal/terminal.h          \
    terminal/terminal_handlers.h \
    terminal/types.h             \
    terminal/typescript.h        \
    terminal/xparsecolor.h

libguac_terminal_la_SOURCES =   \
    buffer.c                    \
    char_mappings.c             \
    common.c                    \
    display.c                   \
    named-colors.c              \
    palette.c                   \
    scrollbar.c                 \
    select.c                    \
    terminal.c                  \
    terminal_handlers.c         \
    terminal-stdin-stream.c     \
    typescript.c                \
    xparsecolor.c

libguac_terminal_la_CFLAGS = \
    -Werror -Wall            \
    @COMMON_INCLUDE@         \
    @LIBGUAC_INCLUDE@        \
    @PANGO_CFLAGS@           \
    @PANGOCAIRO_CFLAGS@

libguac_terminal_la_LIBADD = \
    @LIBGUAC_LTLIB@

libguac_terminal_la_LDFLAGS = \
    @CAIRO_LIBS@              \
    @MATH_LIBS@               \
    @PANGO_LIBS@              \
    @PANGOCAIRO_LIBS@         \
    @PTHREAD_LIBS@

