all:
	$(CC) -shared -fpic -o libjnitest.so -I ../../../include jnitest.c
	$(JAVAC) -source 1.5 -cp . JNITestFixture.java

clean:
	rm -f *.o *.so *.class
