/*
	Copyright (C) 2006 thoduv
	Copyright (C) 2006-2007 Theo Berkau
	Copyright (C) 2008-2013 DeSmuME team

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

#include <cstdlib>
#include <cstring>
#include "types.h"
#include "vio2sf.h"

static const uint8_t FW_CMD_READ = 0x03;
static const uint8_t FW_CMD_WRITEDISABLE = 0x04;
static const uint8_t FW_CMD_READSTATUS = 0x05;
static const uint8_t FW_CMD_WRITEENABLE = 0x06;
static const uint8_t FW_CMD_PAGEWRITE = 0x0A;
static const uint8_t FW_CMD_READ_ID = 0x9F;

// since r2203 this was 0x00.
// but baby pals proves finally that it should be 0xFF:
// the game reads its initial sound volumes from uninitialized data, and if it is 0, the game will be silent
// if it is 0xFF then the game starts with its sound and music at max, as presumably it is supposed to.
// so in r3303 I finally changed it (no$ appears definitely to initialized to 0xFF)
static const uint8_t kUninitializedSaveDataValue = 0xFF;

static const char *kDesmumeSaveCookie = "|-DESMUME SAVE-|";

//the lookup table from user save types to save parameters
const SAVE_TYPE save_types[] =
{
	{"Autodetect", MC_TYPE_AUTODETECT, 1},
	{"EEPROM 4kbit", MC_TYPE_EEPROM1, MC_SIZE_4KBITS},
	{"EEPROM 64kbit", MC_TYPE_EEPROM2, MC_SIZE_64KBITS},
	{"EEPROM 512kbit", MC_TYPE_EEPROM2, MC_SIZE_512KBITS},
	{"FRAM 256kbit", MC_TYPE_FRAM, MC_SIZE_256KBITS},
	{"FLASH 2Mbit", MC_TYPE_FLASH, MC_SIZE_2MBITS},
	{"FLASH 4Mbit", MC_TYPE_FLASH, MC_SIZE_4MBITS},
	{"FLASH 8Mbit", MC_TYPE_FLASH, MC_SIZE_8MBITS},
	{"FLASH 16Mbit", MC_TYPE_FLASH, MC_SIZE_16MBITS},
	{"FLASH 32Mbit", MC_TYPE_FLASH, MC_SIZE_32MBITS},
	{"FLASH 64Mbit", MC_TYPE_FLASH, MC_SIZE_64MBITS},
	{"FLASH 128Mbit", MC_TYPE_FLASH, MC_SIZE_128MBITS},
	{"FLASH 256Mbit", MC_TYPE_FLASH, MC_SIZE_256MBITS},
	{"FLASH 512Mbit", MC_TYPE_FLASH, MC_SIZE_512MBITS}
};
void mc_init(memory_chip_t *mc, int type)
{
	mc->com = 0;
	mc->addr = 0;
	mc->addr_shift = 0;
	mc->data.clear();
	mc->size = 0;
	mc->write_enable = false;
	mc->writeable_buffer = false;
	mc->type = type;
	mc->autodetectsize = 0;

	switch (mc->type)
	{
		case MC_TYPE_EEPROM1:
			mc->addr_size = 1;
			break;
		case MC_TYPE_EEPROM2:
		case MC_TYPE_FRAM:
			mc->addr_size = 2;
			break;
		case MC_TYPE_FLASH:
			mc->addr_size = 3;
	}
}

uint8_t *mc_alloc(memory_chip_t *mc, uint32_t size)
{
	mc->data.clear();
	mc->data.resize(size, 0);
	mc->size = size;
	mc->writeable_buffer = true;

	return nullptr;
}

void mc_free(memory_chip_t *mc)
{
	mc->data.clear();
	mc_init(mc, 0);
}

void fw_reset_com(memory_chip_t *mc)
{
	if (mc->com == FW_CMD_PAGEWRITE)
	{
#if 0
		if (mc->fp)
		{
			fseek(mc->fp, 0, SEEK_SET);
			fwrite(&mc->data[0], mc->size, 1, mc->fp);
		}

		if (mc->isFirmware&&CommonSettings.UseExtFirmware)
		{
			// copy User Settings 1 to User Settings 0 area
			memcpy(&mc->data[0x3FE00], &mc->data[0x3FF00], 0x100);

			fprintf(stderr, "Firmware: save config");
			FILE *fp = fopen(mc->userfile, "wb");
			if (fp)
			{
				if (fwrite(&mc->data[0x3FF00], 1, 0x100, fp) == 0x100) // User Settings
				{
					if (fwrite(&mc->data[0x0002A], 1, 0x1D6, fp) == 0x1D6) // WiFi Settings
					{
						if (fwrite(&mc->data[0x3FA00], 1, 0x300, fp) == 0x300)  // WiFi AP Settings
							fprintf(stderr, " - done\n");
						else
							fprintf(stderr, " - failed\n");
					}
				}
				fclose(fp);
			}
			else
				fprintf(stderr, " - failed\n");
		}
#endif

		mc->write_enable = false;
	}

	mc->com = 0;
}

uint8_t fw_transfer(memory_chip_t *mc, uint8_t data)
{
	if (mc->com == FW_CMD_READ || mc->com == FW_CMD_PAGEWRITE) /* check if we are in a command that needs 3 bytes address */
	{
		if (mc->addr_shift > 0) /* if we got a complete address */
		{
			--mc->addr_shift;
			mc->addr |= data << (mc->addr_shift * 8); /* argument is a byte of address */
		}
		else /* if we have received 3 bytes of address, proceed command */
		{
			switch (mc->com)
			{
				case FW_CMD_READ:
					if (mc->addr < mc->size)  /* check if we can read */
					{
						data = mc->data[mc->addr]; /* return byte */
						++mc->addr; /* then increment address */
					}
					break;
				case FW_CMD_PAGEWRITE:
					if (mc->addr < mc->size)
					{
						mc->data[mc->addr] = data; /* write byte */
						++mc->addr;
					}
			}
		}
	}
	else if (mc->com == FW_CMD_READ_ID)
	{
		switch (mc->addr)
		{
			// here is an ID string measured from an old ds fat: 62 16 00 (0x62=sanyo)
			// but we chose to use an ST from martin's ds fat string so programs might have a clue as to the firmware size:
			// 20 40 12
			case 0:
				data = 0x20;
				mc->addr = 1;
				break;
			case 1:
				data = 0x40; // according to gbatek this is the device ID for the flash on someone's ds fat
				mc->addr = 2;
				break;
			case 2:
				data = 0x12;
				mc->addr = 0;
		}
	}
	else if (mc->com == FW_CMD_READSTATUS)
		return mc->write_enable ? 0x02 : 0x00;
	else // finally, check if it's a new command
	{
		switch (data)
		{
			case 0:
				break; // nothing

			case FW_CMD_READ_ID:
				mc->addr = 0;
				mc->com = FW_CMD_READ_ID;
				break;

			case FW_CMD_READ: //read command
				mc->addr = 0;
				mc->addr_shift = 3;
				mc->com = FW_CMD_READ;
				break;

			case FW_CMD_WRITEENABLE: //enable writing
				if (mc->writeable_buffer)
					mc->write_enable = true;
				break;

			case FW_CMD_WRITEDISABLE: //disable writing
				mc->write_enable = false;
				break;

			case FW_CMD_PAGEWRITE: //write command
				if (mc->write_enable)
				{
					mc->addr = 0;
					mc->addr_shift = 3;
					mc->com = FW_CMD_PAGEWRITE;
				}
				else
					data = 0;
				break;

			case FW_CMD_READSTATUS: //status register command
				mc->com = FW_CMD_READSTATUS;
				break;

			default:
				//fprintf(stderr, "Unhandled FW command: %02X\n", data);
				break;
		}
	}

	return data;
}

BackupDevice::BackupDevice()
{
	this->reset();
}

// due to unfortunate shortcomings in the emulator architecture,
// at reset-time, we won't have a filename to the .dsv file.
// so the only difference between load_rom (init) and reset is that
// one of them saves the filename
void BackupDevice::load_rom(const std::string &fn)
{
	//this->filename = fn;
	this->reset();
}

void BackupDevice::reset_hardware()
{
	this->write_enable = false;
	this->com = 0;
	this->addr = this->addr_counter = 0;
	this->motionInitState = MOTION_INIT_STATE_IDLE;
	this->motionFlag = MOTION_FLAG_NONE;
	this->state = DETECTING;
	this->flushPending = false;
	this->lazyFlushPending = false;
}

void BackupDevice::reset()
{
	memset(&this->info, 0, sizeof(this->info));
	this->reset_hardware();
	this->resize(0);
	this->data_autodetect.resize(0);
	this->addr_size = 0;
	this->loadfile();

#if 0
	// if the user has requested a manual choice for backup type, and we havent imported a raw save file, then apply it now
	if (this->state == DETECTING && CommonSettings.manualBackupType != MC_TYPE_AUTODETECT)
	{
		this->state = RUNNING;
		int savetype = save_types[CommonSettings.manualBackupType].media_type;
		int savesize = save_types[CommonSettings.manualBackupType].size;
		this->ensure(savesize); // expand properly if necessary
		this->resize(savesize); // truncate if necessary
		this->addr_size = this->addr_size_for_old_save_type(savetype);
	}
#endif
}

// guarantees that the data buffer has room enough for the specified number of bytes
void BackupDevice::ensure(uint32_t Addr)
{
	uint32_t size = this->data.size();
	if (size < Addr)
		this->resize(Addr);
}

void BackupDevice::resize(uint32_t size)
{
	size_t old_size = this->data.size();
	this->data.resize(size);
	for (uint32_t i = old_size; i < size; ++i)
		this->data[i] = kUninitializedSaveDataValue;
}

uint32_t BackupDevice::addr_size_for_old_save_size(int bupmem_size)
{
	switch (bupmem_size)
	{
		case MC_SIZE_4KBITS:
			return 1;
		case MC_SIZE_64KBITS:
		case MC_SIZE_256KBITS:
		case MC_SIZE_512KBITS:
			return 2;
		case MC_SIZE_1MBITS:
		case MC_SIZE_2MBITS:
		case MC_SIZE_4MBITS:
		case MC_SIZE_8MBITS:
		case MC_SIZE_16MBITS:
		case MC_SIZE_64MBITS:
			return 3;
		default:
			return 0xFFFFFFFF;
	}
}

uint32_t BackupDevice::addr_size_for_old_save_type(int bupmem_type)
{
	switch (bupmem_type)
	{
		case MC_TYPE_EEPROM1:
			return 1;
		case MC_TYPE_EEPROM2:
		case MC_TYPE_FRAM:
			return 2;
		case MC_TYPE_FLASH:
			return 3;
		default:
			return 0xFFFFFFFF;
	}
}

void BackupDevice::load_old_state(uint32_t addrSize, uint8_t *Data, uint32_t datasize)
{
	this->state = RUNNING;
	this->addr_size = addrSize;
	this->resize(datasize);
	memcpy(&this->data[0], Data, datasize);
}

// ======================================================================= no$GBA
// =======================================================================
// =======================================================================

static int no_gba_unpackSAV(const uint8_t *in_buf, uint32_t fsize, uint8_t *out_buf, uint32_t &size)
{
	const char no_GBA_HEADER_ID[] = "NocashGbaBackupMediaSavDataFile";
	const char no_GBA_HEADER_SRAM_ID[] = "SRAM";
	const uint8_t *src = in_buf;
	uint8_t *dst = out_buf;
	uint32_t src_pos = 0;
	uint32_t dst_pos = 0;
	uint32_t size_unpacked = 0;
	uint32_t compressMethod = 0;

	if (fsize < 0x50)
		return 1;

	for (int i = 0; i < 0x1F; ++i)
		if (src[i] != no_GBA_HEADER_ID[i])
			return 2;
	if (src[0x1F] != 0x1A)
		return 2;
	for (int i = 0; i < 0x4; ++i)
		if (src[i + 0x40] != no_GBA_HEADER_SRAM_ID[i])
			return 2;

	compressMethod = *(reinterpret_cast<const uint32_t *>(src + 0x44));

	if (!compressMethod) // unpacked
	{
		size_unpacked = *(reinterpret_cast<const uint32_t *>(src + 0x48));
		src_pos = 0x4C;
		for (uint32_t i = 0; i < size_unpacked; ++i)
			dst[dst_pos++] = src[src_pos++];
		size = dst_pos;
		return 0;
	}

	if (compressMethod == 1) // packed (method 1)
	{
		size_unpacked = *(reinterpret_cast<const uint32_t *>(src + 0x4C));

		src_pos = 0x50;
		while (true)
		{
			uint8_t cc = src[src_pos++];

			if (!cc)
			{
				size = dst_pos;
				return 0;
			}

			if (cc == 0x80)
			{
				uint16_t tsize = *(reinterpret_cast<const uint16_t *>(src + src_pos + 1));
				for (int t = 0; t < tsize; ++t)
					dst[dst_pos++] = src[src_pos];
				src_pos += 3;
				continue;
			}

			if (cc > 0x80) // repeat
			{
				cc -= 0x80;
				for (int t = 0; t < cc; ++t)
					dst[dst_pos++] = src[src_pos];
				++src_pos;
				continue;
			}
			// copy
			for (int t = 0; t < cc; ++t)
				dst[dst_pos++] = src[src_pos++];
		}
	}
	return 200;
}

static uint32_t no_gba_savTrim(uint8_t *buf, uint32_t size)
{
	uint32_t rows = size / 16;
	uint32_t pos = (size - 16);
	uint8_t *src = buf;

	for (unsigned i = 0; i < rows; ++i, pos -= 16)
	{
		if (src[pos] == 0xFF)
		{
			for (int t = 0; t < 16; ++t)
				if (src[pos + t] != 0xFF)
					return pos + 16;
		}
		else
			return pos + 16;
	}
	return size;
}

static uint32_t no_gba_fillLeft(uint32_t size)
{
	for (uint32_t i = 1; i < ARRAY_SIZE(save_types); ++i)
		if (size <= static_cast<uint32_t>(save_types[i].size))
			return size + (save_types[i].size - size);
	return size;
}

bool BackupDevice::load_no_gba(const char *fname)
{
#if 0
	FILE *fsrc = fopen(fname, "rb");

	if (fsrc)
	{
		fseek(fsrc, 0, SEEK_END);
		uint32_t fsize = ftell(fsrc);
		fseek(fsrc, 0, SEEK_SET);
		//fprintf(stderr, "Open %s file (size %i bytes)\n", fname, fsize);

		auto in_buf = std::unique_ptr<uint8_t[]>(new uint8_t[fsize]);

		if (fread(&in_buf[0], 1, fsize, fsrc) == fsize)
		{
			auto out_buf = std::unique_ptr<uint8_t[]>(new uint8_t[8 * 1024 * 1024 / 8]);

			memset(&out_buf[0], 0xFF, 8 * 1024 * 1024 / 8);
			uint32_t size = 0;
			if (!no_gba_unpackSAV(&in_buf[0], fsize, &out_buf[0], size))
			{
				//fprintf(stderr, "New size %i byte(s)\n", size);
				size = no_gba_savTrim(&out_buf[0], size);
				//fprintf(stderr, "--- new size after trim %i byte(s)\n", size);
				size = no_gba_fillLeft(size);
				//fprintf(stderr, "--- new size after fill %i byte(s)\n", size);
				raw_applyUserSettings(size);
				this->data.resize(size);
				for (uint32_t tt = 0; tt < size; ++tt)
					this->data[tt] = out_buf[tt];

				//dump back out as a dsv, just to keep things sane
				fprintf(stderr, "---- Loaded no$GBA save\n");

				fclose(fsrc);
				return true;
			}
		}
		fclose(fsrc);
	}
#endif

	return false;
}

// ======================================================================= end
// =======================================================================
// ======================================================================= no$GBA

void BackupDevice::loadfile()
{
#if 0
	if (this->filename.empty())
		return; // No sense crashing if no filename supplied

	auto inf = std::unique_ptr<EMUFILE_FILE>(new EMUFILE_FILE(filename.c_str(), "rb"));
	if (inf->fail())
	{
		// no dsv found; we need to try auto-importing a file with .sav extension
		fprintf(stderr, "DeSmuME .dsv save file not found. Trying to load an old raw .sav file.\n");

		// change extension to sav
		char tmp[MAX_PATH];
		strcpy(tmp, this->filename.c_str());
		tmp[strlen(tmp) - 3] = 0;
		strcat(tmp, "sav");

		inf.reset(new EMUFILE_FILE(tmp, "rb"));
		if (inf->fail())
		{
			fprintf(stderr, "Missing save file %s\n", this->filename.c_str());
			return;
		}

		if (!this->load_no_gba(tmp))
			this->load_raw(tmp);
	}
	else
	{
		// scan for desmume save footer
		int32_t cookieLen = strlen(kDesmumeSaveCookie);
		auto sigbuf = std::unique_ptr<char[]>(new char[cookieLen]);
		inf->fseek(-cookieLen, SEEK_END);
		inf->fread(&sigbuf[0], cookieLen);
		int cmp = memcmp(&sigbuf[0], kDesmumeSaveCookie,cookieLen);
		if (cmp)
		{
			// maybe it is a misnamed raw save file. try loading it that way
			fprintf(stderr, "Not a DeSmuME .dsv save file. Trying to load as raw.\n");
			if (!this->load_no_gba(this->filename.c_str()))
				this->load_raw(this->filename.c_str());
			return;
		}
		// desmume format
		inf->fseek(-cookieLen, SEEK_END);
		inf->fseek(-4, SEEK_CUR);
		uint32_t version = 0xFFFFFFFF;
		read32le(&version, inf.get());
		if (version)
		{
			fprintf(stderr, "Unknown save file format\n");
			return;
		}
		inf->fseek(-24, SEEK_CUR);
		read32le(&this->info.size, inf.get());
		read32le(&this->info.padSize, inf.get());
		read32le(&this->info.type, inf.get());
		read32le(&this->info.addr_size, inf.get());
		read32le(&this->info.mem_size, inf.get());

		// establish the save data
		this->resize(this->info.size);
		inf->fseek(0, SEEK_SET);
		if (this->info.size > 0)
			inf->fread(&this->data[0], this->info.size); // read all the raw data we have
		this->state = RUNNING;
		this->addr_size = this->info.addr_size;
		// none of the other fields are used right now
	}
#endif
}

void BackupDevice::raw_applyUserSettings(uint32_t &size, bool manual)
{
#if 0
	// respect the user's choice of backup memory type
	if (CommonSettings.manualBackupType == MC_TYPE_AUTODETECT && !manual)
	{
		this->addr_size = this->addr_size_for_old_save_size(size);
		this->resize(size);
	}
	else
	{
		uint32_t type = CommonSettings.manualBackupType;
		int savetype = save_types[type].media_type;
		int savesize = save_types[type].size;
		this->addr_size = this->addr_size_for_old_save_type(savetype);
		if (static_cast<uint32_t>(savesize) < size)
			size = savesize;
		this->resize(savesize);
	}
#endif

	this->state = RUNNING;
}

bool BackupDevice::load_raw(const char *fn, uint32_t force_size)
{
#if 0
	FILE *inf = fopen(fn,"rb");

	if (!inf)
		return false;

	fseek(inf, 0, SEEK_END);
	uint32_t size = ftell(inf);
	uint32_t left = 0;

	if (force_size > 0)
	{
		if (size > force_size)
			size = force_size;
		else if (size < force_size)
		{
			left = force_size - size;
			size = force_size;
		}
	}

	fseek(inf, 0, SEEK_SET);

	this->raw_applyUserSettings(size, force_size > 0);

	fread(&this->data[0], 1, size - left, inf);
	fclose(inf);

	return true;
#endif

	return false;
}
