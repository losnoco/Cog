/*
	Copyright (C) 2006 thoduv
	Copyright (C) 2006 Theo Berkau
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

#pragma once

#include <vector>
#include <string>
#include <cstdio>
#include <vio2sf/types.h>

#define MAX_SAVE_TYPES 13
#define MC_TYPE_AUTODETECT      0x0
#define MC_TYPE_EEPROM1         0x1
#define MC_TYPE_EEPROM2         0x2
#define MC_TYPE_FLASH           0x3
#define MC_TYPE_FRAM            0x4

#define MC_SIZE_4KBITS                  0x000200
#define MC_SIZE_64KBITS                 0x002000
#define MC_SIZE_256KBITS                0x008000
#define MC_SIZE_512KBITS                0x010000
#define MC_SIZE_1MBITS                  0x020000
#define MC_SIZE_2MBITS                  0x040000
#define MC_SIZE_4MBITS                  0x080000
#define MC_SIZE_8MBITS                  0x100000
#define MC_SIZE_16MBITS                 0x200000
#define MC_SIZE_32MBITS                 0x400000
#define MC_SIZE_64MBITS                 0x800000
#define MC_SIZE_128MBITS                0x1000000
#define MC_SIZE_256MBITS                0x2000000
#define MC_SIZE_512MBITS                0x4000000

struct memory_chip_t
{
	uint8_t com; // persistent command actually handled
	uint32_t addr; // current address for reading/writing
	uint8_t addr_shift; // shift for address (since addresses are transfered by 3 bytes units)
	uint8_t addr_size; // size of addr when writing/reading

	bool write_enable; //is write enabled ?

	std::vector<uint8_t> data; //memory data
	uint32_t size; // memory size
	bool writeable_buffer; // is "data" writeable ?
	int type; //type of Memory
	//char *filename;
	//FILE *fp;
	uint8_t autodetectbuf[32768];
	int autodetectsize;

	// needs only for firmware
	bool isFirmware;
	//char userfile[MAX_PATH];
};

// the new backup system by zeromus
class BackupDevice
{
public:
	BackupDevice();

	// signals the save system that we are in our regular mode, loading up a rom. initializes for that case.
	void load_rom(const std::string &filename);

	void reset();
	void reset_hardware();
	std::string getFilename() { return filename; }

	std::vector<uint8_t> data;

	// this info was saved before the last reset (used for savestate compatibility)
	struct SavedInfo
	{
		uint32_t addr_size;
	} savedInfo;

	// and these are used by old savestates
	void load_old_state(uint32_t addr_size, uint8_t *data, uint32_t datasize);
	static uint32_t addr_size_for_old_save_size(int bupmem_size);
	static uint32_t addr_size_for_old_save_type(int bupmem_type);

	void raw_applyUserSettings(uint32_t &size, bool manual = false);

	bool load_no_gba(const char *fname);
	bool load_raw(const char* filename, uint32_t force_size = 0);

	struct
	{
		uint32_t size, padSize, type, addr_size, mem_size;
	} info;
private:
	std::string filename;

	bool write_enable; // is write enabled?
	uint32_t com; // persistent command actually handled
	uint32_t addr_size, addr_counter;
	uint32_t addr;

	std::vector<uint8_t> data_autodetect;
	enum STATE
	{
		DETECTING = 0,
		RUNNING = 1
	} state;

	enum MOTION_INIT_STATE
	{
		MOTION_INIT_STATE_IDLE,
		MOTION_INIT_STATE_RECEIVED_4,
		MOTION_INIT_STATE_RECEIVED_4_B,
		MOTION_INIT_STATE_FE,
		MOTION_INIT_STATE_FD,
		MOTION_INIT_STATE_FB
	};
	enum MOTION_FLAG
	{
		MOTION_FLAG_NONE,
		MOTION_FLAG_ENABLED,
		MOTION_FLAG_SENSORMODE
	};
	uint8_t motionInitState, motionFlag;

	void loadfile();
	void ensure(uint32_t addr);

	bool flushPending, lazyFlushPending;

	void resize(uint32_t size);
};

#define NDS_FW_SIZE_V1 (256 * 1024) /* size of fw memory on nds v1 */
#define NDS_FW_SIZE_V2 (512 * 1024) /* size of fw memory on nds v2 */

void mc_init(memory_chip_t *mc, int type); /* reset and init values for memory struct */
uint8_t *mc_alloc(memory_chip_t *mc, uint32_t size); /* alloc mc memory */
void mc_free(memory_chip_t *mc); /* delete mc memory */
void fw_reset_com(memory_chip_t *mc); /* reset communication with mc */
uint8_t fw_transfer(memory_chip_t *mc, uint8_t data);

struct SAVE_TYPE
{
	const char *descr;
	int media_type;
	int size;
};
