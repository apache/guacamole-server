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
    guacamole/instruction-constants.h \
    guacamole/instruction.h           \
    guacamole/instruction-types.h     \
    guacamole/layer.h                 \
    guacamole/layer-types.h           \
    guacamole/object.h                \
    guacamole/object-types.h          \
    guacamole/plugin-constants.h      \
    guacamole/plugin.h                \
    guacamole/plugin-types.h          \
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
    guacamole/unicode.h

noinst_HEADERS =      \
    client-handlers.h \
    encode-jpeg.h     \
    encode-png.h      \
    palette.h         \
    wav_encoder.h

libguac_la_SOURCES =  \
    audio.c           \
    client.c          \
    client-handlers.c \
    encode-jpeg.c     \
    encode-png.c      \
    error.c           \
    hash.c            \
    instruction.c     \
    palette.c         \
    plugin.c          \
    pool.c            \
    protocol.c        \
    socket.c          \
    socket-fd.c       \
    socket-nest.c     \
    timestamp.c       \
    unicode.c         \
    wav_encoder.c

# Compile OGG support if available
if ENABLE_OGG
libguac_la_SOURCES += ogg_encoder.c
noinst_HEADERS += ogg_encoder.h
endif

libguac_la_CFLAGS = \
    -Werror -Wall -pedantic -Iguacamole

libguac_la_LDFLAGS =    \
    -version-info 9:0:0 \
    @CAIRO_LIBS@        \
    @JPEG_LIBS@         \
    @PNG_LIBS@          \
    @PTHREAD_LIBS@      \
    @UUID_LIBS@         \
    @VORBIS_LIBS@

libguac_la_LIBADD = \
    @LIBADD_DLOPEN@ 
