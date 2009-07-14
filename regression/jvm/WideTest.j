.class public jvm/WideTest
.super jvm/TestCase

.method public static testIstoreIload()V
    .limit locals 257

    iconst_0
    istore 0

    iconst_3
    istore_w 256
    iload_w 256

    iconst_3
    invokestatic jvm/TestCase/assertEquals(II)V
    return
.end method

.method public static testIinc()V
    .limit locals 257

    iconst_0
    istore 0

    iconst_3
    istore_w 256

    iinc_w 256 1
    iload_w 256

    iconst_4
    invokestatic jvm/TestCase/assertEquals(II)V
    return
.end method

.method public static testGoto()V
    goto_w lab1
lab1:
    return
.end method

.method public static main([Ljava/lang/String;)V
    invokestatic jvm/WideTest/testIstoreIload()V
    invokestatic jvm/WideTest/testIinc()V
    invokestatic jvm/WideTest/testGoto()V
    return
.end method
