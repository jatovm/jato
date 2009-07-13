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
    invokestatic jvm/SubroutineTest/singleSubroutine()I
    iconst_1
    invokestatic jvm/TestCase/assertEquals(II)V
.end method

.method public static main([Ljava/lang/String;)V
    invokestatic jvm/SubroutineTest/testSingleSubroutine()V
    return
.end method
