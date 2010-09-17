JAVA	?= ../../jato

JAR_URL	= http://downloads.sourceforge.net/project/dacapobench/archive/2006-10-MR2/dacapo-2006-10-MR2.jar
JAR	= dacapo-2006-10-MR2.jar

all: test
.PHONY: all

test: $(JAR)
	$(JAVA) -Xmx1g -jar $(JAR) antlr
	$(JAVA) -Xmx1g -jar $(JAR) bloat
	$(JAVA) -Xmx1g -jar $(JAR) chart
	$(JAVA) -Xmx1g -jar $(JAR) eclipse
	$(JAVA) -Xmx1g -jar $(JAR) fop
	$(JAVA) -Xmx1g -jar $(JAR) hsqldb
	$(JAVA) -Xmx1g -jar $(JAR) luindex
	$(JAVA) -Xmx1g -jar $(JAR) lusearch
	$(JAVA) -Xmx1g -jar $(JAR) pmd
	$(JAVA) -Xmx1g -jar $(JAR) xalan
# BROKEN:
#	$(JAVA) -Xmx1g -jar $(JAR) jython

.PHONY: test

$(JAR):
	wget $(JAR_URL)
