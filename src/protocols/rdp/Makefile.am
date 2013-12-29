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
AM_CFLAGS = -Werror -Wall -Iinclude @LIBGUAC_INCLUDE@

lib_LTLIBRARIES = libguac-client-rdp.la

libguac_client_rdp_la_SOURCES = \
	client.c                    \
	default_pointer.c           \
	guac_handlers.c             \
	rdp_bitmap.c                \
	rdp_cliprdr.c               \
	rdp_fs.c                    \
	rdp_gdi.c                   \
	rdp_glyph.c                 \
	rdp_keymap_base.c           \
	rdp_keymap.c                \
	rdp_keymap_de_de.c          \
	rdp_keymap_failsafe.c       \
	rdp_keymap_fr_fr.c          \
	rdp_keymap_en_us.c          \
	rdp_pointer.c               \
	rdp_settings.c              \
	unicode.c

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
	unicode.c

noinst_HEADERS =                             \
	compat/client-cliprdr.h                  \
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
	client.h                                 \
	config.h                                 \
	debug.h                                  \
	default_pointer.h                        \
	guac_handlers.h                          \
	rdp_bitmap.h                             \
	rdp_cliprdr.h                            \
	rdp_fs.h                                 \
	rdp_gdi.h                                \
	rdp_glyph.h                              \
	rdp_keymap.h                             \
	rdp_pointer.h                            \
	rdp_settings.h                           \
	rdp_status.h                             \
	unicode.h

# Add compatibility layer for WinPR if not available
if ! ENABLE_WINPR
    noinst_HEADERS  += compat/winpr-stream.h compat/winpr-wtypes.h
    guacsnd_sources += compat/winpr-stream.c
    guacdr_sources  += compat/winpr-stream.c
endif

libguac_client_rdp_la_LDFLAGS = -version-info 0:0:0 @RDP_LIBS@ @PTHREAD_LIBS@ @CAIRO_LIBS@
guacsnd_ldflags = -module -avoid-version -shared @RDP_LIBS@ @PTHREAD_LIBS@
guacdr_ldflags = -module -avoid-version -shared @RDP_LIBS@ @PTHREAD_LIBS@

libguac_client_rdp_la_LIBADD = @LIBGUAC_LTLIB@
guacsnd_libadd = @LIBGUAC_LTLIB@
guacdr_libadd = @LIBGUAC_LTLIB@

if LEGACY_FREERDP_EXTENSIONS

# FreeRDP 1.0-style extensions
freerdp_LTLIBRARIES = guacsnd.la guacdr.la

guacsnd_la_SOURCES = ${guacsnd_sources}
guacsnd_la_LDFLAGS = ${guacsnd_ldflags}
guacsnd_la_LIBADD  = ${guacsnd_libadd}

guacdr_la_SOURCES = ${guacdr_sources}
guacdr_la_LDFLAGS = ${guacdr_ldflags}
guacdr_la_LIBADD  = ${guacdr_libadd}

else

# FreeRDP 1.1 (and hopefully onward) extensions
freerdp_LTLIBRARIES = guacsnd-client.la guacdr-client.la

guacsnd_client_la_SOURCES = ${guacsnd_sources}
guacsnd_client_la_LDFLAGS = ${guacsnd_ldflags}
guacsnd_client_la_LIBADD  = ${guacsnd_libadd}

guacdr_client_la_SOURCES = ${guacdr_sources}
guacdr_client_la_LDFLAGS = ${guacdr_ldflags}
guacdr_client_la_LIBADD  = ${guacdr_libadd}

endif

freerdpdir = ${libdir}/freerdp

