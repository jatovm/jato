#!/bin/sh

# make sure that we have built jamvm classes
if [ ! -f ../lib/classes.zip ]; then
  make -C ../lib
fi

GNU_CLASSPATH_ROOT=`grep "with_classpath_install_dir =" ../jato/Makefile  | grep -v "#" |  awk '{ print $3 }'`
if test x"$GNU_CLASSPATH_ROOT" = x -o ! -d $GNU_CLASSPATH_ROOT; then
  echo "Error! Cannot find GNU Classpath installed."
  exit
fi

find jamvm/ -name "*.java" | xargs javac

GLIBJ=$GNU_CLASSPATH_ROOT/share/classpath/glibj.zip

../jato/jato -Xbootclasspath:../lib/classes.zip:$GLIBJ -cp . jamvm.IntegerArithmeticTest
if [ $? != 1 ]; then
  echo "Tests FAILED."
else
  echo "Tests OK."
fi

find jamvm/ -name "*.class" | xargs rm -f
