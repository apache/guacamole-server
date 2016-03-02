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

TESTS = test_libguac
check_PROGRAMS = test_libguac

noinst_HEADERS =          \
    client/client_suite.h \
    common/common_suite.h \
    protocol/suite.h      \
    util/util_suite.h

test_libguac_SOURCES =           \
    test_libguac.c               \
    client/client_suite.c        \
    client/buffer_pool.c         \
    client/layer_pool.c          \
    common/common_suite.c        \
    common/guac_iconv.c          \
    common/guac_string.c         \
    common/guac_rect.c           \
    protocol/suite.c             \
    protocol/base64_decode.c     \
    protocol/instruction_parse.c \
    protocol/instruction_read.c  \
    protocol/instruction_write.c \
    protocol/nest_write.c        \
    util/util_suite.c            \
    util/guac_pool.c             \
    util/guac_unicode.c

test_libguac_CFLAGS =       \
    -Werror -Wall -pedantic \
    @COMMON_INCLUDE@        \
    @LIBGUAC_INCLUDE@

test_libguac_LDADD = \
    @COMMON_LTLIB@   \
    @CUNIT_LIBS@     \
    @LIBGUAC_LTLIB@

