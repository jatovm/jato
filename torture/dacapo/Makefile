JAVA	?= ../../jato

JAR_URL	= http://downloads.sourceforge.net/project/dacapobench/archive/2006-10-MR2/dacapo-2006-10-MR2.jar?use_mirror=switch
JAR	= dacapo-2006-10-MR2.jar

all: test
.PHONY: all

test: $(JAR)
	$(JAVA) -Xmx1g -jar $(JAR) antlr
.PHONY: test

$(JAR):
	wget $(JAR_URL)
