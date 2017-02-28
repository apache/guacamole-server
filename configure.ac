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

AC_PREREQ([2.61])
AC_INIT([guacamole-server], [0.9.12-incubating])
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])
AM_SILENT_RULES([yes])

LT_PREREQ([2.2])
LT_INIT([dlopen])

AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])

# Programs
AC_PROG_CC
AC_PROG_CC_C99
AC_PROG_LIBTOOL

# Headers
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h sys/socket.h time.h sys/time.h syslog.h unistd.h cairo/cairo.h pngstruct.h])

# Source characteristics
AC_DEFINE([_XOPEN_SOURCE], [700], [Uses X/Open and POSIX APIs])

# Check for whether math library is required
AC_CHECK_LIB([m], [cos],
             [MATH_LIBS=-lm],
             [AC_CHECK_DECL([cos],,
                            AC_MSG_ERROR("Complex math functions are missing and no libm was found")
                            [#include <math.h>])])

# libpng
AC_CHECK_LIB([png], [png_write_png], [PNG_LIBS=-lpng],
             AC_MSG_ERROR("libpng is required for writing png messages"))

# libjpeg
AC_CHECK_LIB([jpeg], [jpeg_start_compress], [JPEG_LIBS=-ljpeg],
             AC_MSG_ERROR("libjpeg is required for writing jpeg messages"))

# Cairo
AC_CHECK_LIB([cairo], [cairo_create], [CAIRO_LIBS=-lcairo],
             AC_MSG_ERROR("Cairo is required for drawing instructions"))

# libpthread
AC_CHECK_LIB([pthread], [pthread_create], [PTHREAD_LIBS=-lpthread
              AC_DEFINE([HAVE_LIBPTHREAD],,
                        [Whether libpthread was found])])

# OSSP UUID
AC_CHECK_LIB([ossp-uuid], [uuid_make], [UUID_LIBS=-lossp-uuid],
             AC_CHECK_LIB([uuid], [uuid_make], [UUID_LIBS=-luuid],
                          AC_MSG_ERROR("The OSSP UUID library is required")))

# Check for and validate OSSP uuid.h header
AC_CHECK_HEADERS([ossp/uuid.h])
AC_CHECK_DECL([uuid_make],,
              AC_MSG_ERROR("No OSSP uuid.h found in include path"),
              [#ifdef HAVE_OSSP_UUID_H
               #include <ossp/uuid.h>
               #else
               #include <uuid.h>
               #endif
               ])

# cunit
AC_CHECK_LIB([cunit], [CU_run_test], [CUNIT_LIBS=-lcunit])

# WinSock
AC_CHECK_LIB([wsock32], [main])

AC_SUBST(LIBADD_DLOPEN)
AC_SUBST(MATH_LIBS)
AC_SUBST(PNG_LIBS)
AC_SUBST(JPEG_LIBS)
AC_SUBST(CAIRO_LIBS)
AC_SUBST(PTHREAD_LIBS)
AC_SUBST(UUID_LIBS)
AC_SUBST(CUNIT_LIBS)

# Library functions
AC_CHECK_FUNCS([clock_gettime gettimeofday memmove memset select strdup nanosleep])

AC_CHECK_DECL([png_get_io_ptr],
	[AC_DEFINE([HAVE_PNG_GET_IO_PTR],,
               [Whether png_get_io_ptr() is defined])],,
	[#include <png.h>])

AC_CHECK_DECL([cairo_format_stride_for_width],
	[AC_DEFINE([HAVE_CAIRO_FORMAT_STRIDE_FOR_WIDTH],,
               [Whether cairo_format_stride_for_width() is defined])],,
	[#include <cairo/cairo.h>])

# Typedefs
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

# Bundled libguac
AC_SUBST([LIBGUAC_LTLIB],   '$(top_builddir)/src/libguac/libguac.la')
AC_SUBST([LIBGUAC_INCLUDE], '-I$(top_srcdir)/src/libguac')

# Common non-libguac utility library
AC_SUBST([COMMON_LTLIB],   '$(top_builddir)/src/common/libguac_common.la')
AC_SUBST([COMMON_INCLUDE], '-I$(top_srcdir)/src/common')

# Common PulseAudio utility library
AC_SUBST([PULSE_LTLIB],   '$(top_builddir)/src/pulse/libguac_pulse.la')
AC_SUBST([PULSE_INCLUDE], '-I$(top_srcdir)/src/pulse')

# Common utility library for guacd implementations
AC_SUBST([LIBGUACD_LTLIB],   '$(top_builddir)/src/libguacd/libguacd.la')
AC_SUBST([LIBGUACD_INCLUDE], '-I$(top_srcdir)/src/libguacd')

# Common base SSH client
AC_SUBST([COMMON_SSH_LTLIB],   '$(top_builddir)/src/common-ssh/libguac_common_ssh.la')
AC_SUBST([COMMON_SSH_INCLUDE], '-I$(top_srcdir)/src/common-ssh')

# Terminal emulator
AC_SUBST([TERMINAL_LTLIB],   '$(top_builddir)/src/terminal/libguac_terminal.la')
AC_SUBST([TERMINAL_INCLUDE], '-I$(top_srcdir)/src/terminal $(PANGO_CFLAGS) $(PANGOCAIRO_CFLAGS) $(COMMON_INCLUDE)')

# Init directory
AC_ARG_WITH(init_dir,
            [AS_HELP_STRING([--with-init-dir=<path>],
                            [install init scripts to the given directory])
            ],init_dir=$withval)
AM_CONDITIONAL([ENABLE_INIT], [test "x${init_dir}" != "x"])
AC_SUBST(init_dir)

# guacd config file
AC_ARG_WITH(guacd_conf,
            [AS_HELP_STRING([--with-guacd-conf=<path>],
                            [the full path to the guacd config file @<:@default=/etc/guacamole/guacd.conf@:>@])],
            [guacd_conf=$withval],
            [guacd_conf=/etc/guacamole/guacd.conf])
AC_DEFINE_UNQUOTED([GUACD_CONF_FILE], ["$guacd_conf"], [The full path to the guacd config file])

#
# libavcodec
#

have_libavcodec=disabled
AC_ARG_WITH([libavcodec],
            [AS_HELP_STRING([--with-libavcodec],
                            [use libavcodec when encoding video @<:@default=check@:>@])],
            [],
            [with_libavcodec=check])

if test "x$with_libavcodec" != "xno"
then
    have_libavcodec=yes
    PKG_CHECK_MODULES([AVCODEC], [libavcodec],, [have_libavcodec=no]);
fi

AM_CONDITIONAL([ENABLE_AVCODEC], [test "x${have_libavcodec}" = "xyes"])

#
# libavutil
#

have_libavutil=disabled
AC_ARG_WITH([libavutil],
            [AS_HELP_STRING([--with-libavutil],
                            [use libavutil when encoding video @<:@default=check@:>@])],
            [],
            [with_libavutil=check])

if test "x$with_libavutil" != "xno"
then
    have_libavutil=yes
    PKG_CHECK_MODULES([AVUTIL], [libavutil],, [have_libavutil=no]);
fi

AM_CONDITIONAL([ENABLE_AVUTIL], [test "x${have_libavutil}" = "xyes"])

#
# libswscale
#

have_libswscale=disabled
AC_ARG_WITH([libswscale],
            [AS_HELP_STRING([--with-libswscale],
                            [use libswscale when encoding video @<:@default=check@:>@])],
            [],
            [with_libswscale=check])

if test "x$with_libswscale" != "xno"
then
    have_libswscale=yes
    PKG_CHECK_MODULES([SWSCALE], [libswscale],, [have_libswscale=no]);
fi

AM_CONDITIONAL([ENABLE_SWSCALE], [test "x${have_libswscale}" = "xyes"])

#
# libssl
#

have_ssl=disabled
SSL_LIBS=
AC_ARG_WITH([ssl],
            [AS_HELP_STRING([--with-ssl],
                            [support SSL encryption @<:@default=check@:>@])],
            [],
            [with_ssl=check])

if test "x$with_ssl" != "xno"
then
    have_ssl=yes

    AC_CHECK_HEADER(openssl/ssl.h,, [have_ssl=no])
    AC_CHECK_LIB([ssl], [SSL_CTX_new], [SSL_LIBS="$SSL_LIBS -lssl -lcrypto"], [have_ssl=no])

    if test "x${have_ssl}" = "xno"
    then
        AC_MSG_WARN([
  --------------------------------------------
   Unable to find libssl.
   guacd will not support SSL connections.
  --------------------------------------------])
    else
        AC_DEFINE([ENABLE_SSL],, [Whether SSL-related support is enabled])
    fi
fi

AM_CONDITIONAL([ENABLE_SSL], [test "x${have_ssl}" = "xyes"])
AC_SUBST(SSL_LIBS)


#
# Ogg Vorbis
#

have_vorbis=disabled
VORBIS_LIBS=
AC_ARG_WITH([vorbis],
            [AS_HELP_STRING([--with-vorbis],
                            [support Ogg Vorbis @<:@default=check@:>@])],
            [],
            [with_vorbis=check])

if test "x$with_vorbis" != "xno"
then
    have_vorbis=yes

    AC_CHECK_HEADER(vorbis/vorbisenc.h,, [have_vorbis=no])
    AC_CHECK_LIB([ogg], [ogg_stream_init], [VORBIS_LIBS="$VORBIS_LIBS -logg"], [have_vorbis=no])
    AC_CHECK_LIB([vorbis], [vorbis_block_init], [VORBIS_LIBS="$VORBIS_LIBS -lvorbis"], [have_vorbis=no])
    AC_CHECK_LIB([vorbisenc], [vorbis_encode_init], [VORBIS_LIBS="$VORBIS_LIBS -lvorbisenc"], [have_vorbis=no])

    if test "x${have_vorbis}" = "xno"
    then
        AC_MSG_WARN([
  --------------------------------------------
   Unable to find libogg / libvorbis / libvorbisenc.
   Sound will not be encoded with Ogg Vorbis.
  --------------------------------------------])
    else
        AC_DEFINE([ENABLE_OGG],,
                  [Whether support for Ogg Vorbis is enabled])
    fi
fi

AM_CONDITIONAL([ENABLE_OGG], [test "x${have_vorbis}" = "xyes"])
AC_SUBST(VORBIS_LIBS)

#
# PulseAudio
#

have_pulse=disabled
PULSE_LIBS=
AC_ARG_WITH([pulse],
            [AS_HELP_STRING([--with-pulse],
                            [support PulseAudio @<:@default=check@:>@])],
            [],
            [with_pulse=check])

if test "x$with_pulse" != "xno"
then
    have_pulse=yes

    AC_CHECK_LIB([pulse], [pa_context_new], [PULSE_LIBS="$PULSE_LIBS -lpulse"], [have_pulse=no])

    if test "x${have_pulse}" = "xno"
    then
        AC_MSG_WARN([
  --------------------------------------------
   Unable to find libpulse
   Sound support for VNC will be disabled
  --------------------------------------------])
    else
        AC_DEFINE([ENABLE_PULSE],, [Whether PulseAudio support is enabled])
    fi
fi

AM_CONDITIONAL([ENABLE_PULSE], [test "x${have_pulse}" = "xyes"])
AC_SUBST(PULSE_LIBS)

#
# PANGO
#

have_pango=disabled
AC_ARG_WITH([pango],
            [AS_HELP_STRING([--with-pango],
                            [support Pango text layout @<:@default=check@:>@])],
            [],
            [with_pango=check])

if test "x$with_pango" != "xno"
then
    have_pango=yes
    PKG_CHECK_MODULES([PANGO], [pango],, [have_pango=no]);
    PKG_CHECK_MODULES([PANGOCAIRO], [pangocairo],, [have_pango=no]);
fi

#
# Terminal emulator
#

have_terminal=disabled
AC_ARG_WITH([terminal],
            [AS_HELP_STRING([--with-terminal],
                            [support text-based protocols @<:@default=check@:>@])],
            [],
            [with_terminal=check])

if test "x$with_terminal" != "xno"
then
    have_terminal=yes
    if test "x${have_pango}" = "xno"
    then
        have_terminal=no
    fi
fi

AM_CONDITIONAL([ENABLE_TERMINAL], [test "x${have_terminal}" = "xyes"])

#
# libVNCServer
#

have_libvncserver=disabled
VNC_LIBS=
AC_ARG_WITH([vnc],
            [AS_HELP_STRING([--with-vnc],
                            [support VNC @<:@default=check@:>@])],
            [],
            [with_vnc=check])

if test "x$with_vnc" != "xno"
then
    have_libvncserver=yes
    AC_CHECK_LIB([vncclient], [rfbInitClient], [VNC_LIBS="$VNC_LIBS -lvncclient"], [have_libvncserver=no])
fi

AM_CONDITIONAL([ENABLE_VNC], [test "x${have_libvncserver}" = "xyes"])
AC_SUBST(VNC_LIBS)

#
# Repeater support within libVNCServer
#

if test "x${have_libvncserver}" = "xyes"
then

    have_vnc_repeater=yes
    AC_CHECK_MEMBERS([rfbClient.destHost, rfbClient.destPort],
                     [], [have_vnc_repeater=no],
                      [[#include <rfb/rfbclient.h>]])

    if test "x${have_vnc_repeater}" = "xno"
    then
        AC_MSG_WARN([
      --------------------------------------------
       No repeater support found in libvncclient.
       Support for VNC repeaters will not be built.
      --------------------------------------------])
    else
        AC_DEFINE([ENABLE_VNC_REPEATER],,
                  [Whether support for VNC repeaters is enabled.])
    fi

fi

#
# Listening support within libVNCServer
#

if test "x${have_libvncserver}" = "xyes"
then

    have_vnc_listen=yes
    AC_CHECK_DECL([listenForIncomingConnectionsNoFork],
                  [], [have_vnc_listen=no],
                  [[#include <rfb/rfbclient.h>]])

    if test "x${have_vnc_listen}" = "xno"
    then
        AC_MSG_WARN([
      --------------------------------------------
       No listening support found in libvncclient.
       Support for listen-mode connections will not be built.
      --------------------------------------------])
    else
        AC_DEFINE([ENABLE_VNC_LISTEN],,
                  [Whether support for listen-mode VNC connections is enabled.])
    fi

fi

#
# FreeRDP
#

have_freerdp=disabled
RDP_LIBS=
AC_ARG_WITH([rdp],
            [AS_HELP_STRING([--with-rdp],
                            [support RDP @<:@default=check@:>@])],
            [],
            [with_rdp=check])

if test "x$with_rdp" != "xno"
then
    have_winpr=yes
    have_freerdp=yes
    legacy_freerdp_extensions=no
    rdpsettings_interface=unknown
    rdpsettings_audiocapture=yes
    rdpsettings_audioplayback=yes
    rdpsettings_deviceredirection=yes
    freerdp_interface=unknown
    event_interface=unknown

    # libfreerdp-core / libfreerdp
    AC_CHECK_LIB([freerdp-core], [freerdp_new],
                 [RDP_LIBS="$RDP_LIBS -lfreerdp-core"],
                 [AC_CHECK_LIB([freerdp], [freerdp_new],
                               [RDP_LIBS="$RDP_LIBS -lfreerdp -lfreerdp-client"],
                               [AC_MSG_WARN([
  --------------------------------------------
   Unable to find libfreerdp-core / libfreerdp
   RDP will be disabled.
  --------------------------------------------])
                  have_freerdp=no])])
fi


# libfreerdp-cache
if test "x${have_freerdp}" = "xyes"
then
    AC_CHECK_LIB([freerdp-cache], [glyph_cache_register_callbacks],
                 [RDP_LIBS="$RDP_LIBS -lfreerdp-cache"])
fi

# libfreerdp-channels (1.0) / libfreerdp-client + libfreerdp-core (1.1)
if test "x${have_freerdp}" = "xyes"
then
    AC_CHECK_LIB([freerdp-client], [freerdp_channels_new],
                 [RDP_LIBS="$RDP_LIBS -lfreerdp-client"],
                 [AC_CHECK_LIB([freerdp-channels], [freerdp_channels_new],
                               [RDP_LIBS="$RDP_LIBS -lfreerdp-channels"
                               legacy_freerdp_extensions=yes])],
                 [-lfreerdp-core])
fi

# libfreerdp-utils
if test "x${have_freerdp}" = "xyes"
then
    AC_CHECK_LIB([freerdp-utils], [svc_plugin_init],
                 [RDP_LIBS="$RDP_LIBS -lfreerdp-utils"])
fi

# libfreerdp-codec
if test "x${have_freerdp}" = "xyes"
then
    AC_CHECK_LIB([freerdp-codec], [freerdp_image_convert],
                 [RDP_LIBS="$RDP_LIBS -lfreerdp-codec"])
fi

# Available color conversion functions
if test "x${have_freerdp}" = "xyes"
then
    AC_CHECK_DECL([freerdp_convert_gdi_order_color],
        [AC_DEFINE([HAVE_FREERDP_CONVERT_GDI_ORDER_COLOR],,
                   [Whether freerdp_convert_gdi_order_color() is defined])],,
        [#include <freerdp/codec/color.h>])

    AC_CHECK_DECL([freerdp_color_convert_drawing_order_color_to_gdi_color],
        [AC_DEFINE([HAVE_FREERDP_COLOR_CONVERT_DRAWING_ORDER_COLOR_TO_GDI_COLOR],,
                   [Whether freerdp_color_convert_drawing_order_color_to_gdi_color() is defined])],,
        [#include <freerdp/codec/color.h>])
fi

# Check for interval polling in plugins
if test "x${have_freerdp}" = "xyes"
then
    AC_CHECK_MEMBERS([rdpSvcPlugin.interval_ms],,,
                     [[#include <freerdp/utils/svc_plugin.h>]])
fi

# Keyboard layout header
if test "x${have_freerdp}" = "xyes"
then
    AC_CHECK_HEADERS([freerdp/locale/keyboard.h],,
                      [AC_CHECK_HEADERS([freerdp/kbd/layouts.h],,
                                       [AC_MSG_WARN([
  --------------------------------------------
   Unable to find keyboard layout headers
   RDP will be disabled.
  --------------------------------------------])
                                        have_freerdp=no])])
fi

# New headers defining addins
if test "x${have_freerdp}" = "xyes"
then
    AC_CHECK_HEADERS([freerdp/addin.h freerdp/client/channels.h])
fi

# Header defining cliprdr
if test "x${have_freerdp}" = "xyes"
then
    AC_CHECK_HEADERS([freerdp/client/cliprdr.h],,
                     [AC_CHECK_HEADERS([freerdp/plugins/cliprdr.h],,
                                       [AC_MSG_WARN([
  --------------------------------------------
   Unable to find cliprdr headers
   RDP will be disabled.
  --------------------------------------------])
                                        have_freerdp=no],
                                       [#include <freerdp/types.h>])],
                     [#include <winpr/wtypes.h>
                      #include <winpr/collections.h>])
fi

# Header defining display update channel
if test "x${have_freerdp}" = "xyes"
then
    AC_CHECK_HEADERS([freerdp/client/disp.h],
                     [AC_DEFINE([HAVE_FREERDP_DISPLAY_UPDATE_SUPPORT],,
                                [Whether FreeRDP supports the display update channel])]
                     [AC_CHECK_MEMBERS([rdpSettings.SupportDisplayControl],,,
                                       [[#include <freerdp/freerdp.h>]])],,
                     [#include <winpr/wtypes.h>
                      #include <winpr/collections.h>])
fi

# Support for "PubSub" event system
if test "x${have_freerdp}" = "xyes"
then
    AC_CHECK_DECL([PubSub_SubscribeChannelConnected],
        [AC_DEFINE([HAVE_FREERDP_EVENT_PUBSUB],,
                   [Whether this version of FreeRDP provides the PubSub event system])],,
        [#include <freerdp/event.h>])
fi

# Addin registration variations
if test "x${have_freerdp}" = "xyes"
then
    AC_CHECK_DECL([freerdp_register_addin_provider],
        [AC_DEFINE([HAVE_FREERDP_REGISTER_ADDIN_PROVIDER],,
                   [Whether freerdp_register_addin_provider() is defined])],,
        [#include <freerdp/addin.h>])

    AC_CHECK_DECL([freerdp_channels_global_init],
        [AC_DEFINE([HAVE_FREERDP_CHANNELS_GLOBAL_INIT],,
                   [Whether freerdp_channels_global_init() is defined])],,
        [#include <freerdp/channels/channels.h>])
fi

# Availability of ADDIN_ARGV structure for configuring plugins
if test "x${have_freerdp}" = "xyes"
then
    AC_CHECK_TYPE([ADDIN_ARGV],
        [AC_DEFINE([HAVE_ADDIN_ARGV],,
                   [Whether the ADDIN_ARGV type is available])],,
        [#include <freerdp/settings.h>])
fi

#
# FreeRDP: WinPR
#

# Check for stream support via WinPR
if test "x${have_freerdp}" = "xyes"
then
    AC_CHECK_HEADER(winpr/stream.h,,
                    [have_winpr=no,
                     AC_CHECK_DECL([stream_write_uint8],,
                                  [AC_MSG_WARN([
  --------------------------------------------
   Unable to find stream support
   RDP will be disabled.
  --------------------------------------------])
                                   have_freerdp=no],
                                  [#include <freerdp/utils/stream.h>])])
fi

# Check for types in WinPR
if test "x${have_freerdp}" = "xyes"
then
    AC_CHECK_HEADER(winpr/wtypes.h,,
                    [have_winpr=no,
                     AC_CHECK_HEADER(freerdp/types.h,,
                                     [AC_MSG_WARN([
  --------------------------------------------
   Unable to find type definitions
   RDP will be disabled.
  --------------------------------------------])
                                      have_freerdp=no])])
fi

if test "x${have_freerdp}" = "xyes" -a "x${have_winpr}" = "xyes"
then
    AC_DEFINE([ENABLE_WINPR],,
              [Whether library support for WinPR types was found])
fi

#
# FreeRDP: freerdp 
#

if test "x${have_freerdp}" = "xyes"
then
    # Check for current (as of 1.1) freerdp interface
    AC_CHECK_MEMBERS([freerdp.ContextSize],
                     [freerdp_interface=stable],,
                     [[#include <freerdp/freerdp.h>]])

    # If not current, check for legacy interface
    if test "x${freerdp_interface}" = "xunknown"
    then
        AC_CHECK_MEMBERS([freerdp.context_size],
                         [freerdp_interface=legacy],,
                         [[#include <freerdp/freerdp.h>]])
    fi

    # Set defines based on interface type, warn if unknown
    if test "x${freerdp_interface}" = "xlegacy"; then
        AC_DEFINE([LEGACY_FREERDP],,
                  [Whether the older version of the FreeRDP API was found])
    elif test "x${freerdp_interface}" = "xunknown"; then
        AC_MSG_WARN([
  --------------------------------------------
   Unknown FreeRDP interface
   RDP will be disabled.
  --------------------------------------------])
        have_freerdp=no
    fi
fi

#
# FreeRDP: rdpSettings
#

if test "x${have_freerdp}" = "xyes"
then
    # Check for current (as of 1.1) rdpSettings interface
    AC_CHECK_MEMBERS([rdpSettings.Width,
                      rdpSettings.Height,
                      rdpSettings.FastPathInput,
                      rdpSettings.FastPathOutput,
                      rdpSettings.SendPreconnectionPdu,
                      rdpSettings.OrderSupport],
                     [rdpsettings_interface=stable],,
                     [[#include <freerdp/freerdp.h>]])

    # If not current, check for legacy interface
    if test "x${rdpsettings_interface}" = "xunknown"
    then
        AC_CHECK_MEMBERS([rdpSettings.width,
                          rdpSettings.height,
                          rdpSettings.order_support],
                         [rdpsettings_interface=legacy],,
                         [[#include <freerdp/freerdp.h>]])
    fi

    # Set defines based on interface type, warn if unknown
    if test "x${rdpsettings_interface}" = "xlegacy"; then
        AC_DEFINE([LEGACY_RDPSETTINGS],,
                  [Whether the legacy version of the rdpSettings API was found])

        # Legacy interface may not have AudioPlayback settings
        AC_CHECK_MEMBERS([rdpSettings.audio_playback],,
                         [rdpsettings_audioplayback=no],
                         [[#include <freerdp/freerdp.h>]])

        # Legacy interface may not have AudioCapture settings
        AC_CHECK_MEMBERS([rdpSettings.audio_capture],,
                         [rdpsettings_audiocapture=no],
                         [[#include <freerdp/freerdp.h>]])

        # Legacy interface may not have DeviceRedirection settings
        AC_CHECK_MEMBERS([rdpSettings.device_redirection],,
                         [rdpsettings_deviceredirection=no],
                         [[#include <freerdp/freerdp.h>]])

    elif test "x${rdpsettings_interface}" = "xunknown"; then
        AC_MSG_WARN([
  --------------------------------------------
   Unknown rdpSettings interface
   RDP will be disabled.
  --------------------------------------------]) 
        have_freerdp=no
    fi
fi

# Activate audio playback settings if present
if test "x${have_freerdp}" = "xyes" -a "x${rdpsettings_audioplayback}" = "xyes"; then
    AC_DEFINE([HAVE_RDPSETTINGS_AUDIOPLAYBACK],,
              [Whether the rdpSettings structure has AudioPlayback settings])
fi

# Activate audio capture settings if present
if test "x${have_freerdp}" = "xyes" -a "x${rdpsettings_audiocapture}" = "xyes"; then
    AC_DEFINE([HAVE_RDPSETTINGS_AUDIOCAPTURE],,
              [Whether the rdpSettings structure has AudioCapture settings])
fi

# Activate device redirection settings if present
if test "x${have_freerdp}" = "xyes" -a "x${rdpsettings_deviceredirection}" = "xyes"; then
    AC_DEFINE([HAVE_RDPSETTINGS_DEVICEREDIRECTION],,
              [Whether the rdpSettings structure has DeviceRedirection settings])
fi

# Check if the type CHANNEL_ENTRY_POINTS_FREERDP exists, if not define it to CHANNEL_ENTRY_POINTS_EX
if test "x${have_freerdp}" = "xyes"
then
    AC_CHECK_TYPE([CHANNEL_ENTRY_POINTS_FREERDP],,
                  AC_DEFINE([CHANNEL_ENTRY_POINTS_FREERDP],[CHANNEL_ENTRY_POINTS_EX], [Type compatibility]),
                  [[#include <freerdp/svc.h>]])
fi

# Check if the freerdp version header exists
if test "x${have_freerdp}" = "xyes"
then
    AC_CHECK_HEADERS([freerdp/version.h])
fi

#
# FreeRDP: rdpBitmap
#

if test "x${have_freerdp}" = "xyes"
then
    AC_MSG_CHECKING([whether rdpBitmap.Decompress() requires the codec_id])
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([[#include <winpr/wtypes.h>
                                        #include <freerdp/freerdp.h>
                                        void __decompress(rdpContext* context,
                                                          rdpBitmap* bitmap,
                                                          UINT8* data,
                                                          int width,
                                                          int height,
                                                          int bpp,
                                                          int length,
                                                          BOOL compressed,
                                                          int codec_id);
                                        rdpBitmap b = { .Decompress = __decompress };]])],
                      [AC_MSG_RESULT([yes])],
                      [AC_MSG_RESULT([no])
                       AC_DEFINE([LEGACY_RDPBITMAP],,
                                 [Whether the legacy rdpBitmap API was found])])
fi

#
# FreeRDP: IWTSVirtualChannelCallback
#

if test "x${have_freerdp}" = "xyes"
then
    AC_MSG_CHECKING([whether IWTSVirtualChannelCallback.OnDataReceived() uses a wStream])
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([[#include <winpr/wtypes.h>
                                        #include <freerdp/dvc.h>
                                        #include <freerdp/freerdp.h>
                                        int __data_received(
                                            IWTSVirtualChannelCallback* channel_callback,
                                            wStream* stream);
                                        IWTSVirtualChannelCallback cb = {
                                            .OnDataReceived = __data_received
                                        };
                                        int main() {
                                            return
                                                cb.OnDataReceived(NULL, NULL);
                                        }]])],
                      [AC_MSG_RESULT([yes])],
                      [AC_MSG_RESULT([no])
                       AC_DEFINE([LEGACY_IWTSVIRTUALCHANNELCALLBACK],,
                                 [Whether the legacy IWTSVirtualChannelCallback API was found])])
fi

#
# FreeRDP: Decompression function variants
#

# Check whether interleaved_decompress() can handle the palette
if test "x${have_freerdp}" = "xyes"
then
    AC_MSG_CHECKING([whether interleaved_decompress() accepts an additional palette parameter])
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([[#include <freerdp/codec/interleaved.h>

                                        int main() {
                                            BYTE* palette = NULL;
                                            interleaved_decompress(NULL, NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 0, palette);
                                        }]])],
                      [AC_MSG_RESULT([yes])
                       AC_DEFINE([INTERLEAVED_DECOMPRESS_TAKES_PALETTE],,
                                 [Whether interleaved_decompress() accepts an additional palette parameter])],
                      [AC_MSG_RESULT([no])])
fi

# Check whether planar_decompress() will handle flipping
if test "x${have_freerdp}" = "xyes"
then
    AC_MSG_CHECKING([whether planar_decompress() can flip])
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([[#include <freerdp/codec/planar.h>

                                        int main() {
                                            BOOL* flip = TRUE;
                                            planar_decompress(NULL, NULL, 0, NULL, 0, 0, 0, 0, 0, 0, flip);
                                        }]])],
                      [AC_MSG_RESULT([yes])
                       AC_DEFINE([PLANAR_DECOMPRESS_CAN_FLIP],,
                                 [Whether planar_decompress() can flip])],
                      [AC_MSG_RESULT([no])])
fi

#
# FreeRDP: rdpContext
#

# Check for rdpContext.codecs
if test "x${have_freerdp}" = "xyes"
then
    AC_CHECK_MEMBERS([rdpContext.codecs],
                     [AC_DEFINE([FREERDP_BITMAP_REQUIRES_ALIGNED_MALLOC],,
                                [Whether this version of FreeRDP requires _aligned_malloc() for bitmap data])],,
                     [[#include <freerdp/freerdp.h>]])
fi

#
# FreeRDP: rdpPalette
#

if test "x${have_freerdp}" = "xyes"
then
    AC_MSG_CHECKING([whether rdpPalette.entries is static])
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([[#include <freerdp/update.h>
                                        rdpPalette p;
                                        PALETTE_ENTRY* foo = p.entries;]])],
                      [AC_MSG_RESULT([yes])],
                      [AC_MSG_RESULT([no])
                       AC_DEFINE([LEGACY_RDPPALETTE],,
                                 [Whether the legacy rdpPalette API was found])])
fi

#
# FreeRDP: rdpPointer
#

# Check for SetDefault and SetNull members of rdpPointer
if test "x${have_freerdp}" = "xyes"
then
    AC_CHECK_MEMBERS([rdpPointer.SetDefault,
                      rdpPointer.SetNull],
                     ,,
                     [[#include <freerdp/freerdp.h>]])
fi

#
# FreeRDP: wMessage / RDP_EVENT
#

# Check for current (as of 1.1) wMessage interface
if test "x${have_freerdp}" = "xyes"
then
    AC_CHECK_MEMBERS([wMessage.id],
                     [event_interface=stable],,
                     [[#include <winpr/collections.h>]])

    # If not current, check for legacy (RDP_EVENT) interface
    if test "x${event_interface}" = "xunknown"
    then
        AC_CHECK_MEMBERS([RDP_EVENT.event_class],
                         [event_interface=legacy],,
                         [[#include <freerdp/types.h>]])
    fi

    # Set defines based on interface type, warn if unknown
    if test "x${event_interface}" = "xlegacy"; then
        AC_DEFINE([LEGACY_EVENT],,
                  [Whether the legacy RDP_EVENT API was found])
    elif test "x${event_interface}" = "xunknown"; then
        AC_MSG_WARN([
  --------------------------------------------
   Unknown event interface
   RDP will be disabled.
  --------------------------------------------])
        have_freerdp=no
    fi
fi

AM_CONDITIONAL([LEGACY_FREERDP_EXTENSIONS], [test "x${legacy_freerdp_extensions}" = "xyes"])
AM_CONDITIONAL([ENABLE_WINPR], [test "x${have_winpr}"   = "xyes"])
AM_CONDITIONAL([ENABLE_RDP],   [test "x${have_freerdp}" = "xyes"])

AC_SUBST(RDP_LIBS)

#
# libssh2
#

have_libssh2=disabled
SSH_LIBS=
AC_ARG_WITH([ssh],
            [AS_HELP_STRING([--with-ssh],
                            [support SSH @<:@default=check@:>@])],
            [],
            [with_ssh=check])

AC_ARG_ENABLE(ssh_agent,
              [AS_HELP_STRING([--enable-ssh-agent],
                              [enable built-in ssh-agent])
              ],enable_ssh_agent=yes)

if test "x$with_ssh" != "xno"
then
    have_libssh2=yes

    AC_CHECK_LIB([ssh2], [libssh2_session_init_ex],
                         [SSH_LIBS="$SSH_LIBS -lssh2"],
                         [have_libssh2=no])
fi

AM_CONDITIONAL([ENABLE_COMMON_SSH], [test "x${have_libssh2}"  = "xyes" \
                                       -a "x${have_ssl}"      = "xyes"])
AM_COND_IF([ENABLE_COMMON_SSH],
           [AC_DEFINE([ENABLE_COMMON_SSH],,
                      [Whether support for the common SSH core is enabled])])

AM_CONDITIONAL([ENABLE_SSH], [test "x${have_libssh2}"  = "xyes" \
                                -a "x${have_terminal}" = "xyes" \
                                -a "x${have_ssl}"      = "xyes"])

AC_SUBST(SSH_LIBS)

#
# Underlying crypto library usage of libssh2
#

if test "x${have_libssh2}" = "xyes"
then

    # Whether libssh2 was built against libgcrypt
    AC_CHECK_LIB([ssh2], [gcry_control],
                         [AC_CHECK_HEADER(gcrypt.h,
                                          [AC_DEFINE([LIBSSH2_USES_GCRYPT],,
                                                     [Whether libssh2 was built against libgcrypt])],
                                          [AC_MSG_WARN([
  --------------------------------------------
   libssh2 appears to be built against libgcrypt, but the libgcrypt headers
   could not be found. SSH will be disabled.
  --------------------------------------------])
                                           have_libssh2=no])])

fi

#
# Agent forwarding support within libssh2
#

have_ssh_agent=no
if test "x${have_libssh2}" = "xyes" -a "x${enable_ssh_agent}" = "xyes"
then

    AC_CHECK_DECL([libssh2_channel_request_auth_agent],
                  [have_ssh_agent=yes], [],
                  [[#include <libssh2.h>]])

    if test "x${have_ssh_agent}" = "xno"
    then
        AC_MSG_ERROR([
      --------------------------------------------
       Agent forwarding support was requested but no such support was found
       in libssh2.
      --------------------------------------------])
    else
        AC_DEFINE([ENABLE_SSH_AGENT],,
                  [Whether agent forwarding support for SSH is enabled])
    fi

fi

AM_CONDITIONAL([ENABLE_SSH_AGENT],
	   [test "x${have_ssh_agent}"   = "xyes" \
	      -a "x${enable_ssh_agent}" = "xyes"])

#
# libtelnet
#

have_libtelnet=disabled
TELNET_LIBS=
AC_ARG_WITH([telnet],
            [AS_HELP_STRING([--with-telnet],
                            [support Telnet @<:@default=check@:>@])],
            [],
            [with_telnet=check])

if test "x$with_telnet" != "xno"
then
    have_libtelnet=yes
    AC_CHECK_LIB([telnet], [telnet_init],
                           [TELNET_LIBS="$TELNET_LIBS -ltelnet"],
                           [have_libtelnet=no])
fi

AM_CONDITIONAL([ENABLE_TELNET], [test "x${have_libtelnet}"  = "xyes" \
                                   -a "x${have_terminal}" = "xyes"])

AC_SUBST(TELNET_LIBS)

#
# libwebp
#

have_webp=disabled
WEBP_LIBS=
AC_ARG_WITH([webp],
            [AS_HELP_STRING([--with-webp],
                            [support WebP image encoding @<:@default=check@:>@])],
            [],
            [with_webp=check])

if test "x$with_webp" != "xno"
then
    have_webp=yes

    AC_CHECK_HEADER(webp/encode.h,, [have_webp=no])
    AC_CHECK_LIB([webp], [WebPEncode], [WEBP_LIBS="$WEBP_LIBS -lwebp"], [have_webp=no])

    if test "x${have_webp}" = "xno"
    then
        AC_MSG_WARN([
  --------------------------------------------
   Unable to find libwebp.
   Images will not be encoded using WebP.
  --------------------------------------------])
    else
        AC_DEFINE([ENABLE_WEBP],, [Whether WebP support is enabled])
    fi
fi

AM_CONDITIONAL([ENABLE_WEBP], [test "x${have_webp}" = "xyes"])
AC_SUBST(WEBP_LIBS)

#
# guacd
#

AC_ARG_ENABLE([guacd],
              [AS_HELP_STRING([--disable-guacd],
                              [do not build the Guacamole proxy daemon])],
              [],
              [enable_guacd=yes])

AM_CONDITIONAL([ENABLE_GUACD], [test "x${enable_guacd}"  = "xyes"])

#
# guacenc
#

AC_ARG_ENABLE([guacenc],
              [AS_HELP_STRING([--disable-guacenc],
                              [do not build the Guacamole video encoding tool])],
              [],
              [enable_guacenc=yes])

AM_CONDITIONAL([ENABLE_GUACENC], [test "x${enable_guacenc}"  = "xyes" \
                                    -a "x${have_libavcodec}" = "xyes" \
                                    -a "x${have_libavutil}"  = "xyes" \
                                    -a "x${have_libswscale}" = "xyes"])

#
# Output Makefiles
#

AC_CONFIG_FILES([Makefile
                 tests/Makefile
                 src/common/Makefile
                 src/common-ssh/Makefile
                 src/terminal/Makefile
                 src/libguac/Makefile
                 src/libguacd/Makefile
                 src/guacd/Makefile
                 src/guacenc/Makefile
                 src/pulse/Makefile
                 src/protocols/rdp/Makefile
                 src/protocols/ssh/Makefile
                 src/protocols/telnet/Makefile
                 src/protocols/vnc/Makefile])
AC_OUTPUT

#
# Protocol build status
#

AM_COND_IF([ENABLE_RDP],    [build_rdp=yes],    [build_rdp=no])
AM_COND_IF([ENABLE_SSH],    [build_ssh=yes],    [build_ssh=no])
AM_COND_IF([ENABLE_TELNET], [build_telnet=yes], [build_telnet=no])
AM_COND_IF([ENABLE_VNC],    [build_vnc=yes],    [build_vnc=no])

#
# Service / tool build status
#

AM_COND_IF([ENABLE_GUACD],   [build_guacd=yes],   [build_guacd=no])
AM_COND_IF([ENABLE_GUACENC], [build_guacenc=yes], [build_guacenc=no])

#
# Init scripts
#

AM_COND_IF([ENABLE_INIT], [build_init="${init_dir}"], [build_init=no])

#
# Display summary
#

echo "
------------------------------------------------
$PACKAGE_NAME version $PACKAGE_VERSION
------------------------------------------------

   Library status:

     freerdp ............. ${have_freerdp}
     pango ............... ${have_pango}
     libavcodec .......... ${have_libavcodec}
     libavutil ........... ${have_libavutil}
     libssh2 ............. ${have_libssh2}
     libssl .............. ${have_ssl}
     libswscale .......... ${have_libswscale}
     libtelnet ........... ${have_libtelnet}
     libVNCServer ........ ${have_libvncserver}
     libvorbis ........... ${have_vorbis}
     libpulse ............ ${have_pulse}
     libwebp ............. ${have_webp}

   Protocol support:

      RDP ....... ${build_rdp}
      SSH ....... ${build_ssh}
      Telnet .... ${build_telnet}
      VNC ....... ${build_vnc}

   Services / tools:

      guacd ...... ${build_guacd}
      guacenc .... ${build_guacenc}

   Init scripts: ${build_init}

Type \"make\" to compile $PACKAGE_NAME.
"

