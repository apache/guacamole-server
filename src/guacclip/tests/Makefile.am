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

#
# Unit tests for guacclip
#

check_PROGRAMS = test_guacclip
TESTS = $(check_PROGRAMS)

#
# The test runner is generated from the test source files. The guacclip logic
# sources (everything except guacclip.c, which provides main()) are compiled
# directly into the test program so that the interpreter can be exercised
# end-to-end.
#

test_guacclip_SOURCES =    \
    test_sha256.c          \
    test_interpret.c       \
    ../instructions.c      \
    ../interpret.c         \
    ../log.c               \
    ../sha256.c            \
    ../state.c

test_guacclip_CFLAGS =    \
    -Werror -Wall         \
    @LIBGUAC_INCLUDE@     \
    -I$(srcdir)/..

test_guacclip_LDADD =    \
    @CUNIT_LIBS@         \
    @LIBGUAC_LTLIB@

#
# Autogenerate test runner
#

GEN_RUNNER = $(top_srcdir)/util/generate-test-runner.pl
CLEANFILES = _generated_runner.c

_generated_runner.c: $(test_guacclip_SOURCES)
	$(AM_V_GEN) $(GEN_RUNNER) $(test_guacclip_SOURCES) > $@

nodist_test_guacclip_SOURCES = \
    _generated_runner.c

# Use automake's TAP test driver for running any tests
LOG_DRIVER =                \
    env AM_TAP_AWK='$(AWK)' \
    $(SHELL) $(top_srcdir)/build-aux/tap-driver.sh
