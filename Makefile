compile:
	$(MAKE) -f scripts/Makefile.compile obj=src/
	$(MAKE) -f scripts/Makefile.compile obj=vm/

clean:
	$(MAKE) -f scripts/Makefile.clean obj=src/
	$(MAKE) -f scripts/Makefile.clean obj=vm/
