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
#include <stdlib.h>
#include <stdio.h>

extern unsigned long nr_failed;
'

cat $FILES | grep '^void test_' | grep -v "__ignore" | 
    sed -e 's/(.*$//' \
        -e 's/$/(void);/' \
        -e 's/^/extern /'

echo \
'

static void run_tests(void) 
{

'
cat $FILES | grep '^void test_' | grep -v "__ignore" |
    sed -e 's/^void //' \
        -e 's/(.*$//' \
        -e 's/^/    /' \
        -e 's/$/();/'

echo \
'
    print_test_suite_results();
}

int main(int argc, char *argv[])
{
    run_tests();

    if (nr_failed > 0)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
'
