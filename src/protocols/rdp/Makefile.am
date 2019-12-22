#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#
# NOTE: Parts of this file (Makefile.am) are automatically transcluded verbatim
# into Makefile.in. Though the build system (GNU Autotools) automatically adds
# its own license boilerplate to the generated Makefile.in, that boilerplate
# does not apply to the transcluded portions of Makefile.am which are licensed
# to you by the ASF under the Apache License, Version 2.0, as described above.
#

AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4

lib_LTLIBRARIES = libguac-client-rdp.la
SUBDIRS = . tests

#
# Main RDP client library
#

nodist_libguac_client_rdp_la_SOURCES = \
    _generated_keymaps.c

libguac_client_rdp_la_SOURCES = \
    audio_input.c               \
    channels.c                  \
    client.c                    \
    clipboard.c                 \
    decompose.c                 \
    disp.c                      \
    error.c                     \
    input.c                     \
    keyboard.c                  \
    ptr_string.c                \
    rail.c                      \
    rdp.c                       \
    rdp_bitmap.c                \
    rdp_color.c                 \
    rdp_fs.c                    \
    rdp_gdi.c                   \
    rdp_glyph.c                 \
    rdp_keymap.c                \
    rdp_print_job.c             \
    rdp_pointer.c               \
    rdp_settings.c              \
    rdp_stream.c                \
    rdpdr.c                     \
    rdpsnd.c                    \
    resolution.c                \
    svc.c                       \
    unicode.c                   \
    user.c

noinst_HEADERS =                             \
    guac_ai/ai_messages.h                    \
    guac_ai/ai_service.h                     \
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
    audio_input.h                            \
    client.h                                 \
    clipboard.h                              \
    channels.h                               \
    decompose.h                              \
    disp.h                                   \
    error.h                                  \
    input.h                                  \
    keyboard.h                               \
    ptr_string.h                             \
    rail.h                                   \
    rdp.h                                    \
    rdp_bitmap.h                             \
    rdp_color.h                              \
    rdp_fs.h                                 \
    rdp_gdi.h                                \
    rdp_glyph.h                              \
    rdp_keymap.h                             \
    rdp_pointer.h                            \
    rdp_print_job.h                          \
    rdp_settings.h                           \
    rdp_status.h                             \
    rdp_stream.h                             \
    rdpdr.h                                  \
    rdpsnd.h                                 \
    resolution.h                             \
    svc.h                                    \
    unicode.h                                \
    user.h

libguac_client_rdp_la_CFLAGS = \
    -Werror -Wall -Iinclude    \
    @COMMON_INCLUDE@           \
    @COMMON_SSH_INCLUDE@       \
    @LIBGUAC_INCLUDE@          \
    @RDP_CFLAGS@

libguac_client_rdp_la_LDFLAGS = \
    -version-info 0:0:0         \
    @CAIRO_LIBS@                \
    @PTHREAD_LIBS@              \
    @RDP_LIBS@

libguac_client_rdp_la_LIBADD =     \
    @COMMON_LTLIB@                 \
    @LIBGUAC_LTLIB@

#
# Plugins for FreeRDP
#

freerdp_LTLIBRARIES =    \
    libguacai-client.la  \
    libguacsnd-client.la \
    libguacsvc-client.la
#    libguacdr-client.la

freerdpdir = ${libdir}/freerdp2

#
# RDPDR
#

#libguacdr_client_la_SOURCES =                \
#    guac_rdpdr/rdpdr_fs_messages.c           \
#    guac_rdpdr/rdpdr_fs_messages_dir_info.c  \
#    guac_rdpdr/rdpdr_fs_messages_file_info.c \
#    guac_rdpdr/rdpdr_fs_messages_vol_info.c  \
#    guac_rdpdr/rdpdr_fs_service.c            \
#    guac_rdpdr/rdpdr_messages.c              \
#    guac_rdpdr/rdpdr_printer.c               \
#    guac_rdpdr/rdpdr_service.c               \
#    rdp_fs.c                                 \
#    rdp_print_job.c                          \
#    rdp_stream.c                             \
#    unicode.c
#
#libguacdr_client_la_CFLAGS =   \
#    -Werror -Wall -Iinclude    \
#    @COMMON_INCLUDE@           \
#    @COMMON_SSH_INCLUDE@       \
#    @LIBGUAC_INCLUDE@          \
#    @RDP_CFLAGS@
#
#libguacdr_client_la_LDFLAGS =      \
#    -module -avoid-version -shared \
#    @PTHREAD_LIBS@                 \
#    @RDP_LIBS@
#
#libguacdr_client_la_LIBADD = \
#    @COMMON_LTLIB@           \
#    @LIBGUAC_LTLIB@

#
# Audio Input
#

libguacai_client_la_SOURCES = \
    audio_input.c             \
    guac_ai/ai_messages.c     \
    guac_ai/ai_service.c      \
    ptr_string.c

libguacai_client_la_CFLAGS =   \
    -Werror -Wall -Iinclude    \
    @COMMON_INCLUDE@           \
    @COMMON_SSH_INCLUDE@       \
    @LIBGUAC_INCLUDE@          \
    @RDP_CFLAGS@

libguacai_client_la_LDFLAGS =      \
    -module -avoid-version -shared \
    @PTHREAD_LIBS@                 \
    @RDP_LIBS@

libguacai_client_la_LIBADD = \
    @COMMON_LTLIB@           \
    @LIBGUAC_LTLIB@

#
# RDPSND
#

libguacsnd_client_la_SOURCES =    \
    guac_rdpsnd/rdpsnd_messages.c \
    guac_rdpsnd/rdpsnd_service.c

libguacsnd_client_la_CFLAGS = \
    -Werror -Wall -Iinclude   \
    @COMMON_INCLUDE@          \
    @COMMON_SSH_INCLUDE@      \
    @LIBGUAC_INCLUDE@         \
    @RDP_CFLAGS@

libguacsnd_client_la_LDFLAGS =     \
    -module -avoid-version -shared \
    @PTHREAD_LIBS@                 \
    @RDP_LIBS@

libguacsnd_client_la_LIBADD = \
    @COMMON_LTLIB@            \
    @LIBGUAC_LTLIB@

#
# Static Virtual Channels
#

libguacsvc_client_la_SOURCES = \
    guac_svc/svc_service.c     \
    svc.c

libguacsvc_client_la_CFLAGS =  \
    -Werror -Wall -Iinclude    \
    @COMMON_INCLUDE@           \
    @COMMON_SSH_INCLUDE@       \
    @LIBGUAC_INCLUDE@          \
    @RDP_CFLAGS@

libguacsvc_client_la_LDFLAGS =     \
    -module -avoid-version -shared \
    @PTHREAD_LIBS@                 \
    @RDP_LIBS@

libguacsvc_client_la_LIBADD = \
    @COMMON_LTLIB@            \
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
    $(srcdir)/keymaps/base.keymap         \
    $(srcdir)/keymaps/failsafe.keymap     \
    $(srcdir)/keymaps/de_de_qwertz.keymap \
    $(srcdir)/keymaps/de_ch_qwertz.keymap \
    $(srcdir)/keymaps/en_gb_qwerty.keymap \
    $(srcdir)/keymaps/en_us_qwerty.keymap \
    $(srcdir)/keymaps/es_es_qwerty.keymap \
    $(srcdir)/keymaps/fr_fr_azerty.keymap \
    $(srcdir)/keymaps/fr_ch_qwertz.keymap \
    $(srcdir)/keymaps/it_it_qwerty.keymap \
    $(srcdir)/keymaps/ja_jp_qwerty.keymap \
    $(srcdir)/keymaps/pt_br_qwerty.keymap \
    $(srcdir)/keymaps/sv_se_qwerty.keymap \
    $(srcdir)/keymaps/da_dk_qwerty.keymap \
    $(srcdir)/keymaps/tr_tr_qwerty.keymap

_generated_keymaps.c: $(rdp_keymaps)
	$(srcdir)/keymaps/generate.pl $(rdp_keymaps)

EXTRA_DIST =            \
    $(rdp_keymaps)      \
    keymaps/generate.pl

