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
# The Original Code is libguac.
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
AC_INIT([libguac], [0.7.0])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

LT_PREREQ([2.2])
LT_INIT([dlopen])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/protocol.c])
AC_CONFIG_HEADERS([include/protocol.h])

# Programs
AC_PROG_CC
AC_PROG_CC_C99
AC_PROG_LIBTOOL

# Headers
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h sys/socket.h time.h sys/time.h syslog.h unistd.h cairo/cairo.h pngstruct.h])

# Libraries
AC_CHECK_LIB([dl], [dlopen],, AC_MSG_ERROR("libdl is required for loading client plugins"))
AC_CHECK_LIB([png], [png_write_png],, AC_MSG_ERROR("libpng is required for writing png messages"))
AC_CHECK_LIB([cairo], [cairo_create],, AC_MSG_ERROR("cairo is required for drawing instructions"))
AC_CHECK_LIB([pthread], [pthread_create])
AC_CHECK_LIB([wsock32], [main])

# Library functions
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([clock_gettime gettimeofday memmove memset select strdup png_get_io_ptr nanosleep])

# Typedefs
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

# Unit tests
AC_CHECK_LIB([cunit], [CU_run_test], [CUNIT_LIBS=-lcunit])
AC_SUBST(CUNIT_LIBS)

# Defines
AC_DEFINE(_POSIX_C_SOURCE, 199309L)
AC_DEFINE(_BSD_SOURCE)

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 tests/Makefile])
AC_OUTPUT

