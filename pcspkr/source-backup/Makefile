### Makefile for node-finder module
KDIR ?= /lib/modules/`uname -r`/build

default:
	$(MAKE) -C $(KDIR) M=$$PWD # Initialise the Kbuild process.
	# Now clean up the directory- comment this out if needed:
	find . -type f \
		\( -not \( \
			-name "*.c" -or \
			-name "*.ko" -or \
			-name "Kbuild" -or \
			-name "Makefile" \
		\) -or \
		-name "*.mod.c" \) \
	-delete
	# (End of cleanup command to be commented)
