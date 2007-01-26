#!/usr/local/bin/bash

# Auto generate single AllTests file for CuTest.
# Searches through all *.c files in the current directory.
# Prints to stdout.
# Author: Asim Jalis
# Date: 01/08/2003

if test $# -eq 0 ; then FILES="test/*/*.c" ; else FILES=$* ; fi

echo '

/* This is auto-generated code. Edit at your own peril. */

#include <libharness.h>
#include <stdio.h>
#include <stdlib.h>
'

cat $FILES | grep '^void test_' | 
    sed -e 's/(.*$//' \
        -e 's/$/(void);/' \
        -e 's/^/extern /'

echo \
'

static void run_tests(void) 
{

'
cat $FILES | grep '^void test_' | 
    sed -e 's/^void //' \
        -e 's/(.*$//' \
        -e 's/^/    /' \
        -e 's/$/();/'

echo \
'
    print_test_suite_results();
}

extern const char *exe_name;

int main(int argc, char *argv[])
{
    exe_name = argv[0];
    run_tests();
    return 0;
}
'
