JAVA	?= ../../jato

TAR_URL = http://jruby.org.s3.amazonaws.com/downloads/1.5.0.RC3/jruby-bin-1.5.0.RC3.tar.gz
TAR	= jruby-bin-1.5.0.RC3.tar.gz
JAR	= jruby-1.5.0.RC3/lib/jruby.jar

all: test
.PHONY: all

test: $(JAR)
	$(JAVA) -jar $(JAR) hello.rb
.PHONY: test

$(JAR): $(TAR)
	tar xf $<

$(TAR):
	wget $(TAR_URL)
