JAVA	?= ../../jato

JAR_URL = http://math.nist.gov/scimark2/scimark2lib.jar
JAR	= scimark2lib.jar

all: test
.PHONY: all

test: $(JAR)
	$(JAVA) -cp $(JAR) jnt.scimark2.commandline
.PHONY: test

$(JAR):
	wget $(JAR_URL)
