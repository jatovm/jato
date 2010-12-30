.class public jvm/InvokeTest
.super jvm/TestCase

.field public static counter I

.method public static advanceCounter()I
    .limit stack 128
    getstatic jvm/InvokeTest/counter I
    iconst_1
    iadd
    dup
    putstatic jvm/InvokeTest/counter I
    ireturn
.end method

.method public static main([Ljava/lang/String;)V
    .limit stack 3
    .limit locals 1

    iconst_0
    putstatic jvm/InvokeTest/counter I

    invokestatic jvm/InvokeTest/advanceCounter()I

    getstatic jvm/InvokeTest/counter I
    iconst_1
    iadd
    putstatic jvm/InvokeTest/counter I

    iconst_1
    swap
    invokestatic jvm/TestCase/assertEquals(II)V
    return
.end method
