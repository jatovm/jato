#!/usr/bin/env python

from threading import Thread
from Queue import Queue
import multiprocessing
import subprocess
import platform
import argparse
import time
import sys
import os

TEST_DIR = "test/functional"

CLASSPATH_DIR = os.popen('tools/classpath-config').read().strip()

NO_SYSTEM_CLASSLOADER = [ "-bootclasspath", TEST_DIR + ":" + CLASSPATH_DIR + "/share/classpath/glibj.zip", "-Djava.library.path=" + CLASSPATH_DIR + "/lib/classpath/", "-Xnosystemclassloader" ]

TESTS = [
  #                            Exit
  #  Test                      Code  Extra VM arguments       Architectures
  # ========================== ====  =======================  =============
  ( "jvm/EntryTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm/EntryTest", 0, NO_SYSTEM_CLASSLOADER + [ "-Xint" ], [ "i386", "x86_64" ] )
, ( "jvm/EntryTest", 0, NO_SYSTEM_CLASSLOADER + [ "-Xssa" ], [ "i386" ] )
, ( "jvm/EntryTest", 0, NO_SYSTEM_CLASSLOADER + [ "-Xnewgc" ], [ "i386", "x86_64" ] )
, ( "jvm/ExitStatusIsZeroTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm/ExitStatusIsOneTest", 1, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm/ArgsTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.ArrayExceptionsTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.ArrayMemberTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.ArrayTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.BranchTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.CFGCrashTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.ClinitFloatTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.ClassExceptionsTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.ClassLoaderTest", 0, [ ], [ "i386", "x86_64" ] )
, ( "jvm.CloneTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.ControlTransferTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.ConversionTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.DoubleArithmeticTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.DoubleConversionTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.DupTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.ExceptionsTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.ExceptionHandlerTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.FibonacciTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.FinallyTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.FloatArithmeticTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.FloatConversionTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.GcTortureTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.GetstaticPatchingTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.IntegerArithmeticExceptionsTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.IntegerArithmeticTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.InterfaceFieldInheritanceTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.InterfaceInheritanceTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.InvokeinterfaceTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.InvokeResultTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.InvokeTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.InvokestaticPatchingTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.LoadConstantsTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.LongArithmeticExceptionsTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.LongArithmeticTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.MethodInvocationAndReturnTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.MethodInvokeVirtualTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.MethodInvocationExceptionsTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.MultithreadingTest", 0, [ ], [ "i386", "x86_64" ] )
, ( "jvm.MethodOverridingFinal", 1, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.NoSuchMethodErrorTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.ObjectArrayTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.ObjectCreationAndManipulationExceptionsTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.ObjectCreationAndManipulationTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.ObjectStackTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.ParameterPassingTest", 100, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.ParameterPassingLivenessTest", 1, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.PopTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.PrintTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.PutfieldTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.PutstaticPatchingTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.PutstaticTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.RegisterAllocatorTortureTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.StackTraceTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386" ] )
, ( "jvm.StringTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.SubroutineTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.SwitchTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.SynchronizationExceptionsTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.SynchronizationTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.TrampolineBackpatchingTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.VirtualAbstractInterfaceMethodTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.WideTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "test.java.lang.ClassTest", 0, [ ], [ "i386", "x86_64" ] )
, ( "test.java.lang.DoubleTest", 0, [ ], [ "i386", "x86_64" ] )
, ( "test.java.lang.JNITest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "test.java.lang.ref.ReferenceTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "test.java.lang.reflect.FieldAccessorsTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "test.java.lang.reflect.FieldTest", 0, [ ], [ "i386", "x86_64" ] )
, ( "test.java.lang.reflect.MethodTest", 0, [ ], [ "i386", "x86_64" ] )
, ( "test.java.util.HashMapTest", 0, [ ], [ "i386", "x86_64" ] )
, ( "test.sun.misc.UnsafeTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "corrupt.CorruptedExceptionTableEndsAfterCode", 1, [ ], [ "i386", "x86_64" ] )
, ( "corrupt.CorruptedExceptionTableInvalidHandlerPC", 1, [ ], [ "i386", "x86_64" ] )
, ( "corrupt.CorruptedExceptionTableInvertedBorns", 1, [ ], [ "i386", "x86_64" ] )
, ( "corrupt.CorruptedFallingOff", 1, [ ], [ "i386", "x86_64" ] )
, ( "corrupt.CorruptedIncompleteInsn", 1, [ ], [ "i386", "x86_64" ] )
, ( "corrupt.CorruptedInvalidBranchNeg", 1, [ ], [ "i386", "x86_64" ] )
, ( "corrupt.CorruptedInvalidBranchNotOnInsn", 1, [ ], [ "i386", "x86_64" ] )
, ( "corrupt.CorruptedInvalidBranchOut", 1, [ ], [ "i386", "x86_64" ] )
, ( "corrupt.CorruptedInvalidOpcode", 1, [ ], [ "i386", "x86_64" ] )
, ( "corrupt.CorruptedLoadConstantDouble", 1, [ ], [ "i386", "x86_64" ] )
, ( "corrupt.CorruptedLoadConstantIndex", 1, [ ], [ "i386", "x86_64" ] )
, ( "corrupt.CorruptedLoadConstantSimple", 1, [ ], [ "i386", "x86_64" ] )
, ( "corrupt.CorruptedMaxLocalVar", 1, [ ], [ "i386", "x86_64" ] )
]

def guess_arch():
  arch = platform.machine()
  if arch == "i686":
    return "i386"
  return arch

ARCH = guess_arch()

def is_test_supported(t):
  klass, expected_retval, extra_args, archs = t
  return ARCH in archs

def success(s):
  return "\033[32m" + s + "\033[0m"

def failure(s):
  return "\033[31m" + s + "\033[0m"

def skipped_color(s):
  return "\033[33m" + s + "\033[0m"

def run(program, *args):
  pid = os.fork()
  if not pid:
    os.execvp(program, (program,) +  args)
  return os.wait()[0]

def progress(index, total, t):
  percentage = index*100/total
  s = "%#3d%% [%#d/%d]  %-60s" % (percentage, index, total, t)
  sys.stdout.write(s + '\r')
  sys.stdout.flush()

def main():
  optparser = argparse.ArgumentParser("Run Jato functional tests.")
  optparser.add_argument("-s", dest="skipped", action="store_true",
                         help="check which skipped tests actually pass")
  opts = optparser.parse_args()

  results = Queue()

  tests = filter(lambda t: is_test_supported(t) != opts.skipped, TESTS)

  def do_work(t):
    klass, expected_retval, extra_args, archs = t
    fnull = open(os.devnull, "w")
    progress(len(tests) - q.qsize(), len(tests), klass)
    command = ["./jato", "-cp", TEST_DIR ] + extra_args + [ klass ]
    retval = subprocess.call(command, stderr = fnull)
    if retval != expected_retval:
      if not opts.skipped:
        print "%s: Test FAILED%20s" % (klass, "")
      results.put(False)
    else:
      if opts.skipped:
        print "%s: Test SUCCEEDED%20s" % (klass, "")
      results.put(True)

  def worker():
    while True:
      item = q.get()
      do_work(item)
      q.task_done()

  q = Queue()
  for i in range(multiprocessing.cpu_count()):
    t = Thread(target=worker)
    t.daemon = True
    t.start()

  start = time.time()
  for t in tests:
    q.put(t)

  q.join()

  print

  passed = failed = 0
  while results.qsize() > 0:
    result = results.get()
    if result:
      passed += 1
    else:
      failed += 1

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

  skipped = len(TESTS) - len(tests)
  if skipped > 0:
    status += ", " + skipped_color("%d tests skipped" % skipped)

  print "%s (%.2f s) " % (status, elapsed)
  sys.exit(failed)

if __name__ == '__main__':
  main()
