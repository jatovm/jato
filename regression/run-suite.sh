#!/bin/bash

HAS_FAILURES=0
CLASS_LIST=""

function run_java {
  JAVA_CLASS=$1
  EXPECTED=$2

  $GDB ../jato $JAVA_OPTS -cp $PWD $JAVA_CLASS

  ACTUAL=$?

  if [ "$ACTUAL" != "$EXPECTED" ]; then
    HAS_FAILURES=1
    echo "$JAVA_CLASS FAILED. Expected $EXPECTED, but was: $ACTUAL."
  fi
}

GNU_CLASSPATH_ROOT=`../tools/classpath-config`
if test x"$GNU_CLASSPATH_ROOT" = x -o ! -d "$GNU_CLASSPATH_ROOT"; then
  echo "Error! Cannot find GNU Classpath installed."
  exit
fi

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
    run_java jvm.ArrayExceptionsTest 0
    run_java jvm.ArrayMemberTest 0
    run_java jvm.ArrayTest 0
    run_java jvm.BranchTest 0
    run_java jvm.ClassExceptionsTest 0
    run_java jvm.CloneTest 0
    run_java jvm.ControlTransferTest 0
    run_java jvm.ConversionTest 0
    run_java jvm.ExceptionsTest 0
    run_java jvm.ExitStatusIsOneTest 1
    run_java jvm.ExitStatusIsZeroTest 0
    run_java jvm.FibonacciTest 0
    run_java jvm.FinallyTest 0
    run_java jvm.FloatArithmeticTest 0
    run_java jvm.GetstaticPatchingTest 0
    run_java jvm.IntegerArithmeticExceptionsTest 0
    run_java jvm.IntegerArithmeticTest 0
    run_java jvm.InterfaceInheritanceTest 0
    run_java jvm.InvokeinterfaceTest 0
    run_java jvm.InvokestaticPatchingTest 0
    run_java jvm.LoadConstantsTest 0
    run_java jvm.LongArithmeticExceptionsTest 0
    run_java jvm.LongArithmeticTest 0
    run_java jvm.MethodInvocationAndReturnTest 0
    run_java jvm.MethodInvocationExceptionsTest 0
    run_java jvm.ObjectArrayTest 0
    run_java jvm.ObjectCreationAndManipulationExceptionsTest 0
    run_java jvm.ObjectCreationAndManipulationTest 0
    run_java jvm.ObjectStackTest 0
    run_java jvm.ParameterPassingTest 55
    run_java jvm.PrintTest 0
    run_java jvm.PutfieldTest 0
    run_java jvm.PutstaticPatchingTest 0
    run_java jvm.PutstaticTest 0
    run_java jvm.RegisterAllocatorTortureTest 0
    run_java jvm.StackTraceTest 0
    run_java jvm.StringTest 0
    run_java jvm.SynchronizationExceptionsTest 0
    run_java jvm.SynchronizationTest 0
    run_java jvm.TrampolineBackpatchingTest 0
    run_java jvm.SubroutineTest 0
    run_java jvm.WideTest 0
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
