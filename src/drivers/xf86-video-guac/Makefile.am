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

guac_drv_la_LTLIBRARIES = guac_drv.la

guac_drv_ladir = @XORG_MODULE_DIR@/drivers

noinst_HEADERS =      \
    copy.h            \
    cursor.h          \
    daemon.h          \
    display.h         \
    drawable.h        \
    drv.h             \
    gc.h              \
    glyph.h           \
    image.h           \
    input.h           \
    io.h              \
    log.h             \
    pixmap.h          \
    poly.h            \
    rect.h            \
    screen.h          \
    spans.h           \
    user.h            \
    window.h

guac_drv_la_SOURCES = \
    copy.c            \
    cursor.c          \
    daemon.c          \
    display.c         \
    drawable.c        \
    drv.c             \
    gc.c              \
    glyph.c           \
    image.c           \
    input.c           \
    io.c              \
    log.c             \
    pixmap.c          \
    poly.c            \
    rect.c            \
    screen.c          \
    spans.c           \
    user.c            \
    window.c

guac_drv_la_CFLAGS =        \
    -Werror -Wall -pedantic \
    @COMMON_INCLUDE@        \
    @LIBGUACD_INCLUDE@      \
    @LIBGUAC_INCLUDE@       \
    @XORG_CFLAGS@

guac_drv_la_LIBADD = \
    @LIBGUACD_LTLIB@ \
    @LIBGUAC_LTLIB@

guac_drv_la_LDFLAGS =      \
    -module -avoid-version \
    @PTHREAD_LIBS@

