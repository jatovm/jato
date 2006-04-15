MONOBURG=./monoburg/monoburg

all: compile

compile: gen
	$(MAKE) -f scripts/Makefile.compile obj=jit/
	$(MAKE) -f scripts/Makefile.compile obj=src/
	$(MAKE) -f scripts/Makefile.compile obj=vm/

gen:
	$(MONOBURG) -p -e jit/insn-selector.brg > jit/insn-selector.c

clean:
	$(MAKE) -f scripts/Makefile.clean obj=jit/
	$(MAKE) -f scripts/Makefile.clean obj=src/
	$(MAKE) -f scripts/Makefile.clean obj=vm/
