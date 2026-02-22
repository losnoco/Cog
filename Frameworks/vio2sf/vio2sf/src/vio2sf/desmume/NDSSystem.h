/*
	Copyright (C) 2006 yopyop
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

#include <memory>
#include <string>
#include <cstring>
#include <stdlib.h>
#include <vio2sf/armcpu.h>
#include <vio2sf/MMU.h>
#include <vio2sf/SPU.h>
#include <vio2sf/mem.h>
#include <vio2sf/emufile.h>
#include <vio2sf/firmware.h>

struct vio2sf_state;

template<typename Type> struct buttonstruct
{
	union
	{
		struct
		{
			// changing the order of these fields would break stuff
			//fRLDUTSBAYXWEg
			Type G; // debug
			Type E; // right shoulder
			Type W; // left shoulder
			Type X;
			Type Y;
			Type A;
			Type B;
			Type S; // start
			Type T; // select
			Type U; // up
			Type D; // down
			Type L; // left
			Type R; // right
			Type F; // lid
		};
		Type array[14];
	};
};

struct NDS_header
{
	char gameTile[12];
	char gameCode[4];
	uint16_t makerCode;
	uint8_t unitCode;
	uint8_t deviceCode;
	uint8_t cardSize;
	uint8_t cardInfo[8];
	uint8_t flags;
	uint8_t romversion;

	uint32_t ARM9src;
	uint32_t ARM9exe;
	uint32_t ARM9cpy;
	uint32_t ARM9binSize;

	uint32_t ARM7src;
	uint32_t ARM7exe;
	uint32_t ARM7cpy;
	uint32_t ARM7binSize;

	uint32_t FNameTblOff;
	uint32_t FNameTblSize;

	uint32_t FATOff;
	uint32_t FATSize;

	uint32_t ARM9OverlayOff;
	uint32_t ARM9OverlaySize;
	uint32_t ARM7OverlayOff;
	uint32_t ARM7OverlaySize;

	uint32_t unknown2a;
	uint32_t unknown2b;

	uint32_t IconOff;
	uint16_t CRC16;
	uint16_t ROMtimeout;
	uint32_t ARM9unk;
	uint32_t ARM7unk;

	uint8_t unknown3c[8];
	uint32_t ROMSize;
	uint32_t HeaderSize;
	uint8_t unknown5[56];
	uint8_t logo[156];
	uint16_t logoCRC16;
	uint16_t headerCRC16;
	uint8_t reserved[160];
};

struct TSequenceItem
{
	vio2sf_state *st;

	uint64_t timestamp;
	uint32_t param;
	bool enabled;

	TSequenceItem(vio2sf_state *_st) : st(_st) { }
	virtual ~TSequenceItem() { }

	virtual bool isTriggered() const;

	virtual uint64_t next() const
	{
		return this->timestamp;
	}
};

template<int procnum, int num> struct TSequenceItem_Timer : public TSequenceItem
{
	TSequenceItem_Timer(vio2sf_state *_st) : TSequenceItem(_st) { }
	bool isTriggered() const;
	void schedule();
	uint64_t next() const;
	void exec();
};

template<int procnum, int chan> struct TSequenceItem_DMA : public TSequenceItem
{
	DmaController *controller;

	TSequenceItem_DMA(vio2sf_state *_st) : TSequenceItem(_st) { }

	bool isTriggered() const;
	bool isEnabled() const;
	uint64_t next() const;
	void exec();
};

struct TSequenceItem_divider : public TSequenceItem
{
	TSequenceItem_divider(vio2sf_state *_st) : TSequenceItem(_st) { }

	bool isTriggered() const;
	bool isEnabled();
	uint64_t next() const;
	void exec();
};

struct TSequenceItem_sqrtunit : public TSequenceItem
{
	TSequenceItem_sqrtunit(vio2sf_state *_st) : TSequenceItem(_st) { }

	bool isTriggered() const;
	bool isEnabled();
	uint64_t next() const;
	void exec();
};

struct Sequencer
{
	bool nds_vblankEnded;
	bool reschedule;
	vio2sf_state *st;
	TSequenceItem dispcnt;
	TSequenceItem wifi;
	TSequenceItem_divider divider;
	TSequenceItem_sqrtunit sqrtunit;
	TSequenceItem/*_GXFIFO*/ gxfifo;
	TSequenceItem_DMA<0, 0> dma_0_0; TSequenceItem_DMA<0, 1> dma_0_1;
	TSequenceItem_DMA<0, 2> dma_0_2; TSequenceItem_DMA<0, 3> dma_0_3;
	TSequenceItem_DMA<1, 0> dma_1_0; TSequenceItem_DMA<1, 1> dma_1_1;
	TSequenceItem_DMA<1, 2> dma_1_2; TSequenceItem_DMA<1, 3> dma_1_3;
	TSequenceItem_Timer<0, 0> timer_0_0; TSequenceItem_Timer<0, 1> timer_0_1;
	TSequenceItem_Timer<0, 2> timer_0_2; TSequenceItem_Timer<0, 3> timer_0_3;
	TSequenceItem_Timer<1, 0> timer_1_0; TSequenceItem_Timer<1, 1> timer_1_1;
	TSequenceItem_Timer<1, 2> timer_1_2; TSequenceItem_Timer<1, 3> timer_1_3;

	Sequencer(vio2sf_state *_st = nullptr);
	void init();

	void execHardware();
	uint64_t findNext();
};

void NDS_Reschedule(vio2sf_state *st);
void NDS_RescheduleDMA(vio2sf_state *st);
void NDS_RescheduleTimers(vio2sf_state *st);

enum NDS_CONSOLE_TYPE
{
	NDS_CONSOLE_TYPE_FAT,
	NDS_CONSOLE_TYPE_LITE,
	NDS_CONSOLE_TYPE_IQUE,
	NDS_CONSOLE_TYPE_DSI
};

struct NDSSystem
{
	int32_t cycles;
	uint64_t timerCycle[2][4];
	uint32_t VCount;
	uint32_t old;

	uint8_t *FW_ARM9BootCode;
	uint8_t *FW_ARM7BootCode;
	uint32_t FW_ARM9BootCodeAddr;
	uint32_t FW_ARM7BootCodeAddr;
	uint32_t FW_ARM9BootCodeSize;
	uint32_t FW_ARM7BootCodeSize;

	bool sleeping;
	bool cardEjected;
	uint32_t freezeBus;

	// console type must be copied in when the system boots. it can't be changed on the fly.
	int ConsoleType;
	bool Is_DSI() { return this->ConsoleType == NDS_CONSOLE_TYPE_DSI; }

	bool isInVblank() const { return this->VCount >= 192; }
	bool isIn3dVblank() const { return this->VCount >= 192 && this->VCount < 215; }
};

#define MAX_FW_NICKNAME_LENGTH 10
#define MAX_FW_MESSAGE_LENGTH 26

struct NDS_fw_config_data
{
	NDS_CONSOLE_TYPE ds_type;

	uint8_t fav_colour;
	uint8_t birth_month;
	uint8_t birth_day;

	uint16_t nickname[MAX_FW_NICKNAME_LENGTH];
	uint8_t nickname_len;

	uint16_t message[MAX_FW_MESSAGE_LENGTH];
	uint8_t message_len;

	uint8_t language;
};

int NDS_Init (vio2sf_state *st);

void NDS_DeInit(vio2sf_state *st);

bool NDS_SetROM(vio2sf_state *st, uint8_t * rom, uint32_t mask);

struct RomBanner
{
	RomBanner(bool defaultInit);
	uint16_t version; //Version  (0001h)
	uint16_t crc16; //CRC16 across entries 020h..83Fh
	uint8_t reserved[0x1C]; //Reserved (zero-filled)
	uint8_t bitmap[0x200]; //Icon Bitmap  (32x32 pix) (4x4 tiles, each 4x8 bytes, 4bit depth)
	uint16_t palette[0x10]; //Icon Palette (16 colors, 16bit, range 0000h-7FFFh) (Color 0 is transparent, so the 1st palette entry is ignored)
	enum { NUM_TITLES = 6 };
	union
	{
		struct
		{
			uint16_t title_jp[0x80]; // Title 0 Japanese (128 characters, 16bit Unicode)
			uint16_t title_en[0x80]; // Title 1 English  ("")
			uint16_t title_fr[0x80]; // Title 2 French   ("")
			uint16_t title_de[0x80]; // Title 3 German   ("")
			uint16_t title_it[0x80]; // Title 4 Italian  ("")
			uint16_t title_es[0x80]; // Title 5 Spanish  ("")
		};
		uint16_t titles[NUM_TITLES][0x80];
	};
	uint8_t end0xFF[0x1C0];
	//840h  ?    (Maybe newer/chinese firmware do also support chinese title?)
	//840h  -    End of Icon/Title structure (next 1C0h bytes usually FFh-filled)
};

struct GameInfo
{
	GameInfo() : romdata() { }

	void loadData(char *buf, int size)
	{
		this->resize(size);
		memcpy(&this->romdata[0], buf, size);
		this->romsize = size;
		this->fillGap();
	}

	void fillGap()
	{
		memset(&this->romdata[this->romsize], 0xFF, this->allocatedSize - this->romsize);
	}

	void resize(int size)
	{
		// calculate the necessary mask for the requested size
		mask = size - 1;
		mask |= mask >> 1;
		mask |= mask >> 2;
		mask |= mask >> 4;
		mask |= mask >> 8;
		mask |= mask >> 16;

		// now, we actually need to over-allocate, because bytes from anywhere protected by that mask
		// could be read from the rom
		this->allocatedSize = mask + 4;

		this->romdata.reset(new char[allocatedSize]);
		this->romsize = size;
	}
	uint32_t crc;
	NDS_header header;
	char ROMserial[20];
	char ROMname[20];
	std::unique_ptr<char[]> romdata;
	uint32_t romsize;
	uint32_t allocatedSize;
	uint32_t mask;
	bool isHomebrew;
};

struct UserButtons : buttonstruct<bool>
{
};
struct UserTouch
{
	uint16_t touchX;
	uint16_t touchY;
	bool isTouch;
};
struct UserMicrophone
{
	uint32_t micButtonPressed;
};
struct UserInput
{
	UserButtons buttons;
	UserTouch touch;
	UserMicrophone mic;
};

void NDS_FreeROM(vio2sf_state *st);
void NDS_Reset(vio2sf_state *st);

void NDS_Sleep(vio2sf_state *st);

void execHardware_doAllDma(vio2sf_state *st, EDMAMode modeNum);

template<bool FORCE> void NDS_exec(vio2sf_state *st, int32_t nb = 560190 << 1);

struct TCommonSettings
{
	TCommonSettings() : UseExtBIOS(false), SWIFromBIOS(false), PatchSWI3(false), UseExtFirmware(false), BootFromFirmware(false), ConsoleType(NDS_CONSOLE_TYPE_FAT), rigorous_timing(false), advanced_timing(true),
		spuInterpolationMode(SPUInterpolation_Linear), manualBackupType(0), spu_captureMuted(false), spu_advanced(false)
	{
		strcpy(this->ARM9BIOS, "biosnds9.bin");
		strcpy(this->ARM7BIOS, "biosnds7.bin");
		strcpy(this->Firmware, "firmware.bin");
		NDS_FillDefaultFirmwareConfigData(&this->InternalFirmConf);

    bool solo = false;
    static char* soloEnv = strdup("SOLO_2SF_n");
    static char* muteEnv = strdup("MUTE_2SF_n");
		for (int i = 0; i < 16; ++i) {
      if (i < 10) {
        soloEnv[9] = '0' + i;
      } else {
        soloEnv[9] = 'A' + (i - 10);
      }
      char* soloVal = getenv(soloEnv);
      if (soloVal && soloVal[0] == '1') {
        solo = true;
        this->spu_muteChannels[i] = false;
      } else {
        this->spu_muteChannels[i] = true;
      }
    }
    if (!solo) {
      for (int i = 0; i < 16; ++i) {
        if (i < 10) {
          muteEnv[9] = '0' + i;
        } else {
          muteEnv[9] = 'A' + (i - 10);
        }
        char* muteVal = getenv(muteEnv);
        this->spu_muteChannels[i] = muteVal && muteVal[0] == '1';
      }
    }
	}

	bool UseExtBIOS;
	char ARM9BIOS[256];
	char ARM7BIOS[256];
	bool SWIFromBIOS;
	bool PatchSWI3;

	bool UseExtFirmware;
	char Firmware[256];
	bool BootFromFirmware;
	struct NDS_fw_config_data InternalFirmConf;

	NDS_CONSOLE_TYPE ConsoleType;

	bool rigorous_timing;

	bool advanced_timing;

	bool use_jit;
	uint32_t jit_max_block_size;

	SPUInterpolationMode spuInterpolationMode;

	// this is the user's choice of manual backup type, for cases when the autodetection can't be trusted
	int manualBackupType;

	bool spu_muteChannels[16];
	bool spu_captureMuted;
	bool spu_advanced;
};
