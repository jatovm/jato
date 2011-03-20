#!/usr/bin/env python

from threading import Thread
from Queue import Queue
import multiprocessing
import subprocess
import platform
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
, ( "jvm/ExitStatusIsZeroTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm/ExitStatusIsOneTest", 1, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm/ArgsTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "java.lang.JNITest", 0, NO_SYSTEM_CLASSLOADER, [ "i386" ] )
, ( "java.lang.reflect.ClassTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "java.lang.reflect.MethodTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.ArrayExceptionsTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386" ] )
, ( "jvm.ArrayMemberTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.ArrayTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386" ] )
, ( "jvm.BranchTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.CFGCrashTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.ClinitFloatTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.ClassExceptionsTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.ClassLoaderTest", 0, [ ], [ "i386" ] )
, ( "jvm.CloneTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.ControlTransferTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.ConversionTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386" ] )
, ( "jvm.DoubleArithmeticTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386" ] )
, ( "jvm.DoubleConversionTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386" ] )
, ( "jvm.DupTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386" ] )
, ( "jvm.ExceptionsTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386" ] )
, ( "jvm.ExceptionHandlerTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.FibonacciTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.FinallyTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.FloatArithmeticTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386" ] )
, ( "jvm.FloatConversionTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386" ] )
, ( "jvm.GcTortureTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.GetstaticPatchingTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.IntegerArithmeticExceptionsTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.IntegerArithmeticTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386" ] )
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
, ( "jvm.MethodInvokeVirtualTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386" ] )
, ( "jvm.MethodInvocationExceptionsTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.MultithreadingTest", 0, [ ], [ "i386" ] )
, ( "jvm.NoSuchMethodErrorTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.ObjectArrayTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.ObjectCreationAndManipulationExceptionsTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.ObjectCreationAndManipulationTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386" ] )
, ( "jvm.ObjectStackTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.ParameterPassingTest", 100, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.PopTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.PrintTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386" ] )
, ( "jvm.PutfieldTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386" ] )
, ( "jvm.PutstaticPatchingTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386" ] )
, ( "jvm.PutstaticTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386" ] )
, ( "jvm.RegisterAllocatorTortureTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.StackTraceTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386" ] )
, ( "jvm.StringTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.SubroutineTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.SwitchTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386" ] )
, ( "jvm.SynchronizationExceptionsTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.SynchronizationTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386" ] )
, ( "jvm.TrampolineBackpatchingTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.VirtualAbstractInterfaceMethodTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.WideTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386", "x86_64" ] )
, ( "jvm.lang.reflect.FieldTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386" ] )
, ( "sun.misc.UnsafeTest", 0, NO_SYSTEM_CLASSLOADER, [ "i386" ] )
, ( "test.java.lang.ClassTest", 0, [ ], [ "i386" ] )
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
  results = Queue()

  tests = filter(is_test_supported, TESTS)

  def do_work(t):
    klass, expected_retval, extra_args, archs = t
    progress(len(tests) - q.qsize(), len(tests), klass)
    command = ["./jato", "-cp", TEST_DIR ] + extra_args + [ klass ]
    retval = subprocess.call(command)
    if retval != expected_retval:
      print klass + ": Test FAILED"
      results.put(False)
    else:
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

  print "%s (%.2f s) " % (status, elapsed)
  sys.exit(failed)

if __name__ == '__main__':
  main()
