.class public jvm/SubroutineTest
.super jvm/TestCase

.method public static singleSubroutine()I
    .limit locals 2

    jsr sub1
    iload 0
    ireturn
sub1:
    astore 1
    iconst_1
    istore 0
    ret 1
.end method

.method public static testSingleSubroutine()V
    .limit stack 64

    invokestatic jvm/SubroutineTest/singleSubroutine()I
    iconst_1
    invokestatic jvm/TestCase/assertEquals(II)V
    return
.end method

.method public static testWideInstructions()V
    .limit locals 2

    jsr_w subroutine
    return

subroutine:
    astore 1
    ret 1
.end method

; This test checks correct exception handler range split When JSR S
; instruction is protected by an exception handler which does not
; protect the S body then exception handler range must be split so
; that it does not cover the JSR instruction. Otherwise exception
; thrown from S could be incorrectly caught by handler protecting
; the JSR instruction.
.method public static testExceptionRangeSplit()V
    .limit stack 64
    .limit locals 2
    .catch java/lang/RuntimeException from c_start to c_end using handler
    .catch java/lang/Exception from sub_start to sub_end using sub_handler

c_start:
    jsr_w subroutine
c_end:
    return
handler:
    pop
    invokestatic jvm/TestCase/fail()V
    return

subroutine:
    astore 1
sub_start:
    new java/lang/RuntimeException
    dup
    invokespecial java/lang/RuntimeException/<init>()V
    athrow
sub_end:
    goto sub_out
sub_handler:
    pop
sub_out:
    ret 1
.end method

; This method contains 3 nested subroutines.
.method public static testNestedSubroutines()V
    .limit locals 7

    iconst_0
    istore 4
    iconst_0
    istore 5
    iconst_0
    istore 6

    jsr s1
    jsr s2
    goto l1
s1:
    astore 1
    iinc 4 1
    jsr s2
    jsr s3
    goto s1_out
s2:
    astore 2
    iinc 5 1
    jsr s3
    goto s2_out
s3:
    astore 3
    iinc 6 1
    ret 3
s2_out:
    ret 2
s1_out:
    ret 1

l1:
    jsr s3

    ; S1 counter
    iload 4
    iconst_1
    invokestatic jvm/TestCase/assertEquals(II)V

    ; S2 counter
    iload 5
    iconst_2
    invokestatic jvm/TestCase/assertEquals(II)V

    ; S3 counter
    iload 6
    iconst_4
    invokestatic jvm/TestCase/assertEquals(II)V

    return
.end method

.method public static main([Ljava/lang/String;)V
    invokestatic jvm/SubroutineTest/testSingleSubroutine()V
    invokestatic jvm/SubroutineTest/testWideInstructions()V
    invokestatic jvm/SubroutineTest/testExceptionRangeSplit()V
    invokestatic jvm/SubroutineTest/testNestedSubroutines()V
    return
.end method
