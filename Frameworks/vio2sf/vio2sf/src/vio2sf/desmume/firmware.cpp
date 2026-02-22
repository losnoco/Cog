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

#include "firmware.h"
#include "NDSSystem.h"

static inline uint32_t DWNUM(uint32_t i) { return i >> 2; }

bool CFIRMWARE::getKeyBuf()
{
	return false;
#if 0
	FILE *file = fopen(CommonSettings.ARM7BIOS, "rb");
	if (!file)
		return false;

	fseek(file, 0x30, SEEK_SET);
	size_t res = fread(this->keyBuf, 4, 0x412, file);
	fclose(file);
	return res == 0x412;
#endif
}

void CFIRMWARE::crypt64BitUp(uint32_t *ptr)
{
	uint32_t Y = ptr[0];
	uint32_t X = ptr[1];

	for (uint32_t i = 0x00; i <= 0x0F; ++i)
	{
		uint32_t Z = this->keyBuf[i] ^ X;
		X = this->keyBuf[DWNUM(0x048 + (((Z >> 24) & 0xFF) << 2))];
		X = this->keyBuf[DWNUM(0x448 + (((Z >> 16) & 0xFF) << 2))] + X;
		X = this->keyBuf[DWNUM(0x848 + (((Z >> 8) & 0xFF) << 2))] ^ X;
		X = this->keyBuf[DWNUM(0xC48 + ((Z & 0xFF) << 2))] + X;
		X = Y ^ X;
		Y = Z;
	}

	ptr[0] = X ^ this->keyBuf[DWNUM(0x40)];
	ptr[1] = Y ^ this->keyBuf[DWNUM(0x44)];
}

void CFIRMWARE::crypt64BitDown(uint32_t *ptr)
{
	uint32_t Y = ptr[0];
	uint32_t X = ptr[1];

	for (uint32_t i = 0x11; i >= 0x02; --i)
	{
		uint32_t Z = this->keyBuf[i] ^ X;
		X = this->keyBuf[DWNUM(0x048 + (((Z >> 24) & 0xFF) << 2))];
		X = this->keyBuf[DWNUM(0x448 + (((Z >> 16) & 0xFF) << 2))] + X;
		X = this->keyBuf[DWNUM(0x848 + (((Z >> 8) & 0xFF) << 2))] ^ X;
		X = this->keyBuf[DWNUM(0xC48 + ((Z & 0xFF) << 2))] + X;
		X = Y ^ X;
		Y = Z;
	}

	ptr[0] = X ^ this->keyBuf[DWNUM(0x04)];
	ptr[1] = Y ^ this->keyBuf[DWNUM(0x00)];
}

static inline uint32_t bswap32(uint32_t val) { return ((val & 0x000000FF) << 24) | ((val & 0x0000FF00) << 8) | ((val & 0x00FF0000) >> 8) | ((val & 0xFF000000) >> 24); }
void CFIRMWARE::applyKeycode(uint32_t modulo)
{
	this->crypt64BitUp(&this->keyCode[1]);
	this->crypt64BitUp(&this->keyCode[0]);

	uint32_t scratch[] = { 0x00000000, 0x00000000 };

	for (uint32_t i = 0; i <= 0x44; i += 4)
		this->keyBuf[DWNUM(i)] = this->keyBuf[DWNUM(i)] ^ bswap32(this->keyCode[DWNUM(i % modulo)]);

	for (uint32_t i = 0; i <= 0x1040; i += 8)
	{
		this->crypt64BitUp(scratch);
		this->keyBuf[DWNUM(i)] = scratch[1];
		this->keyBuf[DWNUM(i + 4)] = scratch[0];
	}
}

bool CFIRMWARE::initKeycode(uint32_t idCode, int level, uint32_t modulo)
{
	if (!this->getKeyBuf())
		return false;

	this->keyCode[0] = idCode;
	this->keyCode[1] = idCode >> 1;
	this->keyCode[2] = idCode << 1;

	if (level >= 1)
		this->applyKeycode(modulo);
	if (level >= 2)
		this->applyKeycode(modulo);

	this->keyCode[1] <<= 1;
	this->keyCode[2] >>= 1;

	if (level >= 3)
		this->applyKeycode(modulo);

	return true;
}

uint16_t CFIRMWARE::getBootCodeCRC16()
{
	uint32_t crc = 0xFFFF;
	const uint16_t val[] = { 0xC0C1, 0xC181, 0xC301, 0xC601, 0xCC01, 0xD801, 0xF001, 0xA001 };

	unsigned i, j;
	for (i = 0; i < this->size9; ++i)
	{
		crc ^= this->tmp_data9[i];

		for (j = 0; j < 8; ++j)
		{
			if (crc & 0x0001)
				crc = (crc >> 1) ^ (val[j] << (7 - j));
			else
				crc >>= 1;
		}
	}

	for (i = 0; i < this->size7; ++i)
	{
		crc ^= this->tmp_data7[i];

		for (j = 0; j < 8; ++j)
		{
			if (crc & 0x0001)
				crc = (crc >> 1) ^ (val[j] << (7 - j));
			else
				crc >>= 1;
		}
	}

	return crc & 0xFFFF;
}

uint32_t CFIRMWARE::decrypt(const uint8_t *in, std::unique_ptr<uint8_t[]> &out)
{
	uint32_t curBlock[2] = { 0 };

	uint32_t xIn = 4, xOut = 0;

	memcpy(curBlock, in, 8);
	this->crypt64BitDown(curBlock);
	uint32_t blockSize = curBlock[0] >> 8;

	if (!blockSize)
		return 0;

	out.reset(new uint8_t[blockSize]);
	if (!out)
		return 0;
	memset(&out[0], 0xFF, blockSize);

	uint32_t xLen = blockSize;
	while (xLen > 0)
	{
		uint8_t d = T1ReadByte(reinterpret_cast<uint8_t *>(curBlock), xIn % 8);
		++xIn;
		if (!(xIn % 8))
		{
			memcpy(curBlock, in + xIn, 8);
			this->crypt64BitDown(curBlock);
		}

		for (uint32_t i = 0; i < 8; ++i)
		{
			if (d & 0x80)
			{
				uint16_t data = T1ReadByte(reinterpret_cast<uint8_t *>(curBlock), xIn % 8) << 8;
				++xIn;
				if (!(xIn % 8))
				{
					memcpy(curBlock, in + xIn, 8);
					this->crypt64BitDown(curBlock);
				}
				data |= T1ReadByte(reinterpret_cast<uint8_t *>(curBlock), xIn % 8);
				++xIn;
				if (!(xIn % 8))
				{
					memcpy(curBlock, in + xIn, 8);
					this->crypt64BitDown(curBlock);
				}

				uint32_t len = (data >> 12) + 3;
				uint32_t offset = data & 0xFFF;
				uint32_t windowOffset = xOut - offset - 1;

				for (uint32_t j = 0; j < len; ++j)
				{
					T1WriteByte(&out[0], xOut, T1ReadByte(&out[0], windowOffset));
					++xOut;
					++windowOffset;

					--xLen;
					if (!xLen)
						return blockSize;
				}
			}
			else
			{
				T1WriteByte(&out[0], xOut, T1ReadByte(reinterpret_cast<uint8_t *>(curBlock), xIn % 8));
				++xOut;
				++xIn;
				if (!(xIn % 8))
				{
					memcpy(curBlock, in + xIn, 8);
					this->crypt64BitDown(curBlock);
				}

				--xLen;
				if (!xLen)
					return blockSize;
			}

			d = (d << 1) & 0xFF;
		}
	}

	return blockSize;
}

uint32_t CFIRMWARE::decompress(const uint8_t *in, std::unique_ptr<uint8_t[]> &out)
{
	uint32_t curBlock[2] = { 0 };

	uint32_t xIn = 4, xOut = 0;

	memcpy(curBlock, in, 8);
	uint32_t blockSize = curBlock[0] >> 8;

	if (!blockSize)
		return 0;

	out.reset(new uint8_t[blockSize]);
	if (!out)
		return 0;
	memset(&out[0], 0xFF, blockSize);

	uint32_t xLen = blockSize;
	while (xLen > 0)
	{
		uint8_t d = T1ReadByte(reinterpret_cast<uint8_t *>(curBlock), xIn % 8);
		++xIn;
		if (!(xIn % 8))
			memcpy(curBlock, in + xIn, 8);

		for (uint32_t i = 0; i < 8; ++i)
		{
			if (d & 0x80)
			{
				uint16_t data = T1ReadByte(reinterpret_cast<uint8_t *>(curBlock), xIn % 8) << 8;
				++xIn;
				if (!(xIn % 8))
					memcpy(curBlock, in + xIn, 8);
				data |= T1ReadByte(reinterpret_cast<uint8_t *>(curBlock), xIn % 8);
				++xIn;
				if (!(xIn % 8))
					memcpy(curBlock, in + xIn, 8);

				uint32_t len = (data >> 12) + 3;
				uint32_t offset = data & 0xFFF;
				uint32_t windowOffset = xOut - offset - 1;

				for (uint32_t j = 0; j < len; ++j)
				{
					T1WriteByte(&out[0], xOut, T1ReadByte(&out[0], windowOffset));
					++xOut;
					++windowOffset;

					--xLen;
					if (!xLen)
						return blockSize;
				}
			}
			else
			{
				T1WriteByte(&out[0], xOut, T1ReadByte(reinterpret_cast<uint8_t *>(curBlock), xIn % 8));
				++xOut;
				++xIn;
				if (!(xIn % 8))
					memcpy(curBlock, in + xIn, 8);

				--xLen;
				if (!xLen)
					return blockSize;
			}

			d = (d << 1) & 0xFF;
		}
	}

	return blockSize;
}

// ================================================================================
bool CFIRMWARE::load()
{
	return false;
#if 0
	if (!CommonSettings.UseExtFirmware)
		return false;
	if (!strlen(CommonSettings.Firmware))
		return false;

	FILE *fp = fopen(CommonSettings.Firmware, "rb");
	if (!fp)
		return false;
	fseek(fp, 0, SEEK_END);
	uint32_t size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	if (size != 262144 && size != 524288)
	{
		fclose(fp);
		return false;
	}

#if 1
	if (size == 524288)
	{
		fclose(fp);
		return false;
	}
#endif

	auto data = std::unique_ptr<uint8_t[]>(new uint8_t[size]);
	if (!data)
	{
		fclose(fp);
		return false;
	}

	if (fread(&data[0], 1, size, fp) != size)
	{
		fclose(fp);
		return false;
	}

	memcpy(&header, &data[0], sizeof(header));
	if (header.fw_identifier[0] != 'M' || header.fw_identifier[1] != 'A' || header.fw_identifier[2] != 'C')
	{
		fclose(fp);
		return false;
	}

	uint16_t shift1 = header.shift_amounts & 0x07;
	uint16_t shift2 = (header.shift_amounts >> 3) & 0x07;
	uint16_t shift3 = (header.shift_amounts >> 6) & 0x07;
	uint16_t shift4 = (header.shift_amounts >> 9) & 0x07;

	// todo - add support for 512Kb
	uint32_t part1addr = header.part1_rom_boot9_addr << (2 + shift1);
	uint32_t part1ram = 0x02800000 - (header.part1_ram_boot9_addr << (2 + shift2));
	uint32_t part2addr = header.part2_rom_boot7_addr << (2 + shift3);
	uint32_t part2ram = 0x03810000 - (header.part2_ram_boot7_addr << (2 + shift4));

	this->ARM9bootAddr = part1ram;
	this->ARM7bootAddr = part2ram;

	if (!this->initKeycode(T1ReadLong(&data[0], 0x08), 1, 0xC))
	{
		fclose(fp);
		return false;
	}

#if 0
	this->crypt64BitDown(reinterpret_cast<uint32_t *>(&data[0x18]));
#else
	// fix touch coords
	data[0x18] = 0x00;
	data[0x19] = 0x00;
	data[0x1A] = 0x00;
	data[0x1B] = 0x00;

	data[0x1C] = 0x00;
	data[0x1D] = 0xFF;
	data[0x1E] = 0x00;
	data[0x1F] = 0x00;
#endif

	if (!this->initKeycode(T1ReadLong(&data[0], 0x08), 2, 0xC))
	{
		fclose(fp);
		return false;
	}

	this->size9 = this->decrypt(&data[part1addr], this->tmp_data9);
	if (!this->tmp_data9)
	{
		fclose(fp);
		return false;
	}

	this->size7 = this->decrypt(&data[part2addr], this->tmp_data7);
	if (!this->tmp_data7)
	{
		this->tmp_data9.reset();
		fclose(fp);
		return false;
	}

	uint16_t crc16_mine = this->getBootCodeCRC16();

	if (crc16_mine != header.part12_boot_crc16)
	{
		this->tmp_data9.reset();
		this->tmp_data7.reset();
		fclose(fp);
		return false;
	}

	// Copy firmware boot codes to their respective locations
	uint32_t src = 0;
	for (uint32_t i = 0; i < (this->size9 >> 2); ++i)
	{
		_MMU_write32<ARMCPU_ARM9>(part1ram, T1ReadLong(&this->tmp_data9[0], src));
		src += 4;
		part1ram += 4;
	}

	src = 0;
	for (uint32_t i = 0; i < (this->size7 >> 2); ++i)
	{
		_MMU_write32<ARMCPU_ARM7>(part2ram, T1ReadLong(&this->tmp_data7[0], src));
		src += 4;
		part2ram += 4;
	}
	this->tmp_data7.reset();
	this->tmp_data9.reset();

	this->patched = false;
	if (data[0x17C] != 0xFF)
		this->patched = true;

	if (this->patched)
	{
		uint32_t patch_offset = 0x3FC80;
		if (data[0x17C] > 1)
			patch_offset = 0x3F680;

		memcpy(&header, &data[patch_offset], sizeof(header));

		shift1 = header.shift_amounts & 0x07;
		shift2 = (header.shift_amounts >> 3) & 0x07;
		shift3 = (header.shift_amounts >> 6) & 0x07;
		shift4 = (header.shift_amounts >> 9) & 0x07;

		// todo - add support for 512Kb
		part1addr = header.part1_rom_boot9_addr << (2 + shift1);
		part1ram = 0x02800000 - (header.part1_ram_boot9_addr << (2 + shift2));
		part2addr = header.part2_rom_boot7_addr << (2 + shift3);
		part2ram = 0x03810000 - (header.part2_ram_boot7_addr << (2 + shift4));

		this->ARM9bootAddr = part1ram;
		this->ARM7bootAddr = part2ram;

		this->size9 = this->decompress(&data[part1addr], this->tmp_data9);
		if (!this->tmp_data9)
		{
			fclose(fp);
			return false;
		}

		this->size7 = this->decompress(&data[part2addr], this->tmp_data7);
		if (!this->tmp_data7)
		{
			this->tmp_data9.reset();
			fclose(fp);
			return false;
		};
		// Copy firmware boot codes to their respective locations
		src = 0;
		for (uint32_t i = 0; i < (this->size9 >> 2); ++i)
		{
			_MMU_write32<ARMCPU_ARM9>(part1ram, T1ReadLong(&this->tmp_data9[0], src));
			src += 4;
			part1ram += 4;
		}

		src = 0;
		for (uint32_t i = 0; i < (this->size7 >> 2); ++i)
		{
			_MMU_write32<ARMCPU_ARM7>(part2ram, T1ReadLong(&this->tmp_data7[0], src));
			src += 4;
			part2ram += 4;
		}
		this->tmp_data7.reset();
		this->tmp_data9.reset();
	}

	// TODO: add 512Kb support
	memcpy(&MMU.fw.data[0], &data[0], 262144);
	MMU.fw.fp = nullptr;

	return true;
#endif
}

// =====================================================================================================
static uint32_t calc_CRC16(uint32_t start, const uint8_t *data, int count)
{
	uint32_t crc = start & 0xffff;
	const uint16_t val[] = { 0xC0C1, 0xC181, 0xC301, 0xC601, 0xCC01, 0xD801, 0xF001, 0xA001 };
	for (int i = 0; i < count; ++i)
	{
		crc ^= data[i];

		for (int j = 0; j < 8; ++j)
		{
			bool do_bit = false;

			if (crc & 0x1)
				do_bit = true;

			crc >>= 1;

			if (do_bit)
				crc ^= val[j] << (7 - j);
		}
	}
	return crc;
}

int copy_firmware_user_data(uint8_t *dest_buffer, const uint8_t *fw_data)
{
	/*
	 * Determine which of the two user settings in the firmware is the current
	 * and valid one and then copy this into the destination buffer.
	 *
	 * The current setting will have a greater count.
	 * Settings are only valid if its CRC16 is correct.
	 */
	int copy_good = 0;

	uint32_t user_settings_offset = fw_data[0x20];
	user_settings_offset |= fw_data[0x21] << 8;
	user_settings_offset <<= 3;

	if (user_settings_offset <= 0x3FE00)
	{
		int32_t copy_settings_offset = -1;

		uint32_t crc = calc_CRC16(0xffff, &fw_data[user_settings_offset], NDS_FW_USER_SETTINGS_MEM_BYTE_COUNT);
		uint32_t fw_crc = fw_data[user_settings_offset + 0x72];
		fw_crc |= fw_data[user_settings_offset + 0x73] << 8;
		bool user1_valid = crc == fw_crc;

		crc = calc_CRC16(0xffff, &fw_data[user_settings_offset + 0x100], NDS_FW_USER_SETTINGS_MEM_BYTE_COUNT);
		fw_crc = fw_data[user_settings_offset + 0x100 + 0x72];
		fw_crc |= fw_data[user_settings_offset + 0x100 + 0x73] << 8;
		bool user2_valid = crc == fw_crc;

		if (user1_valid)
		{
			if (user2_valid)
			{
				uint16_t count1, count2;

				count1 = fw_data[user_settings_offset + 0x70];
				count1 |= fw_data[user_settings_offset + 0x71] << 8;

				count2 = fw_data[user_settings_offset + 0x100 + 0x70];
				count2 |= fw_data[user_settings_offset + 0x100 + 0x71] << 8;

				if (count2 > count1)
					copy_settings_offset = user_settings_offset + 0x100;
				else
					copy_settings_offset = user_settings_offset;
			}
			else
				copy_settings_offset = user_settings_offset;
		}
		else if (user2_valid)
			/* copy the second user settings */
			copy_settings_offset = user_settings_offset + 0x100;

		if (copy_settings_offset > 0)
		{
			memcpy(dest_buffer, &fw_data[copy_settings_offset], NDS_FW_USER_SETTINGS_MEM_BYTE_COUNT);
			copy_good = 1;
		}
	}

	return copy_good;
}

void NDS_FillDefaultFirmwareConfigData(NDS_fw_config_data *fw_config)
{
	const char *default_nickname = "DeSmuME";
	const char *default_message = "DeSmuME makes you happy!";

	memset(fw_config, 0, sizeof(struct NDS_fw_config_data));
	fw_config->ds_type = NDS_CONSOLE_TYPE_FAT;

	fw_config->fav_colour = 7;

	fw_config->birth_day = 23;
	fw_config->birth_month = 6;

	int str_length = strlen(default_nickname);
	int i;
	for (i = 0; i < str_length; ++i)
		fw_config->nickname[i] = default_nickname[i];
	fw_config->nickname_len = str_length;

	str_length = strlen(default_message);
	for (i = 0; i < str_length; ++i)
		fw_config->message[i] = default_message[i];
	fw_config->message_len = str_length;

	/* default to English */
	fw_config->language = 1;
}
