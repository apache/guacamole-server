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
# The Original Code is guacamole-server.
#
# The Initial Developer of the Original Code is
# Michael Jumper.
# Portions created by the Initial Developer are Copyright (C) 2010
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


AC_PREREQ([2.61])
AC_INIT([guacamole-server], [0.8.2])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

LT_PREREQ([2.2])
LT_INIT([dlopen])

AC_CONFIG_MACRO_DIR([m4])

# Programs
AC_PROG_CC
AC_PROG_CC_C99
AC_PROG_LIBTOOL

# Headers
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h sys/socket.h time.h sys/time.h syslog.h unistd.h cairo/cairo.h pngstruct.h])

# Source characteristics
AC_DEFINE([_POSIX_C_SOURCE], [199309L], [Uses POSIX APIs])
AC_DEFINE([_BSD_SOURCE],     [],        [Uses BSD APIs])

# libdl
AC_CHECK_LIB([dl], [dlopen], [DL_LIBS=-ldl],
             AC_MSG_ERROR("libdl is required for loading client plugins"))

# libpng
AC_CHECK_LIB([png], [png_write_png], [PNG_LIBS=-lpng],
             AC_MSG_ERROR("libpng is required for writing png messages"))

# Cairo
AC_CHECK_LIB([cairo], [cairo_create], [CAIRO_LIBS=-lcairo],
             AC_MSG_ERROR("Cairo is required for drawing instructions"))

# libpthread
AC_CHECK_LIB([pthread], [pthread_create], [PTHREAD_LIBS=-lpthread
              AC_DEFINE([HAVE_LIBPTHREAD])])

# cunit
AC_CHECK_LIB([cunit], [CU_run_test], [CUNIT_LIBS=-lcunit])

# WinSock
AC_CHECK_LIB([wsock32], [main])

AC_SUBST(DL_LIBS)
AC_SUBST(PNG_LIBS)
AC_SUBST(CAIRO_LIBS)
AC_SUBST(PTHREAD_LIBS)
AC_SUBST(CUNIT_LIBS)

# Library functions
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([clock_gettime gettimeofday memmove memset select strdup nanosleep])

AC_CHECK_DECL([png_get_io_ptr],
	[AC_DEFINE([HAVE_PNG_GET_IO_PTR])],,
	[#include <png.h>])

AC_CHECK_DECL([cairo_format_stride_for_width],
	[AC_DEFINE([HAVE_CAIRO_FORMAT_STRIDE_FOR_WIDTH])],,
	[#include <cairo/cairo.h>])

# Typedefs
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

# Bundled libguac
AC_SUBST([LIBGUAC_LTLIB],   '$(top_builddir)/src/libguac/libguac.la')
AC_SUBST([LIBGUAC_INCLUDE], '-I$(top_srcdir)/src/libguac')

# Options
AC_ARG_WITH(init_dir,
            [AS_HELP_STRING([--with-init-dir=<path>],
                            [install init scripts to the given directory])
            ],init_dir=$withval)
AM_CONDITIONAL([ENABLE_INIT], [test "x${init_dir}" != "x"])
AC_SUBST(init_dir)

#
# libssl
#

have_ssl=yes
SSL_LIBS=

AC_CHECK_HEADER(openssl/ssl.h,, [have_ssl=no])
AC_CHECK_LIB([ssl], [SSL_CTX_new], [SSL_LIBS="$SSL_LIBS -lssl"], [have_ssl=no])
AM_CONDITIONAL([ENABLE_SSL], [test "x${have_ssl}" = "xyes"])

if test "x${have_ssl}" = "xno"
then
    AC_MSG_WARN([
  --------------------------------------------
   Unable to find libssl.
   guacd will not support SSL connections.
  --------------------------------------------])
else
    AC_DEFINE([ENABLE_SSL])
fi

AC_SUBST(SSL_LIBS)


#
# Ogg Vorbis
#

have_vorbis=yes
VORBIS_LIBS=

AC_CHECK_HEADER(vorbis/vorbisenc.h,, [have_vorbis=no])
AC_CHECK_LIB([ogg], [ogg_stream_init], [VORBIS_LIBS="$VORBIS_LIBS -logg"], [have_vorbis=no])
AC_CHECK_LIB([vorbis], [vorbis_block_init], [VORBIS_LIBS="$VORBIS_LIBS -lvorbis"], [have_vorbis=no])
AC_CHECK_LIB([vorbisenc], [vorbis_encode_init], [VORBIS_LIBS="$VORBIS_LIBS -lvorbisenc"], [have_vorbis=no])
AM_CONDITIONAL([ENABLE_OGG], [test "x${have_vorbis}" = "xyes"])

if test "x${have_vorbis}" = "xno"
then
    AC_MSG_WARN([
  --------------------------------------------
   Unable to find libogg / libvorbis / libvorbisenc.
   Sound will not be encoded with Ogg Vorbis.
  --------------------------------------------])
else
    AC_DEFINE([ENABLE_OGG])
fi

AC_SUBST(VORBIS_LIBS)

#
# PANGO
#

have_pango=yes
PKG_CHECK_MODULES([PANGO], [pango],, [have_pango=no]);
PKG_CHECK_MODULES([PANGOCAIRO], [pangocairo],, [have_pango=no]);

#
# libVNCServer
#

have_libvncserver=yes
VNC_LIBS=

AC_CHECK_LIB([vncclient], [rfbInitClient], [VNC_LIBS="$VNC_LIBS -lvncclient"], [have_libvncserver=no])
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
        AC_DEFINE([ENABLE_VNC_REPEATER])
    fi

fi

#
# FreeRDP
#

have_freerdp=yes
RDP_LIBS=

AC_CHECK_LIB([freerdp-cache],    [glyph_cache_register_callbacks], [RDP_LIBS="$RDP_LIBS -lfreerdp-cache"], [have_freerdp=no])
AC_CHECK_LIB([freerdp-core],     [freerdp_new], [RDP_LIBS="$RDP_LIBS -lfreerdp-core"], [have_freerdp=no])
AC_CHECK_LIB([freerdp-channels], [freerdp_channels_new], [RDP_LIBS="$RDP_LIBS -lfreerdp-channels"], [have_freerdp=no])
AC_CHECK_LIB([freerdp-utils],    [xzalloc], [RDP_LIBS="$RDP_LIBS -lfreerdp-utils"], [have_freerdp=no])
AC_CHECK_LIB([freerdp-codec],    [freerdp_image_convert], [RDP_LIBS="$RDP_LIBS -lfreerdp-codec"], [have_freerdp=no])
AM_CONDITIONAL([ENABLE_RDP], [test "x${have_freerdp}" = "xyes"])

AC_SUBST(RDP_LIBS)

#
# libssh
#

have_libssh=yes
SSH_LIBS=

AC_CHECK_LIB([ssh], [ssh_new], [SSH_LIBS="$SSH_LIBS -lssh"], [have_libssh=no])
AM_CONDITIONAL([ENABLE_SSH], [test "x${have_libssh}" = "xyes" -a "x${have_pango}" = "xyes"])

AC_SUBST(SSH_LIBS)

AC_CONFIG_FILES([Makefile
                 tests/Makefile
                 src/libguac/Makefile
                 src/guacd/Makefile
                 src/protocols/rdp/Makefile
                 src/protocols/ssh/Makefile
                 src/protocols/vnc/Makefile])
AC_OUTPUT

AM_COND_IF([ENABLE_RDP],  [build_rdp=yes], [build_rdp=no])
AM_COND_IF([ENABLE_SSH],  [build_ssh=yes], [build_ssh=no])
AM_COND_IF([ENABLE_VNC],  [build_vnc=yes], [build_vnc=no])

AM_COND_IF([ENABLE_INIT], [build_init="${init_dir}"], [build_init=no])

echo "
------------------------------------------------
$PACKAGE_NAME version $PACKAGE_VERSION
------------------------------------------------

   Library status:

     freerdp ............. ${have_freerdp}
     pango ............... ${have_pango}
     libssh .............. ${have_libssh}
     libssl .............. ${have_ssl}
     libVNCServer ........ ${have_libvncserver}
     libvorbis ........... ${have_vorbis}

   Protocol support:

      RDP ....... ${build_rdp}
      SSH ....... ${build_ssh}
      VNC ....... ${build_vnc}

   Init scripts: ${build_init}

Type \"make\" to compile $PACKAGE_NAME.
"

