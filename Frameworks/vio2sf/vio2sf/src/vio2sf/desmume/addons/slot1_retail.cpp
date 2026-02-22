/*  Copyright (C) 2010-2011 DeSmuME team

    This file is part of DeSmuME

    DeSmuME is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    DeSmuME is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with DeSmuME; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include "../vio2sf.h"

static void info(vio2sf_state *, char *info) { strcpy(info, "Slot1 Retail card emulation"); }
static void config(vio2sf_state *) {}

static bool init(vio2sf_state *) { return true; }

static void reset(vio2sf_state *) {}

static void close(vio2sf_state *) {}

static void write08(vio2sf_state *, uint8_t, uint32_t, uint8_t) {}
static void write16(vio2sf_state *, uint8_t, uint32_t, uint16_t) {}

static void write32_GCROMCTRL(vio2sf_state *st, uint8_t PROCNUM, uint32_t)
{
	nds_dscard &card = st->MMU.dscard[PROCNUM];

	switch (card.command[0])
	{
		case 0x00: // Data read
		case 0xB7:
			card.address = (card.command[1] << 24) | (card.command[2] << 16) | (card.command[3] << 8) | card.command[4];
			card.transfer_count = 0x80;
			break;

		case 0xB8: // Chip ID
			card.address = 0;
			card.transfer_count = 1;
			break;

		default:
			card.address = 0;
			card.transfer_count = 0;
	}
}

static void write32(vio2sf_state *st, uint8_t PROCNUM, uint32_t adr, uint32_t val)
{
	switch (adr)
	{
		case REG_GCROMCTRL:
			write32_GCROMCTRL(st, PROCNUM, val);
	}
}

static uint8_t read08(vio2sf_state *, uint8_t, uint32_t)
{
	return 0xFF;
}

static uint16_t read16(vio2sf_state *, uint8_t, uint32_t)
{
	return 0xFFFF;
}

static uint32_t read32_GCDATAIN(vio2sf_state *st, uint8_t PROCNUM)
{
	nds_dscard &card = st->MMU.dscard[PROCNUM];

	switch (card.command[0])
	{
		// Get ROM chip ID
		case 0x90:
		case 0xB8:
			{
				// Note: the BIOS stores the chip ID in main memory
				// Most games continuously compare the chip ID with
				// the value in memory, probably to know if the card
				// was removed.
				// As DeSmuME normally boots directly from the game, the chip
				// ID in main mem is zero and this value needs to be
				// zero too.

				// note that even if desmume was booting from firmware, and reading this chip ID to store in main memory,
				// this still works, since it will have read 00 originally and then read 00 to validate.

				// staff of kings verifies this (it also uses the arm7 IRQ 20)
				if (st->nds.cardEjected) // TODO - handle this with ejected card slot1 device (and verify using this case)
					return 0xFFFFFFFF;
				else
					return 0;
			}

		// Data read
		case 0x00:
		case 0xB7:
			{
				// it seems that etrian odyssey 3 doesnt work unless we mask this to cart size.
				// but, a thought: does the internal rom address counter register wrap around? we may be making a mistake by keeping the extra precision
				// but there is no test case yet
				uint32_t address = card.address & st->gameInfo.mask;

				// Make sure any reads below 0x8000 redirect to 0x8000+(adr&0x1FF) as on real cart
				if (card.command[0] == 0xB7 && address < 0x8000)
				{
					// TODO - refactor this to include the PROCNUM, for debugging purposes if nothing else
					// (can refactor gbaslot also)

					//INFO("Read below 0x8000 (0x%04X) from: ARM%s %08X\n", card.address, (PROCNUM ? "7":"9"), (PROCNUM ? NDS_ARM7:NDS_ARM9).instruct_adr);

					address = 0x8000 + (address & 0x1FF);
				}

				// as a sanity measure for funny-sized roms (homebrew and perhaps truncated retail roms)
				// we need to protect ourselves by returning 0xFF for things still out of range
				if (address >= st->gameInfo.romsize)
				{
					//DEBUG_Notify.ReadBeyondEndOfCart(address, gameInfo. romsize);
					return 0xFFFFFFFF;
				}

				return T1ReadLong(st->MMU.CART_ROM, address);
			}
		default:
			return 0;
	} //switch(card.command[0])
} //read32_GCDATAIN

static uint32_t read32(vio2sf_state *st, uint8_t PROCNUM, uint32_t adr)
{
	switch (adr)
	{
		case REG_GCDATAIN:
			return read32_GCDATAIN(st, PROCNUM);
		default:
			return 0;
	}
}

SLOT1INTERFACE slot1Retail =
{
	"Retail",
	init,
	reset,
	close,
	config,
	write08,
	write16,
	write32,
	read08,
	read16,
	read32,
	info
};
