#
# Copyright (C) 2013 Glyptodon LLC
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

lib_LTLIBRARIES = libguac-client-ssh.la

libguac_client_ssh_la_SOURCES = \
    client.c                    \
    clipboard.c                 \
    guac_handlers.c             \
    sftp.c                      \
    ssh_buffer.c                \
    ssh_client.c                \
    ssh_key.c

noinst_HEADERS =                \
    client.h                    \
    clipboard.h                 \
    guac_handlers.h             \
    sftp.h                      \
    ssh_buffer.h                \
    ssh_client.h                \
    ssh_key.h

# Add agent sources if enabled
if ENABLE_SSH_AGENT
libguac_client_ssh_la_SOURCES += ssh_agent.c
noinst_HEADERS += ssh_agent.h
endif

libguac_client_ssh_la_CFLAGS = -Werror -Wall -Iinclude @LIBGUAC_INCLUDE@ @TERMINAL_INCLUDE@
libguac_client_ssh_la_LIBADD = @LIBGUAC_LTLIB@ @TERMINAL_LTLIB@
libguac_client_ssh_la_LDFLAGS = -version-info 0:0:0 @SSH_LIBS@ @SSL_LIBS@ @PTHREAD_LIBS@

