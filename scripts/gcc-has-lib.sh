#!/bin/sh

GCC=$1
LIB=$2

echo "int main(int argc, char *argv[]) {return 0;}" | gcc -l$LIB -x c - > /dev/null 2>&1
if [ "$?" -eq "0" ] ; then
	echo y
else
	echo n
fi
