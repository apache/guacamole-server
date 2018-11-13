#!/usr/bin/perl
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

#
# generate-test-runner.pl
#
# Generates a test runner for the .c files given on the command line. Each .c
# file may declare any number of tests so long as each test uses CUnit and is
# declared with the following convention:
#
# void test_SUITENAME__TESTNAME() {
#     ...
# }
#
# where TESTNAME is the arbitrary name of the test and SUITENAME is the
# arbitrary name of the test suite that this test belongs to.
#
# Absolutely all tests MUST follow the above convention if they are to be
# picked up by this script. Functions which are not tests MUST NOT follow
# the above convention.
#

use strict;

# Parse all test declarations from given file
my %test_suites = ();
while (<>) {
    if ((my $suite_name, my $test_name) = m/^void\s+test_(\w+)__(\w+)/) {
        $test_suites{$suite_name} //= ();
        push @{$test_suites{$suite_name}}, $test_name;
    }
}

#
# Common test runner header
#

print <<'END';
/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <stdlib.h>
#include <CUnit/Basic.h>
END

#
# Prototypes for all test functions
#

while ((my $suite_name, my $test_names) = each (%test_suites)) {
    print "\n/* Automatically-generated prototypes for the $suite_name suite */\n";
    foreach my $test_name (@{ $test_names }) {
        print "void test_${suite_name}__${test_name}();\n";
    }
}

#
# Beginning of main() function body for test runner
#

print <<"END";

/* Automatically-generated test runner */
int main() {

    /* Init CUnit test registry */
    if (CU_initialize_registry() != CUE_SUCCESS)
        return CU_get_error();
END

#
# Within main(), register each test and its corresponding test suite
#

while ((my $suite_name, my $test_names) = each (%test_suites)) {

    print <<"    END";

    /* Create and register all tests for the $suite_name suite */
    CU_pSuite $suite_name = CU_add_suite("$suite_name", NULL, NULL);
    if ($suite_name == NULL
    END

    foreach my $test_name (@{ $test_names }) {
        print <<"        END";
        || CU_add_test($suite_name, "$test_name", test_${suite_name}__${test_name}) == NULL
        END
    }

    print <<"    END";
    ) goto cleanup;
    END

}

#
# End of main() function
#

print <<"END";

    /* Run all tests in all suites */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

cleanup:
    /* Tests complete */
    CU_cleanup_registry();
    return CU_get_error();

}
END

