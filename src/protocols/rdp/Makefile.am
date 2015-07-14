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

lib_LTLIBRARIES = libguac-client-rdp.la

libguac_client_rdp_la_SOURCES = \
    _generated_keymaps.c        \
    client.c                    \
    guac_handlers.c             \
    rdp_bitmap.c                \
    rdp_cliprdr.c               \
    rdp_color.c                 \
    rdp_fs.c                    \
    rdp_gdi.c                   \
    rdp_glyph.c                 \
    rdp_keymap.c                \
    rdp_pointer.c               \
    rdp_rail.c                  \
    rdp_settings.c              \
    rdp_stream.c                \
    rdp_svc.c                   \
    resolution.c                \
    unicode.c

guacsvc_sources =          \
    guac_svc/svc_service.c \
    rdp_svc.c

guacsnd_sources =                 \
    guac_rdpsnd/rdpsnd_messages.c \
    guac_rdpsnd/rdpsnd_service.c

guacdr_sources =                             \
    guac_rdpdr/rdpdr_fs_messages.c           \
    guac_rdpdr/rdpdr_fs_messages_dir_info.c  \
    guac_rdpdr/rdpdr_fs_messages_file_info.c \
    guac_rdpdr/rdpdr_fs_messages_vol_info.c  \
    guac_rdpdr/rdpdr_fs_service.c            \
    guac_rdpdr/rdpdr_messages.c              \
    guac_rdpdr/rdpdr_printer.c               \
    guac_rdpdr/rdpdr_service.c               \
    rdp_fs.c                                 \
    rdp_stream.c                             \
    unicode.c

noinst_HEADERS =                             \
    compat/client-cliprdr.h                  \
    compat/rail.h                            \
    guac_rdpdr/rdpdr_fs_messages.h           \
    guac_rdpdr/rdpdr_fs_messages_dir_info.h  \
    guac_rdpdr/rdpdr_fs_messages_file_info.h \
    guac_rdpdr/rdpdr_fs_messages_vol_info.h  \
    guac_rdpdr/rdpdr_fs_service.h            \
    guac_rdpdr/rdpdr_messages.h              \
    guac_rdpdr/rdpdr_printer.h               \
    guac_rdpdr/rdpdr_service.h               \
    guac_rdpsnd/rdpsnd_messages.h            \
    guac_rdpsnd/rdpsnd_service.h             \
    guac_svc/svc_service.h                   \
    client.h                                 \
    guac_handlers.h                          \
    rdp_bitmap.h                             \
    rdp_cliprdr.h                            \
    rdp_color.h                              \
    rdp_fs.h                                 \
    rdp_gdi.h                                \
    rdp_glyph.h                              \
    rdp_keymap.h                             \
    rdp_pointer.h                            \
    rdp_rail.h                               \
    rdp_settings.h                           \
    rdp_status.h                             \
    rdp_stream.h                             \
    rdp_svc.h                                \
    resolution.h                             \
    unicode.h

# Add compatibility layer for WinPR if not available
if ! ENABLE_WINPR
noinst_HEADERS  += compat/winpr-stream.h compat/winpr-wtypes.h
libguac_client_rdp_la_SOURCES += compat/winpr-stream.c
guacsvc_sources += compat/winpr-stream.c
guacsnd_sources += compat/winpr-stream.c
guacdr_sources  += compat/winpr-stream.c
endif

# Add display update channel support, if supported by FreeRDP
if ENABLE_DISPLAY_UPDATE
noinst_HEADERS += rdp_disp.h
libguac_client_rdp_la_SOURCES += rdp_disp.c
endif

#
# Main RDP client library
#

libguac_client_rdp_la_CFLAGS = \
    -Werror -Wall -Iinclude    \
    @COMMON_INCLUDE@           \
    @COMMON_SSH_INCLUDE@       \
    @LIBGUAC_INCLUDE@

libguac_client_rdp_la_LDFLAGS = \
    -version-info 0:0:0         \
    @CAIRO_LIBS@                \
    @PTHREAD_LIBS@              \
    @RDP_LIBS@

libguac_client_rdp_la_LIBADD =     \
    @COMMON_LTLIB@                 \
    @LIBGUAC_LTLIB@

#
# RDPDR
#

guacdr_cflags                = \
    -Werror -Wall -Iinclude    \
    @COMMON_INCLUDE@           \
    @COMMON_SSH_INCLUDE@       \
    @LIBGUAC_INCLUDE@

guacdr_ldflags =                   \
    -module -avoid-version -shared \
    @PTHREAD_LIBS@                 \
    @RDP_LIBS@

guacdr_libadd =     \
    @COMMON_LTLIB@  \
    @LIBGUAC_LTLIB@

#
# RDPSND
#

guacsnd_cflags               = \
    -Werror -Wall -Iinclude    \
    @COMMON_INCLUDE@           \
    @COMMON_SSH_INCLUDE@       \
    @LIBGUAC_INCLUDE@

guacsnd_ldflags =                  \
    -module -avoid-version -shared \
    @PTHREAD_LIBS@                 \
    @RDP_LIBS@

guacsnd_libadd =    \
    @COMMON_LTLIB@  \
    @LIBGUAC_LTLIB@

#
# Static Virtual Channels
#

guacsvc_cflags               = \
    -Werror -Wall -Iinclude    \
    @COMMON_INCLUDE@           \
    @COMMON_SSH_INCLUDE@       \
    @LIBGUAC_INCLUDE@

guacsvc_ldflags =                  \
    -module -avoid-version -shared \
    @PTHREAD_LIBS@                 \
    @RDP_LIBS@

guacsvc_libadd =    \
    @COMMON_LTLIB@  \
    @LIBGUAC_LTLIB@

#
# Optional SFTP support
#

if ENABLE_COMMON_SSH
libguac_client_rdp_la_SOURCES += sftp.c
noinst_HEADERS                += sftp.h
libguac_client_rdp_la_LIBADD  += @COMMON_SSH_LTLIB@
endif

#
# Autogenerate keymaps
#

CLEANFILES = _generated_keymaps.c
BUILT_SOURCES = _generated_keymaps.c

rdp_keymaps =                   \
    keymaps/base.keymap         \
    keymaps/failsafe.keymap     \
    keymaps/de_de_qwertz.keymap \
    keymaps/en_us_qwerty.keymap \
    keymaps/fr_fr_azerty.keymap \
    keymaps/it_it_qwerty.keymap \
    keymaps/sv_se_qwerty.keymap

_generated_keymaps.c: $(rdp_keymaps)
	keymaps/generate.pl $(rdp_keymaps)

EXTRA_DIST =            \
    $(rdp_keymaps)      \
    keymaps/generate.pl

if LEGACY_FREERDP_EXTENSIONS

# FreeRDP 1.0-style extensions
freerdp_LTLIBRARIES = \
    guacdr.la         \
    guacsnd.la        \
    guacsvc.la

guacdr_la_SOURCES = ${guacdr_sources}
guacdr_la_CFLAGS  = ${guacdr_cflags}
guacdr_la_LDFLAGS = ${guacdr_ldflags}
guacdr_la_LIBADD  = ${guacdr_libadd}

guacsnd_la_SOURCES = ${guacsnd_sources}
guacsnd_la_CFLAGS  = ${guacsnd_cflags}
guacsnd_la_LDFLAGS = ${guacsnd_ldflags}
guacsnd_la_LIBADD  = ${guacsnd_libadd}

guacsvc_la_SOURCES = ${guacsvc_sources}
guacsvc_la_CFLAGS  = ${guacsvc_cflags}
guacsvc_la_LDFLAGS = ${guacsvc_ldflags}
guacsvc_la_LIBADD  = ${guacsvc_libadd}

else

# FreeRDP 1.1 (and hopefully onward) extensions
freerdp_LTLIBRARIES = \
    guacdr-client.la  \
    guacsnd-client.la \
    guacsvc-client.la

guacdr_client_la_SOURCES = ${guacdr_sources}
guacdr_client_la_CFLAGS  = ${guacdr_cflags}
guacdr_client_la_LDFLAGS = ${guacdr_ldflags}
guacdr_client_la_LIBADD  = ${guacdr_libadd}

guacsnd_client_la_SOURCES = ${guacsnd_sources}
guacsnd_client_la_CFLAGS  = ${guacsnd_cflags}
guacsnd_client_la_LDFLAGS = ${guacsnd_ldflags}
guacsnd_client_la_LIBADD  = ${guacsnd_libadd}

guacsvc_client_la_SOURCES = ${guacsvc_sources}
guacsvc_client_la_CFLAGS  = ${guacsvc_cflags}
guacsvc_client_la_LDFLAGS = ${guacsvc_ldflags}
guacsvc_client_la_LIBADD  = ${guacsvc_libadd}

endif

freerdpdir = ${libdir}/freerdp

