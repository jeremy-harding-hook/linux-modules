### Makefile for node-finder module
KDIR ?= /lib/modules/`uname -r`/build

default:
	$(MAKE) -C $(KDIR) M=$$PWD # Initialise the Kbuild process.

clean:
	find . -maxdepth 1 -type f \
		\( -not \( \
			-name "*.c" -or \
			-name "*.h" -or \
			-name "*.ko" -or \
			-name "Kbuild" -or \
			-name "Makefile" -or \
			-name "Doxyfile" \
		\) -or \
		-name "*.mod.c" -or \
		-name "*.mod.h" \) \
	-delete
