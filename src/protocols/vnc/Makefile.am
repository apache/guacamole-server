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

lib_LTLIBRARIES = libguac-client-vnc.la

libguac_client_vnc_la_SOURCES = \
    auth.c                      \
    client.c                    \
    clipboard.c                 \
    cursor.c                    \
    display.c                   \
    input.c                     \
    log.c                       \
    settings.c                  \
    user.c                      \
    vnc.c
    
noinst_HEADERS =      \
    auth.h            \
    client.h          \
    clipboard.h       \
    cursor.h          \
    display.h         \
    input.h           \
    log.h             \
    settings.h        \
    user.h            \
    vnc.h

# Optional PulseAudio support
if ENABLE_PULSE
libguac_client_vnc_la_SOURCES += pulse.c
noinst_HEADERS += pulse.h
endif

libguac_client_vnc_la_CFLAGS =        \
    -Werror -Wall -pedantic -Iinclude \
    @COMMON_INCLUDE@                  \
    @COMMON_SSH_INCLUDE@              \
    @LIBGUAC_INCLUDE@

libguac_client_vnc_la_LDFLAGS = \
    -version-info 0:0:0         \
    @CAIRO_LIBS@                \
    @PULSE_LIBS@                \
    @VNC_LIBS@ 

libguac_client_vnc_la_LIBADD = \
    @COMMON_LTLIB@             \
    @LIBGUAC_LTLIB@

# Optional SFTP support
if ENABLE_COMMON_SSH
libguac_client_vnc_la_SOURCES += sftp.c
noinst_HEADERS                += sftp.h
libguac_client_vnc_la_LIBADD  += @COMMON_SSH_LTLIB@
endif

