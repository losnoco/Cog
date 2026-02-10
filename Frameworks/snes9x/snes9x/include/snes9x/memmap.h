/*****************************************************************************\
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#pragma once

#include <memory>
#include <cstdint>

inline constexpr int32_t MEMMAP_BLOCK_SIZE = 0x1000;
inline constexpr size_t MEMMAP_NUM_BLOCKS = 0x1000000 / MEMMAP_BLOCK_SIZE;
inline constexpr uint32_t MEMMAP_SHIFT = 12;
inline constexpr uint32_t MEMMAP_MASK = MEMMAP_BLOCK_SIZE - 1;

struct CMemory
{
    struct S9xState *st;

	enum { MAX_ROM_SIZE = 0x800000 };

	enum
	{
		NOPE,
		YEAH,
		BIGFIRST,
		SMALLFIRST
	};

    enum
	{
		MAP_TYPE_I_O,
		MAP_TYPE_RAM
	};

	enum
	{
		MAP_CPU,
		MAP_PPU,
		MAP_LOROM_SRAM,
		MAP_HIROM_SRAM,
		MAP_SA1RAM,
		MAP_RONLY_SRAM,
		MAP_NONE,
		MAP_LAST
	};

	std::unique_ptr<uint8_t[]> RAM;
	std::unique_ptr<uint8_t[]> RealROM;
	uint8_t *ROM;
	std::unique_ptr<uint8_t[]> SRAM;
	std::unique_ptr<uint8_t[]> VRAM;
	uint8_t *FillRAM;

	uint8_t *Map[MEMMAP_NUM_BLOCKS];
	uint8_t *WriteMap[MEMMAP_NUM_BLOCKS];
	uint8_t BlockIsRAM[MEMMAP_NUM_BLOCKS];
	uint8_t BlockIsROM[MEMMAP_NUM_BLOCKS];
	uint8_t ExtendedFormat;

	char ROMName[ROM_NAME_LEN];
	char ROMId[5];
	uint8_t ROMRegion;
	uint8_t ROMSpeed;
	uint8_t ROMType;

	bool HiROM;
	bool LoROM;
	uint8_t SRAMSize;
	uint32_t SRAMMask;
	uint32_t CalculatedSize;

	std::unique_ptr<char[]> safe;
	int safe_len;

	bool Init(struct S9xState *st);
	void Deinit();

	int ScoreHiROM(bool, int32_t romoff = 0);
	int ScoreLoROM(bool, int32_t romoff = 0);
	int First512BytesCountZeroes() const;
	bool LoadROMSNSF(const uint8_t *, int32_t, const uint8_t *, int32_t);

	char *Safe(const char *);
	void ParseSNESHeader(uint8_t *);
	void InitROM();

	ssize_t map_mirror(uint32_t, uint32_t);
	void map_lorom(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
	void map_hirom(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
	void map_lorom_offset(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
	void map_hirom_offset(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
	void map_space(uint32_t, uint32_t, uint32_t, uint32_t, uint8_t *);
	void map_index(uint32_t, uint32_t, uint32_t, uint32_t, int, int);
	void map_System();
	void map_WRAM();
	void map_LoROMSRAM();
	void map_HiROMSRAM();
	void map_WriteProtectROM();
	void Map_Initialize();
	void Map_LoROMMap();
	void Map_NoMAD1LoROMMap();
	void Map_JumboLoROMMap();
	void Map_ROM24MBSLoROMMap();
	void Map_SRAM512KLoROMMap();
	void Map_SDD1LoROMMap();
	void Map_HiROMMap();
	void Map_ExtendedHiROMMap();

	bool match_na(const char *);
	bool match_nn(const char *);
	bool match_id(const char *);
	void ApplyROMFixes();
};

enum s9xwrap_t
{
	WRAP_NONE,
	WRAP_BANK,
	WRAP_PAGE
};

enum s9xwriteorder_t
{
	WRITE_01,
	WRITE_10
};

#include <snes9x/getset.h>
