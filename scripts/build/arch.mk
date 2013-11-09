ifneq ($(ARCH),$(uname_M))
TEST		=
else
TEST		= test
endif

ifeq ($(ARCH),i386)
	override ARCH	= x86
	ARCH_POSTFIX	= _32
	#WARNINGS	+= -Werror
	MB_DEFINES	+= -DCONFIG_X86_32
	ifeq ($(uname_M),x86_64)
		ARCH_CFLAGS	+= -m32
		TEST		= test
	endif
endif

ifeq ($(ARCH),x86_64)
	override ARCH	= x86
	ARCH_POSTFIX	= _64
	ARCH_CFLAGS	+= -fno-omit-frame-pointer
	MB_DEFINES	+= -DCONFIG_X86_64
endif

ifeq ($(SYS),darwin)
	DEFAULT_CFLAGS += -D_XOPEN_SOURCE=1
endif
export ARCH_CFLAGS

