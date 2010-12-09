.class public jvm/ExceptionHandlerTest
.super jvm/TestCase

; This test tests a method that has two exception handlers that start at
; different offsets but share some bytecode.
.method public static testSharedExceptionHandler()V
    .limit stack 64
    .limit locals 2
    .catch java/lang/RuntimeException from c_start to c_end using handler_1
    .catch java/lang/Exception from c_start to c_end using handler_2

c_start:
    new java/lang/RuntimeException
    dup
    invokespecial java/lang/RuntimeException/<init>()V
    athrow
    invokestatic jvm/TestCase/fail()V
c_end:
    return
handler_1:
    nop
handler_2:
    pop
    return
.end method

.method public static main([Ljava/lang/String;)V
    invokestatic jvm/ExceptionHandlerTest/testSharedExceptionHandler()V
    return
.end method
