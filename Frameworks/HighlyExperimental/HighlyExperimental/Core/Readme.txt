PSX emulation core - PS1/PS2

This core emulates the PS2 IOP, which can conveniently be dropped into PS1
compatibility mode to play PS1 sound as well.

The unfortunate dirty secret here is that Sony BIOS is used.  Originally this
was the plain 512K SCPH1001 image we all know and love.  But when PS2 support
was added, starting with core version 0007 (HE 1.08), a new technique was used.
A PS2 BIOS image (5.0 North American) is stripped module-by-module to just the
necessary code for sound playing, plus PS1 compatibility (TBIN/SBIN).  The
result is about 350K smaller when compressed.

Making this stuff 100% legal (via IOP kernel and PS1 BIOS HLE) is on the to-do
list.


mkhebios - This directory contains the tools for generating the new-style PS2
  BIOS images.

he - Source code for HE IOP module.  This is the code that handles "hefile:"
  requests.  Compiled into an .IRX file and then included in the mkhebios
  process.


SHTest - A system for testing the emulation core for byte order / alignment
  issues, by compiling it to a sh-arm-elf target and then running it in a simple
  emulator.  Slow, but effective.


bios.c - when compiled, contains the BIOS image.  Also includes some environment
  variable-like stuff (can be set in mkhebios).

iop.c - IOP emulation (mostly just glue between other modules)

ioptimer.c - IOP timers (root counters), both the 16-bit PS1 style and 32-bit
  PS2 style.

psx.c - top-level PS1/PS2 emulation.

r3000.c - R3000 core. all C, all slow (though with the wait loop detection,
  this hasn't been a big deal).

r3000asm.c - R3000 quick assembler as used in PSFLab.

r3000dis.c - R3000 disassembler (also as used in PSFLab).

spu.c - SPU1 or SPU2 emulation.

spucore.c - Emulates one SPU core (24 channels).  PS2 has a pair of these.

vfs.c - Virtual PSF2 filesystem stuff.
