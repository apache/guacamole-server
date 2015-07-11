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

noinst_LTLIBRARIES = libguac_terminal.la

noinst_HEADERS =                \
    blank.h                     \
    buffer.h                    \
    char_mappings.h             \
    common.h                    \
    cursor.h                    \
    display.h                   \
    ibar.h                      \
    packet.h                    \
    pointer.h                   \
    scrollbar.h                 \
    terminal.h                  \
    terminal_handlers.h         \
    types.h

libguac_terminal_la_SOURCES =   \
    blank.c                     \
    buffer.c                    \
    char_mappings.c             \
    common.c                    \
    cursor.c                    \
    display.c                   \
    ibar.c                      \
    packet.c                    \
    pointer.c                   \
    scrollbar.c                 \
    terminal.c                  \
    terminal_handlers.c

libguac_terminal_la_CFLAGS = \
    -Werror -Wall -pedantic  \
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

