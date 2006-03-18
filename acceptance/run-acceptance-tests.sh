#!/bin/sh

find jamvm/ -name "*.java" | xargs javac

GLIBJ=/usr/local/classpath/share/classpath/glibj.zip

../jato/jato -Xbootclasspath:../lib/classes.zip:$GLIBJ -cp . jamvm.IntegerArithmeticTest
if [ $? != 0 ]; then
  echo "Tests FAILED."
else
  echo "Tests OK."
fi

find jamvm/ -name "*.class" | xargs rm -f
