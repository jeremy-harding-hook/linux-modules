### Makefile for node-finder module
KDIR ?= /lib/modules/`uname -r`/build

default:
	$(MAKE) -C $(KDIR) M=$$PWD # Initialise the Kbuild process.

clean:
	$(MAKE) -C $(KDIR) M=$$PWD clean # Clean up the source directories.
