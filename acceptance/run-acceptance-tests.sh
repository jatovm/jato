#!/bin/sh

make -C ..
make -C ../jato

# make sure that we have built jamvm classes
if [ ! -f ../lib/classes.zip ]; then
  make -C ../lib
fi

GNU_CLASSPATH_ROOT=`grep "with_classpath_install_dir =" ../jato/Makefile  | grep -v "#" |  awk '{ print $3 }'`
if test x"$GNU_CLASSPATH_ROOT" = x -o ! -d $GNU_CLASSPATH_ROOT; then
  echo "Error! Cannot find GNU Classpath installed."
  exit
fi

GLIBJ=$GNU_CLASSPATH_ROOT/share/classpath/glibj.zip

BOOTCLASSPATH=../lib/classes.zip:$GLIBJ

find jamvm/ -name "*.java" | xargs jikes -cp $BOOTCLASSPATH

../jato/jato \
	-Dgnu.classpath.boot.library.path=$GNU_CLASSPATH_ROOT/lib/classpath \
	-Xbootclasspath:$BOOTCLASSPATH -cp . jamvm.IntegerArithmeticTest
if [ $? == 0 ]; then
  echo "Tests OK."
else
  echo "Tests FAILED. Program exited with error code $?."
fi

find jamvm/ -name "*.class" | xargs rm -f
