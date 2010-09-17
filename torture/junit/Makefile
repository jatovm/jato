JAVA		?= ../../jato
JAVAC		?= javac

JAR_URL		= http://downloads.sourceforge.net/project/junit/junit/4.8.1/junit-4.8.1.jar
JAR		= junit-4.8.1.jar

TEST_CLASSES	= SimpleTest.class
RUNNER		= junit.textui.TestRunner

all: test
.PHONY: all

test: $(JAR) $(TEST_CLASSES)
	$(E) "  JAVA    " $(RUNNER)
	$(Q) $(JAVA) -cp $(JAR):. $(RUNNER) SimpleTest
.PHONY: test

$(TEST_CLASSES): %.class: %.java
	$(E) "  JAVAC   " $@
	$(Q) $(JAVAC) -cp $(JAR) $<

$(JAR):
	$(E) "  WGET    " $@
	$(Q) wget $(JAR_URL)

include ../../scripts/build/common.mk
