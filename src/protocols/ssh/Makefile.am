# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is libguac-client-ssh.
#
# The Initial Developer of the Original Code is
# Michael Jumper.
# Portions created by the Initial Developer are Copyright (C) 2011
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****

AUTOMAKE_OPTIONS = foreign

ACLOCAL_AMFLAGS = -I m4

lib_LTLIBRARIES = libguac-client-ssh.la

libguac_client_ssh_la_SOURCES = \
    blank.c                     \
    buffer.c                    \
    char_mappings.c             \
    client.c                    \
    common.c                    \
    cursor.c                    \
    display.c                   \
    guac_handlers.c             \
    ibar.c                      \
    sftp.c                      \
    ssh_buffer.c                \
    ssh_client.c                \
    ssh_key.c                   \
    terminal.c                  \
    terminal_handlers.c

noinst_HEADERS =                \
    blank.h                     \
    buffer.h                    \
    char_mappings.h             \
    client.h                    \
    common.h                    \
    cursor.h                    \
    display.h                   \
    guac_handlers.h             \
    ibar.h                      \
    sftp.h                      \
    ssh_buffer.h                \
    ssh_client.h                \
    ssh_key.h                   \
    terminal.h                  \
    terminal_handlers.h         \
    types.h

libguac_client_ssh_la_CFLAGS = -Werror -Wall -pedantic -Iinclude @PANGO_CFLAGS@ @PANGOCAIRO_CFLAGS@ @LIBGUAC_INCLUDE@
libguac_client_ssh_la_LIBADD = @LIBGUAC_LTLIB@
libguac_client_ssh_la_LDFLAGS = -version-info 0:0:0 @SSH_LIBS@ @SSL_LIBS@ @PTHREAD_LIBS@ @PANGO_LIBS@ @PANGOCAIRO_LIBS@ @CAIRO_LIBS@

