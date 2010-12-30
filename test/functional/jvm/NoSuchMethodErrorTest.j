.class public jvm/NoSuchMethodErrorTest
.super jvm/TestCase

.method public static testMissingStaticMethod()V
    .catch java/lang/NoSuchMethodError from c_start to c_end using handler

c_start:
    invokestatic jvm/NoSuchMethodErrorTest/missingStaticMethod()V
c_end:
    invokestatic jvm/TestCase/fail()V
    return

handler:
    pop
    return
.end method

.method public static main([Ljava/lang/String;)V
    invokestatic jvm/NoSuchMethodErrorTest/testMissingStaticMethod()V
    return
.end method
