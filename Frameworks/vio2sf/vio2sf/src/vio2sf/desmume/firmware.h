/*
	Copyright (C) 2009-2011 DeSmuME Team

	This file is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	This file is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with the this software.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <memory>
#include <vio2sf/types.h>

// the count of bytes copied from the firmware into memory
const int NDS_FW_USER_SETTINGS_MEM_BYTE_COUNT = 0x70;

#define FW_CONFIG_FILE_EXT "dfc"

class CFIRMWARE
{
private:
	std::unique_ptr<uint8_t[]> tmp_data9;
	std::unique_ptr<uint8_t[]> tmp_data7;
	uint32_t size9, size7;

	uint32_t keyBuf[0x412];
	uint32_t keyCode[3];

	bool getKeyBuf();
	void crypt64BitUp(uint32_t *ptr);
	void crypt64BitDown(uint32_t *ptr);
	void applyKeycode(uint32_t modulo);
	bool initKeycode(uint32_t idCode, int level, uint32_t modulo);
	uint16_t getBootCodeCRC16();
	uint32_t decrypt(const uint8_t *in, std::unique_ptr<uint8_t[]> &out);
	uint32_t decompress(const uint8_t *in, std::unique_ptr<uint8_t[]> &out);
public:
	CFIRMWARE(): size9(0), size7(0), ARM9bootAddr(0), ARM7bootAddr(0), patched(0) { }

	bool load();

	struct HEADER
	{
		uint16_t part3_rom_gui9_addr;		// 000h
		uint16_t part4_rom_wifi7_addr;		// 002h
		uint16_t part34_gui_wifi_crc16;		// 004h
		uint16_t part12_boot_crc16;			// 006h
		uint8_t fw_identifier[4];			// 008h
		uint16_t part1_rom_boot9_addr;		// 00Ch
		uint16_t part1_ram_boot9_addr;		// 00Eh
		uint16_t part2_rom_boot7_addr;		// 010h
		uint16_t part2_ram_boot7_addr;		// 012h
		uint16_t shift_amounts;				// 014h
		uint16_t part5_data_gfx_addr;		// 016h

		uint8_t fw_timestamp[5];			// 018h
		uint8_t console_type;				// 01Dh
		uint16_t unused1;					// 01Eh
		uint16_t user_settings_offset;		// 020h
		uint16_t unknown1;					// 022h
		uint16_t unknown2;					// 024h
		uint16_t part5_crc16;				// 026h
		uint16_t unused2;					// 028h	- FFh filled
	} header;

	uint32_t ARM9bootAddr;
	uint32_t ARM7bootAddr;
	bool patched;
};

int copy_firmware_user_data(uint8_t *dest_buffer, const uint8_t *fw_data);
void NDS_FillDefaultFirmwareConfigData(struct NDS_fw_config_data *fw_config);
