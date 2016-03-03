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

sbin_PROGRAMS = guacd

man_MANS =           \
    man/guacd.8      \
    man/guacd.conf.5

noinst_HEADERS =  \
    conf-args.h   \
    conf-file.h   \
    conf-parse.h  \
    connection.h  \
    log.h         \
    move-fd.h     \
    proc.h        \
    proc-map.h    \
    user.h

guacd_SOURCES =  \
    conf-args.c  \
    conf-file.c  \
    conf-parse.c \
    connection.c \
    daemon.c     \
    log.c        \
    move-fd.c    \
    proc.c       \
    proc-map.c   \
    user.c

guacd_CFLAGS =              \
    -Werror -Wall -pedantic \
    @COMMON_INCLUDE@        \
    @LIBGUAC_INCLUDE@

guacd_LDADD =       \
    @COMMON_LTLIB@  \
    @LIBGUAC_LTLIB@

guacd_LDFLAGS =    \
    @PTHREAD_LIBS@ \
    @SSL_LIBS@

EXTRA_DIST =         \
    init.d/guacd.in  \
    man/guacd.8      \
    man/guacd.conf.5

CLEANFILES = $(init_SCRIPTS)

# SSL support
if ENABLE_SSL
noinst_HEADERS += socket-ssl.h
guacd_SOURCES  += socket-ssl.c
endif

# Init script
if ENABLE_INIT
initdir = @init_dir@
init_SCRIPTS = init.d/guacd

init.d/guacd: init.d/guacd.in
	sed -e 's,[@]sbindir[@],$(sbindir),g' < init.d/guacd.in > init.d/guacd
	chmod +x init.d/guacd
endif

