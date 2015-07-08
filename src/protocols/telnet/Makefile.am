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

lib_LTLIBRARIES = libguac-client-telnet.la

libguac_client_telnet_la_SOURCES = \
    client.c                       \
    clipboard.c                    \
    guac_handlers.c                \
    telnet_client.c

noinst_HEADERS =                \
    client.h                    \
    clipboard.h                 \
    guac_handlers.h             \
    telnet_client.h

libguac_client_telnet_la_CFLAGS = \
    -Werror -Wall -Iinclude       \
    @LIBGUAC_INCLUDE@             \
    @TERMINAL_INCLUDE@

libguac_client_telnet_la_LIBADD = \
    @COMMON_LTLIB@                \
    @LIBGUAC_LTLIB@               \
    @TERMINAL_LTLIB@

libguac_client_telnet_la_LDFLAGS = \
    -version-info 0:0:0            \
    @PTHREAD_LIBS@                 \
    @TELNET_LIBS@

