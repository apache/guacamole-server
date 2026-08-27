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

AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -include config.h
ACLOCAL_AMFLAGS = -I m4

check_PROGRAMS = test_guacd
TESTS = $(check_PROGRAMS)

test_guacd_SOURCES =   \
    ../conf-parse.c    \
    client-timeout.c

test_guacd_CFLAGS =         \
    -Werror -Wall -pedantic \
    -I$(top_srcdir)/src/guacd \
    @LIBGUAC_INCLUDE@

test_guacd_LDADD = \
    @CUNIT_LIBS@

GEN_RUNNER = $(top_srcdir)/util/generate-test-runner.pl
CLEANFILES = _generated_runner.c

_generated_runner.c: $(test_guacd_SOURCES)
	$(AM_V_GEN) $(GEN_RUNNER) $(test_guacd_SOURCES) > $@

nodist_test_guacd_SOURCES = \
    _generated_runner.c

LOG_DRIVER =                \
    env AM_TAP_AWK='$(AWK)' \
    $(SHELL) $(top_srcdir)/build-aux/tap-driver.sh
