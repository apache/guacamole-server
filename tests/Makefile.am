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

AUTOMAKE_OPTIONS = foreign 
ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = -Werror -Wall -pedantic @LIBGUAC_INCLUDE@

TESTS = test_libguac
check_PROGRAMS = test_libguac

noinst_HEADERS =          \
	client/client_suite.h \
	protocol/suite.h      \
	util/util_suite.h

test_libguac_SOURCES =           \
    test_libguac.c               \
	client/client_suite.c        \
	client/buffer_pool.c         \
	client/layer_pool.c          \
	protocol/suite.c             \
	protocol/base64_decode.c     \
	protocol/instruction_read.c  \
	protocol/instruction_write.c \
	protocol/nest_write.c        \
	util/util_suite.c            \
	util/guac_pool.c             \
	util/guac_unicode.c

test_libguac_LDADD = @LIBGUAC_LTLIB@ @CUNIT_LIBS@

