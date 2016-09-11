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
    daemon.h          \
    default_pointer.h \
    guac_copy.h       \
    guac_cursor.h     \
    guac_display.h    \
    guac_drawable.h   \
    guac_drv.h        \
    guac_gc.h         \
    guac_image.h      \
    guac_image_text.h \
    guac_input.h      \
    guac_pixmap.h     \
    guac_poly.h       \
    guac_poly_text.h  \
    guac_protocol.h   \
    guac_rect.h       \
    guac_screen.h     \
    guac_spans.h      \
    guac_user.h       \
    guac_window.h     \
    io.h              \
    list.h            \
    log.h

guac_drv_la_SOURCES = \
    daemon.c          \
    default_pointer.c \
    guac_copy.c       \
    guac_cursor.c     \
    guac_display.c    \
    guac_drawable.c   \
    guac_drv.c        \
    guac_gc.c         \
    guac_image.c      \
    guac_image_text.c \
    guac_input.c      \
    guac_pixmap.c     \
    guac_poly.c       \
    guac_poly_text.c  \
    guac_protocol.c   \
    guac_rect.c       \
    guac_screen.c     \
    guac_spans.c      \
    guac_user.c       \
    guac_window.c     \
    io.c              \
    list.c            \
    log.c

guac_drv_la_CFLAGS =        \
    -Werror -Wall -pedantic \
    @LIBGUACD_INCLUDE@      \
    @LIBGUAC_INCLUDE@       \
    @XORG_CFLAGS@

guac_drv_la_LIBADD = \
    @LIBGUACD_LTLIB@ \
    @LIBGUAC_LTLIB@

guac_drv_la_LDFLAGS =      \
    -module -avoid-version \
    @PTHREAD_LIBS@

