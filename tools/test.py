#!/usr/bin/env python

TEST_DIR = "regression"

TESTS = [
  ( "jvm/EntryTest"           , 0, [ "i386" ] )
, ( "jvm/ExitStatusIsZeroTest", 0, [ "i386" ] )
, ( "jvm/ExitStatusIsOneTest" , 1, [ "i386" ] )
, ( "jvm/ArgsTest"            , 0, [ "i386" ] )
]

import subprocess
import platform
import time
import sys
import os

def guess_arch():
  arch = platform.machine()
  if arch == "i686":
    return "i386"
  return "unknown"

ARCH = guess_arch()

def is_test_supported(t):
  klass, expected_retval, archs = t
  return ARCH in archs

def success(s):
  return "\033[32m" + s + "\033[0m"

def failure(s):
  return "\033[31m" + s + "\033[0m"

def run(program, *args):
  pid = os.fork()
  if not pid:
    os.execvp(program, (program,) +  args)
  return os.wait()[0]

def main():
  retval = passed = failed = 0
  start = time.time()
  for t in TESTS:
    klass, expected_retval, archs = t
    if is_test_supported(t):
      retval = subprocess.call(["./jato", "-cp", TEST_DIR, klass])
      if retval != expected_retval:
        print klass + ": Test FAILED"
        failed += 1
      else:
        passed += 1

  end = time.time()

  elapsed = end - start

  status = ""
  if passed != 0 or failed != 0:
    if passed == 0:
      status += "No tests passed"
    elif passed == 1:
      status += "%d test passed" % passed
    else:
      status += "%d tests passed" % passed

    if failed == 1:
      status += ", %d test failed" % failed
    elif failed > 1:
      status += ", %d tests failed" % failed
  else:
    status += "No tests run"

  if failed > 0:
    status = failure(status)
  elif passed > 0:
    status = success(status)

  print "%s (%.2f s) " % (status, elapsed)
  sys.exit(retval)

if __name__ == '__main__':
  sys.exit(main())
