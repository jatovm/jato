JAVA ?= ../../jato

all: clojure dacapo junit scimark
.PHONY: all

clojure:
	make -C clojure JAVA=$(JAVA)
.PHONY: clojure

dacapo:
	make -C dacapo JAVA=$(JAVA)
.PHONY: dacapo

junit:
	make -C junit JAVA=$(JAVA)
.PHONY: junit

scimark:
	make -C scimark JAVA=$(JAVA)
.PHONY: scimark
