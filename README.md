# linux-modules
A repo to contain my linux modules, modified drivers, etc.

This is mainly for holding customisations I make for my pc, although if I ever
have any of significant size or quality I'll break them out into their own
repositories.

The project this repo was originally created for is one to make my power button
flash to replace the bell sound. Because I wanted a universal solution that
worked no matter what rang the bell, I decided modifying the pcspkr driver was
the way to go. On top of this in order to access the specific led object
belonging to the power button I needed to modify the thinkpad\_acpi driver as
well. The downside of this approach is obviously that it requires running a
tainted kernel. I haven't seen any other downsides yet though.

To build the modules, just call "make" (potentially using "make clean"
beforehand). If there are errors, that probably means a kernel update broke it
and the changes should probably just be applied anew to the latest source for
the original modules in the case of a module modification, or just fix the
module if it's an original.

In order to install the modules assuming a fairly normal system (the author's
machine runs essentially out-of-the-box Arch), just copy the built modules into:

    /usr/lib/modules/$(uname -r)/updates

And then call:

    mkinitcpio -p linux

in order to rebuild the initcpio images. This is necessary because modules that
are loaded early on such as thinkpad\_acpi will otherwise not have the updated
version loaded. (Also remember to do any post-image-generation handling; in the
author's case, that means copying the images to the /efi directory after
mounting it.)

You might need to compress the modules using gzip first; this is what I've done
in the past and it works fine, but it may work even without compression. Note
you will have to repeat this every time you update the kernel, unless and until
you figure out some way around it.

I don't expect that anyone will find this repo useful in any way, but I figured
I'd make it public on the off-chance.
