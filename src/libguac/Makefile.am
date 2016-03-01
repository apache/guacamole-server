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

lib_LTLIBRARIES = libguac.la

libguacincdir = $(includedir)/guacamole

libguacinc_HEADERS =                  \
    guacamole/audio.h                 \
    guacamole/audio-fntypes.h         \
    guacamole/audio-types.h           \
    guacamole/client-constants.h      \
    guacamole/client.h                \
    guacamole/client-fntypes.h        \
    guacamole/client-types.h          \
    guacamole/error.h                 \
    guacamole/error-types.h           \
    guacamole/hash.h                  \
    guacamole/layer.h                 \
    guacamole/layer-types.h           \
    guacamole/object.h                \
    guacamole/object-types.h          \
    guacamole/parser-constants.h      \
    guacamole/parser.h                \
    guacamole/parser-types.h          \
    guacamole/plugin-constants.h      \
    guacamole/plugin.h                \
    guacamole/pool.h                  \
    guacamole/pool-types.h            \
    guacamole/protocol.h              \
    guacamole/protocol-types.h        \
    guacamole/socket-constants.h      \
    guacamole/socket.h                \
    guacamole/socket-fntypes.h        \
    guacamole/socket-types.h          \
    guacamole/stream.h                \
    guacamole/stream-types.h          \
    guacamole/timestamp.h             \
    guacamole/timestamp-types.h       \
    guacamole/unicode.h               \
    guacamole/user.h                  \
    guacamole/user-constants.h        \
    guacamole/user-fntypes.h          \
    guacamole/user-types.h

noinst_HEADERS =      \
    id.h              \
    encode-jpeg.h     \
    encode-png.h      \
    palette.h         \
    user-handlers.h   \
    raw_encoder.h

# Temporarily removed audio.c and raw_encoder.c for testing of others
libguac_la_SOURCES =  \
    client.c          \
    encode-jpeg.c     \
    encode-png.c      \
    error.c           \
    hash.c            \
    id.c              \
    palette.c         \
    parser.c          \
    pool.c            \
    protocol.c        \
    socket.c          \
    socket-fd.c       \
    socket-nest.c     \
    timestamp.c       \
    unicode.c         \
    user.c            \
    user-handlers.c

# Compile WebP support if available
if ENABLE_WEBP
libguac_la_SOURCES += encode-webp.c
noinst_HEADERS += encode-webp.h
endif


libguac_la_CFLAGS = \
    -Werror -Wall -pedantic -Iguacamole

libguac_la_LDFLAGS =     \
    -version-info 11:0:0 \
    @CAIRO_LIBS@         \
    @JPEG_LIBS@          \
    @PNG_LIBS@           \
    @PTHREAD_LIBS@       \
    @UUID_LIBS@          \
    @VORBIS_LIBS@        \
    @WEBP_LIBS@

libguac_la_LIBADD = \
    @LIBADD_DLOPEN@ 
