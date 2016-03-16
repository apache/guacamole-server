#
# Copyright (C) 2015 Glyptodon LLC
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
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

