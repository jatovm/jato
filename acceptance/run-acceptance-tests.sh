#!/bin/bash

HAS_FAILURES=0
JAVA_OPTS=$*

function run_java {
  JAVA_CLASS=$1
  EXPECTED=$2

  ../jato-exe \
      -Dgnu.classpath.boot.library.path=$GNU_CLASSPATH_ROOT/lib/classpath \
      -Xbootclasspath:$BOOTCLASSPATH $JAVA_OPTS -cp . $JAVA_CLASS

  ACTUAL=$?

  if [ "$ACTUAL" != "$EXPECTED" ]; then
    HAS_FAILURES=1
    echo "$JAVA_CLASS FAILED. Expected $EXPECTED, but was: $ACTUAL."
  fi
}

make -C ..

# make sure that we have built jamvm classes
if [ ! -f ../lib/classes.zip ]; then
  make -C ../lib
fi

GNU_CLASSPATH_ROOT=`grep "with_classpath_install_dir =" ../Makefile  | grep -v "#" |  awk '{ print $3 }'`
if test x"$GNU_CLASSPATH_ROOT" = x -o ! -d $GNU_CLASSPATH_ROOT; then
  echo "Error! Cannot find GNU Classpath installed."
  exit
fi

GLIBJ=$GNU_CLASSPATH_ROOT/share/classpath/glibj.zip

BOOTCLASSPATH=../lib/classes.zip:$GLIBJ

find jamvm/ -name "*.java" | xargs jikes -cp $BOOTCLASSPATH

run_java jamvm.ExitStatusIsZeroTest 0
run_java jamvm.ExitStatusIsOneTest 1
run_java jamvm.IntegerArithmeticTest 0

find jamvm/ -name "*.class" | xargs rm -f

if [ "$HAS_FAILURES" == "0" ]; then
  echo "Tests OK."
else
  echo "Tests FAILED."
fi
