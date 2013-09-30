QSound core for Highly Quixotic


Another dirty secret (gasp), the Z80 core is based on the one from MAME.
Of course it required extensive modification to make it work outside MAMEland.

Opcodes have a separate map so that we can stick a Kabuki-decrypted mirror
underneath the first 32K and execute opcodes from that.


kabuki.c - Kabuki decryption code.

qmix.c - QSound mixer code. Most of the sound emulation happens here.

qsound.c - top-level QSound system emulation.

z80.c - Z80 core.
