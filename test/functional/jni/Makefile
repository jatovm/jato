all:
	$(CC) -shared -fpic -o libjnitest.so -I ../../../include jnitest.c
	$(JAVAC) -source 1.6 -target 1.6 -cp . JNITestFixture.java

clean:
	rm -f *.o *.so *.class
