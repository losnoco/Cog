#ifndef __DROPLAYER_HPP__
#define __DROPLAYER_HPP__

#include "../stdtype.h"
#include "../emu/EmuStructs.h"
#include "../emu/Resampler.h"
#include "helper.h"
#include "playerbase.hpp"
#include "../utils/DataLoader.h"
#include "../emu/logging.h"
#include <vector>
#include <string>


#define FCC_DRO 	0x44524F00

// DRO v0 header (DOSBox 0.62, SVN r1864)
//	Ofs	Len	Description
//	00	08	"DBRAWOPL"
//	04	04	data length in milliseconds
//	08	04	data length in bytes
//	0C	01	hardware type (0 = OPL2, 1 = OPL3, 2 = DualOPL2)

// DRO v1 header (DOSBox 0.63, SVN r2065)
//	Ofs	Len	Description
//	00	08	"DBRAWOPL"
//	04	04	version minor
//	08	04	version major
//	0C	04	data length in milliseconds
//	10	04	data length in bytes
//	14	04	hardware type (0 = OPL2, 1 = OPL3, 2 = DualOPL2)

// DRO v2 header (DOSBox 0.73, SVN r3178)
//	Ofs	Len	Description
//	00	08	"DBRAWOPL"
//	04	04	version major
//	08	04	version minor
//	0C	04	data length in "pairs" (1 pair = 2 bytes, a pair consists of command + data)
//	10	04	data length in milliseconds
//	14	01	hardware type (0 = OPL2, 1 = DualOPL2, 2 = OPL3)
//	15	01	data format (0 = interleaved command/data)
//	16	01	compression (0 = no compression)
//	17	01	command code for short delay
//	18	01	command code for long delay
//	19	01	size of register codemap cl
//	1A	cl	register codemap

struct DRO_HEADER
{
	UINT16 verMajor;
	UINT16 verMinor;
	UINT32 dataSize;	// in bytes
	UINT32 lengthMS;
	UINT8 hwType;
	UINT8 format;
	UINT8 compression;
	UINT8 cmdDlyShort;
	UINT8 cmdDlyLong;
	UINT8 regCmdCnt;
	UINT8 regCmdMap[0x80];
	UINT32 dataOfs;
};

// DRO v2 often incorrectly specify DualOPL2 instead of OPL3
// These constants allow a configuration of how to handle DualOPL2 in DRO v2 files.
#define DRO_V2OPL3_DETECT	0x00	// scan the initialization block and use OPL3 if "OPL3 enable" if found [default]
#define DRO_V2OPL3_HEADER	0x01	// strictly follow the DRO header
#define DRO_V2OPL3_ENFORCE	0x02	// always enforce OPL3 mode when the DRO says DualOPL2
struct DRO_PLAY_OPTIONS
{
	PLR_GEN_OPTS genOpts;
	UINT8 v2opl3Mode;	// DRO v2 DualOPL2 -> OPL3 fixes
};


class DROPlayer : public PlayerBase
{
private:
	struct DEVLOG_CB_DATA
	{
		DROPlayer* player;
		size_t chipDevID;
	};
	struct DRO_CHIPDEV
	{
		VGM_BASEDEV base;
		size_t optID;
		DEVFUNC_WRITE_A8D8 write;
		DEVLOG_CB_DATA logCbData;
	};
	
public:
	DROPlayer();
	~DROPlayer();
	
	UINT32 GetPlayerType(void) const;
	const char* GetPlayerName(void) const;
	static UINT8 PlayerCanLoadFile(DATA_LOADER *dataLoader);
	UINT8 CanLoadFile(DATA_LOADER *dataLoader) const;
	UINT8 LoadFile(DATA_LOADER *dataLoader);
	UINT8 UnloadFile(void);
	const DRO_HEADER* GetFileHeader(void) const;
	
	const char* const* GetTags(void);
	UINT8 GetSongInfo(PLR_SONG_INFO& songInf);
	UINT8 GetSongDeviceInfo(std::vector<PLR_DEV_INFO>& devInfList) const;
	UINT8 SetDeviceOptions(UINT32 id, const PLR_DEV_OPTS& devOpts);
	UINT8 GetDeviceOptions(UINT32 id, PLR_DEV_OPTS& devOpts) const;
	UINT8 SetDeviceMuting(UINT32 id, const PLR_MUTE_OPTS& muteOpts);
	UINT8 GetDeviceMuting(UINT32 id, PLR_MUTE_OPTS& muteOpts) const;
	UINT8 SetPlayerOptions(const DRO_PLAY_OPTIONS& playOpts);
	UINT8 GetPlayerOptions(DRO_PLAY_OPTIONS& playOpts) const;
	
	//UINT32 GetSampleRate(void) const;
	UINT8 SetSampleRate(UINT32 sampleRate);
	double GetPlaybackSpeed(void) const;
	UINT8 SetPlaybackSpeed(double speed);
	//void SetEventCallback(PLAYER_EVENT_CB cbFunc, void* cbParam);
	UINT32 Tick2Sample(UINT32 ticks) const;
	UINT32 Sample2Tick(UINT32 samples) const;
	double Tick2Second(UINT32 ticks) const;
	//double Sample2Second(UINT32 samples) const;
	
	UINT8 GetState(void) const;
	UINT32 GetCurPos(UINT8 unit) const;
	UINT32 GetCurLoop(void) const;
	UINT32 GetTotalTicks(void) const;	// get time for playing once in ticks
	UINT32 GetLoopTicks(void) const;	// get time for one loop in ticks
	//UINT32 GetTotalPlayTicks(UINT32 numLoops) const;	// get time for playing + looping (without fading)
	
	UINT8 Start(void);
	UINT8 Stop(void);
	UINT8 Reset(void);
	UINT8 Seek(UINT8 unit, UINT32 pos);
	UINT32 Render(UINT32 smplCnt, WAVE_32BS* data);
	
private:
	size_t DeviceID2OptionID(UINT32 id) const;
	void RefreshMuting(DRO_CHIPDEV& chipDev, const PLR_MUTE_OPTS& muteOpts);
	void RefreshPanning(DRO_CHIPDEV& chipDev, const PLR_PAN_OPTS& panOpts);
	
	void ScanInitBlock(void);

	void RefreshTSRates(void);
	
	static void PlayerLogCB(void* userParam, void* source, UINT8 level, const char* message);
	static void SndEmuLogCB(void* userParam, void* source, UINT8 level, const char* message);
	
	void GenerateDeviceConfig(void);
	UINT8 SeekToTick(UINT32 tick);
	UINT8 SeekToFilePos(UINT32 pos);
	void ParseFile(UINT32 ticks);
	void DoCommand_v1(void);
	void DoCommand_v2(void);
	void DoFileEnd(void);
	void WriteReg(UINT8 port, UINT8 reg, UINT8 data);
	
	DEV_LOGGER _logger;
	DATA_LOADER* _dLoad;
	const UINT8* _fileData;	// data pointer for quick access, equals _dLoad->GetFileData().data()
	
	DRO_HEADER _fileHdr;
	std::vector<DEV_ID> _devTypes;
	std::vector<UINT8> _devPanning;
	std::vector<DEV_GEN_CFG> _devCfgs;
	UINT8 _realHwType;
	UINT8 _portShift;	// 0 for OPL2 (1 port per chip), 1 for OPL3 (2 ports per chip)
	UINT8 _portMask;	// (1 << _portShift) - 1
	UINT32 _tickFreq;
	UINT32 _totalTicks;
	
	// information about the initialization block (ends with first delay)
	UINT32 _initBlkEndOfs;	// offset of end of init. block (for special fixes)
	std::vector<bool> _initRegSet;	// registers set during init. block
	UINT8 _initOPL3Enable;	// value of OPL3 enable register set during init. block
	
	//UINT32 _outSmplRate;
	
	// tick/sample conversion rates
	UINT64 _tsMult;
	UINT64 _tsDiv;
	UINT64 _ttMult;
	UINT64 _lastTsMult;
	UINT64 _lastTsDiv;
	
	DRO_PLAY_OPTIONS _playOpts;
	PLR_DEV_OPTS _devOpts[3];	// 0 = 1st OPL2, 1 = 2nd OPL2, 2 = 1st OPL3
	std::vector<DRO_CHIPDEV> _devices;
	std::vector<std::string> _devNames;
	size_t _optDevMap[3];	// maps _devOpts vector index to _devices vector
	
	UINT32 _filePos;
	UINT32 _fileTick;
	UINT32 _playTick;
	UINT32 _playSmpl;
	
	UINT8 _playState;
	UINT8 _psTrigger;	// used to temporarily trigger special commands
	UINT8 _selPort;		// currently selected OPL chip (for DRO v1)
	//PLAYER_EVENT_CB _eventCbFunc;
	//void* _eventCbParam;
};

#endif	// __DROPLAYER_HPP__
