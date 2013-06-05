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
AC_INIT([guacamole-server], [0.8.0])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

LT_PREREQ([2.2])
LT_INIT([dlopen])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SUBDIRS([libguac guacd])

#
# Protocol support options
#

AC_ARG_ENABLE([vnc],
  [AS_HELP_STRING([--disable-vnc],
     [do not build VNC support])],
  [],
  [enable_vnc=yes])

AC_ARG_ENABLE([rdp],
  [AS_HELP_STRING([--disable-rdp],
     [do not build support for Windows Remote Desktop (RDP)])],
  [],
  [enable_rdp=yes])

AC_ARG_ENABLE([ssh],
  [AS_HELP_STRING([--disable-ssh],
     [do not build SSH support])],
  [],
  [enable_ssh=yes])

AC_ARG_ENABLE([spice],
  [AS_HELP_STRING([--disable-spice],
     [do not build SPICE support])],
  [],
  [enable_spice=yes])

PROTOCOL_DIRS=

# VNC
if test "x$enable_vnc" = xyes; then
    PROTOCOL_DIRS="$PROTOCOL_DIRS protocols/vnc"
    AC_CONFIG_SUBDIRS([protocols/vnc])
fi

# RDP 
if test "x$enable_rdp" = xyes; then
    PROTOCOL_DIRS="$PROTOCOL_DIRS protocols/rdp"
    AC_CONFIG_SUBDIRS([protocols/rdp])
fi

# SSH
if test "x$enable_ssh" = xyes; then
    PROTOCOL_DIRS="$PROTOCOL_DIRS protocols/ssh"
    AC_CONFIG_SUBDIRS([protocols/ssh])
fi

# SPICE 
if test "x$enable_spice" = xyes; then
    PROTOCOL_DIRS="$PROTOCOL_DIRS protocols/spice"
    AC_CONFIG_SUBDIRS([protocols/spice])
fi

AC_SUBST([PROTOCOL_DIRS])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT

