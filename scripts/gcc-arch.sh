#!/bin/sh
#
# gcc-arch [-p] gcc-command
#
# Prints the machine architecture gcc compiles to. This script is based on
# linux/scripts/gcc-version.sh.
#

compiler="$*"

if [ ${#compiler} -eq 0 ]; then
        echo "Error: No compiler specified."
        printf "Usage:\n\t$0 <gcc-command>\n"
        exit 1
fi

X86_32=$(echo __i386__ | $compiler -E -xc - | tail -n 1)
X86_64=$(echo __x86_64__ | $compiler -E -xc - | tail -n 1)

if [ "$X86_32" = "1" ]; then
  echo "i386"
elif [ "$X86_64" = 1 ]; then
  echo "x86_64"
else
  echo "unknown"
fi
