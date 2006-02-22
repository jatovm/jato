#!/bin/sh

find jamvm/ -name "*.java" | xargs javac

java -cp . jamvm.IntegerArithmeticTest
if [ $? != 0 ]; then
  echo "Tests FAILED."
else
  echo "Tests OK."
fi

find jamvm/ -name "*.class" | xargs rm -f
