.class public jvm/PopTest
.super jvm/TestCase

.method public static testPop()V
    .limit stack 128
    iconst_0
    iconst_0

    iconst_1
    pop

    invokestatic jvm/TestCase/assertEquals(II)V

    return
.end method

.method public static testIntegerPop2()V
    .limit stack 128
    iconst_0
    iconst_0

    iconst_1
    iconst_1

    pop2

    invokestatic jvm/TestCase/assertEquals(II)V

    return
.end method

; TODO: Must raise VerifyError
;.method public static testOneIntegerPop2()V
;    .limit stack 128
;    iconst_0
;
;    pop2
;
;    return
;.end method

.method public static testLongPop2()V
    .limit stack 128
    lconst_0
    lconst_0

    lconst_1
    pop2

    invokestatic jvm/TestCase/assertEquals(JJ)V

    return
.end method

.method public static main([Ljava/lang/String;)V
    invokestatic jvm/PopTest/testPop()V
    invokestatic jvm/PopTest/testIntegerPop2()V
;   invokestatic jvm/PopTest/testOneIntegerPop2()V
    invokestatic jvm/PopTest/testLongPop2()V
    return
.end method
