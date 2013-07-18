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
# The Original Code is libguac-client-rdp.
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
AM_CFLAGS = -Werror -Wall -Iinclude @LIBGUAC_INCLUDE@

lib_LTLIBRARIES = libguac-client-rdp.la

libguac_client_rdp_la_SOURCES = \
	audio.c                     \
	client.c                    \
	default_pointer.c           \
	guac_handlers.c             \
	rdp_bitmap.c                \
	rdp_cliprdr.c               \
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
	wav_encoder.c

guacsnd_sources =                 \
	guac_rdpsnd/rdpsnd_messages.c \
	guac_rdpsnd/rdpsnd_service.c  \
	audio.c

guacdr_sources =                \
	guac_rdpdr/rdpdr_messages.c \
	guac_rdpdr/rdpdr_printer.c  \
	guac_rdpdr/rdpdr_service.c

noinst_HEADERS =                     \
	compat/client-cliprdr.h          \
	guac_rdpdr/rdpdr_messages.h      \
	guac_rdpdr/rdpdr_printer.h       \
	guac_rdpdr/rdpdr_service.h       \
	guac_rdpsnd/rdpsnd_messages.h    \
	guac_rdpsnd/rdpsnd_service.h     \
	audio.h                          \
	client.h                         \
	config.h                         \
	default_pointer.h                \
	guac_handlers.h                  \
	rdp_bitmap.h                     \
	rdp_cliprdr.h                    \
	rdp_gdi.h                        \
	rdp_glyph.h                      \
	rdp_keymap.h                     \
	rdp_pointer.h                    \
	rdp_settings.h                   \
	wav_encoder.h

# Add compatibility layer for WinPR if not available
if ! ENABLE_WINPR
    noinst_HEADERS += compat/winpr-stream.h
    guacsnd_sources += compat/winpr-stream.c
    guacdr_sources += compat/winpr-stream.c
endif

# Compile OGG support if available
if ENABLE_OGG
    libguac_client_rdp_la_SOURCES += ogg_encoder.c
    noinst_HEADERS += ogg_encoder.h
endif

libguac_client_rdp_la_LDFLAGS = -version-info 0:0:0 @RDP_LIBS@ @VORBIS_LIBS@ @PTHREAD_LIBS@ @CAIRO_LIBS@
guacsnd_ldflags = -module -avoid-version -shared @RDP_LIBS@ @VORBIS_LIBS@ @PTHREAD_LIBS@
guacdr_ldflags = -module -avoid-version -shared @RDP_LIBS@ @PTHREAD_LIBS@

libguac_client_rdp_la_LIBADD = @LIBGUAC_LTLIB@
guacsnd_libadd = @LIBGUAC_LTLIB@
guacdr_libadd = @LIBGUAC_LTLIB@

if LEGACY_FREERDP_EXTENSIONS

# FreeRDP 1.0-style extensions
freerdp_LTLIBRARIES = guacsnd.la guacdr.la
freerdpdir = ${libdir}/freerdp

guacsnd_la_SOURCES = ${guacsnd_sources}
guacsnd_la_LDFLAGS = ${guacsnd_ldflags}
guacsnd_la_LIBADD  = ${guacsnd_libadd}

guacdr_la_SOURCES = ${guacdr_sources}
guacdr_la_LDFLAGS = ${guacdr_ldflags}
guacdr_la_LIBADD  = ${guacdr_libadd}

else

# FreeRDP 1.1 (and hopefully onward) extensions
freerdp_LTLIBRARIES = guacsnd-client.la guacdr-client.la
freerdpdir = ${libdir}/freerdp/extensions

guacsnd_client_la_SOURCES = ${guacsnd_sources}
guacsnd_client_la_LDFLAGS = ${guacsnd_ldflags}
guacsnd_client_la_LIBADD  = ${guacsnd_libadd}

guacdr_client_la_SOURCES = ${guacdr_sources}
guacdr_client_la_LDFLAGS = ${guacdr_ldflags}
guacdr_client_la_LIBADD  = ${guacdr_libadd}

endif

