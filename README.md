# linux-modules
A repo to contain my linux modules, modified drivers, etc.

This is mainly for holding customisations I make for my pc, although if I ever have any of significant size or quality I'll break them out into their own repositories.

The project this repo was originally created for is one to make my power button flash to replace the bell sound. Because I wanted a universal solution that worked no matter what rang the bell, I decided modifying the pcspkr driver was the way to go. On top of this in order to access the specific led object belonging to the power button I needed to modify the thinkpad_acpi driver as well. The downside of this approach is obviously that it requires running a tainted kernel. I haven't seen any other downsides yet though.

I don't expect that anyone will find this repo useful in any way, but I figured I'd make it public on the off-chance.
