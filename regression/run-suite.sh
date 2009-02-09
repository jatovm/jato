#!/bin/bash

HAS_FAILURES=0
CLASS_LIST=""

function run_java {
  JAVA_CLASS=$1
  EXPECTED=$2

  ../jato \
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

while [ "$#" -ge 1 ]; do
    case "$1" in 
	-t)
	echo 'Tracing execution.'
	JAVA_OPTS="$JAVA_OPTS -Xtrace:jit"
	;;

	-*)
	JAVA_OPTS="$JAVA_OPTS $1"
	;;

	*)
	echo 'Running test' $1
	CLASS_LIST="$CLASS_LIST $1"
	;;
    esac;

    shift
done

if [ -z "$CLASS_LIST" ]; then
    run_java jamvm.ExitStatusIsZeroTest 0
    run_java jamvm.ExitStatusIsOneTest 1
    run_java jamvm.LoadConstantsTest 0
    run_java jamvm.IntegerArithmeticTest 0
    run_java jamvm.LongArithmeticTest 0
    run_java jamvm.ConversionTest 0
    run_java jamvm.ObjectCreationAndManipulationTest 0
    run_java jamvm.SynchronizationTest 0
    run_java jamvm.MethodInvocationAndReturnTest 0
    run_java jamvm.ControlTransferTest 0
else 
    for i in $CLASS_LIST; do
	run_java $i 0
    done
fi


if [ "$HAS_FAILURES" == "0" ]; then
  echo "Tests OK."
else
  echo "Tests FAILED."
fi
