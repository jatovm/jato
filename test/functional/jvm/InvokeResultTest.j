.class public jvm/InvokeResultTest
.super jvm/TestCase

.method public static byteMethod()B
    .limit stack 1
    .limit locals 0

    bipush -77
    ireturn
.end method

.method public static shortMethod()S
    .limit stack 1
    .limit locals 0

    sipush -7777
    ireturn
.end method

.method public static charMethod()C
    .limit stack 1
    .limit locals 1

    ldc 65535
    ireturn
.end method

.method public static booleanMethod()Z
    .limit stack 1
    .limit locals 0

    bipush 1
    ireturn
.end method

.method public static testByteResult()V
    .limit stack 2
    .limit locals 0

    bipush -77
    invokestatic jvm/InvokeResultTest/byteMethod()B
    i2s
    invokestatic jvm/TestCase/assertEquals(II)V
    return
.end method

.method public static testShortResult()V
    .limit stack 2
    .limit locals 0

    sipush -7777
    invokestatic jvm/InvokeResultTest/shortMethod()S
    i2s
    invokestatic jvm/TestCase/assertEquals(II)V
    return
.end method

.method public static testCharResult()V
    .limit stack 2
    .limit locals 0

    ldc 65535
    invokestatic jvm/InvokeResultTest/charMethod()C
    i2c
    invokestatic jvm/TestCase/assertEquals(II)V
    return
.end method

.method public static testBooleanResult()V
    .limit stack 2
    .limit locals 0

    bipush 1
    invokestatic jvm/InvokeResultTest/booleanMethod()Z
    i2s
    invokestatic jvm/TestCase/assertEquals(II)V
    return
.end method

.method public static main([Ljava/lang/String;)V
    .limit stack 1
    .limit locals 1

    invokestatic jvm/InvokeResultTest/testByteResult()V
    invokestatic jvm/InvokeResultTest/testBooleanResult()V
    invokestatic jvm/InvokeResultTest/testCharResult()V
    invokestatic jvm/InvokeResultTest/testShortResult()V
    return
.end method
