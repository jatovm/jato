#!/bin/bash

HAS_FAILURES=0
JAVA_OPTS=$*

function run_java {
  JAVA_CLASS=$1
  EXPECTED=$2

  ../bin/java \
      -Dgnu.classpath.boot.library.path=$GNU_CLASSPATH_ROOT/lib/classpath \
      -Xbootclasspath:$BOOTCLASSPATH $JAVA_OPTS -cp . $JAVA_CLASS

  ACTUAL=$?

  if [ "$ACTUAL" != "$EXPECTED" ]; then
    HAS_FAILURES=1
    echo "$JAVA_CLASS FAILED. Expected $EXPECTED, but was: $ACTUAL."
  fi
}

GNU_CLASSPATH_ROOT=`../tools/classpath-config`
if test x"$GNU_CLASSPATH_ROOT" = x -o ! -d $GNU_CLASSPATH_ROOT; then
  echo "Error! Cannot find GNU Classpath installed."
  exit
fi

GLIBJ=$GNU_CLASSPATH_ROOT/share/classpath/glibj.zip

BOOTCLASSPATH=../lib/classes.zip:$GLIBJ

run_java jamvm.ExitStatusIsZeroTest 0
run_java jamvm.ExitStatusIsOneTest 1
run_java jamvm.LoadConstantsTest 0
run_java jamvm.IntegerArithmeticTest 0
run_java jamvm.LongArithmeticTest 0
run_java jamvm.ObjectCreationAndManipulationTest 0
run_java jamvm.SynchronizationTest 0
run_java jamvm.MethodInvocationAndReturnTest 0
run_java jamvm.ControlTransferTest 0

if [ "$HAS_FAILURES" == "0" ]; then
  echo "Tests OK."
else
  echo "Tests FAILED."
fi
