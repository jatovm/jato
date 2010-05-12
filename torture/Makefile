JAVA ?= ../../jato

all: clojure dacapo junit scimark
.PHONY: all

clojure:
	make -C clojure JAVA=$(JAVA)
.PHONY: clojure

dacapo:
	make -C dacapo JAVA=$(JAVA)
.PHONY: dacapo

jruby:
	make -C jruby JAVA=$(JAVA)
.PHONY: jruby

junit:
	make -C junit JAVA=$(JAVA)
.PHONY: junit

scimark:
	make -C scimark JAVA=$(JAVA)
.PHONY: scimark
