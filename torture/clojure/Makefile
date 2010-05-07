JAVA	?= ../../jato

ZIP_URL	= http://clojure.googlecode.com/files/clojure-1.1.0.zip
ZIP	= clojure-1.1.0.zip
JAR	= clojure-1.1.0/clojure.jar

all: test
.PHONY: all

test: $(JAR)
	$(JAVA) -cp $(JAR) clojure.main hello.clj
.PHONY: test

$(ZIP):
	wget $(ZIP_URL)

$(JAR): $(ZIP)
	unzip -n $<
