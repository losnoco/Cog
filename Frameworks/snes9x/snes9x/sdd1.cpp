/*****************************************************************************\
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#include <algorithm>
#include "snes9x.h"

#include <snes9x/snes.hpp>
#include <snes9x/smp.hpp>
#include <snes9x/sdsp.hpp>

void S9xSetSDD1MemoryMap(struct S9xState *st, uint32_t bank, uint32_t value)
{
	bank = 0xc00 + bank * 0x100;
	value *= 1024 * 1024;

	for (int c = 0; c < 0x100; c += 16)
	{
		uint8_t *block = &st->Memory.ROM[value + (c << 12)];
		std::fill_n(&st->Memory.Map[bank + c], 16, block);
	}
}

void S9xResetSDD1(struct S9xState *st)
{
	std::fill_n(&st->Memory.FillRAM[0x4800], 4, 0);
	int i = 0;
	std::generate_n(&st->Memory.FillRAM[0x4804], 4, [&]() -> int { S9xSetSDD1MemoryMap(st, i, i); return i++; });
}
