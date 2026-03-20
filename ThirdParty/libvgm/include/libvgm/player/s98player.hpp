#ifndef __S98PLAYER_HPP__
#define __S98PLAYER_HPP__

#include "../stdtype.h"
#include "../emu/EmuStructs.h"
#include "../emu/Resampler.h"
#include "../utils/StrUtils.h"
#include "helper.h"
#include "playerbase.hpp"
#include "../utils/DataLoader.h"
#include "../emu/logging.h"
#include <vector>
#include <map>
#include <string>


#define FCC_S98 	0x53393800

struct S98_HEADER
{
	UINT8 fileVer;
	UINT32 tickMult;	// [v1] tick timing numerator
	UINT32 tickDiv;		// [v2] tick timing denumerator
	UINT32 compression;	// [v1: 0 - no compression, >0 - size of uncompressed data] [v2: ??] [v3: must be 0]
	UINT32 tagOfs;		// [v1/2: song title file offset] [v3: tag data file offset]
	UINT32 dataOfs;		// play data file offset
	UINT32 loopOfs;		// loop file offset
};
struct S98_DEVICE
{
	UINT32 devType;
	UINT32 clock;
	UINT32 pan;			// [v2: reserved] [v3: pan setting]
	UINT32 app_spec;	// [v2: application-specific] [v3: reserved]
};
struct S98_PLAY_OPTIONS
{
	PLR_GEN_OPTS genOpts;
};


class S98Player : public PlayerBase
{
private:
	struct DevCfgBuffer
	{
		std::vector<UINT8> data;
	};
	struct DEVLOG_CB_DATA
	{
		S98Player* player;
		size_t chipDevID;
	};
	struct S98_CHIPDEV
	{
		VGM_BASEDEV base;
		size_t optID;
		std::vector<UINT8> cfg;
		DEVFUNC_WRITE_A8D8 write;
		DEVLOG_CB_DATA logCbData;
	};
	struct DEVLINK_CB_DATA
	{
		S98Player* player;
		S98_CHIPDEV* chipDev;
	};
	
public:
	S98Player();
	~S98Player();
	
	UINT32 GetPlayerType(void) const;
	const char* GetPlayerName(void) const;
	static UINT8 PlayerCanLoadFile(DATA_LOADER *dataLoader);
	UINT8 CanLoadFile(DATA_LOADER *dataLoader) const;
	UINT8 LoadFile(DATA_LOADER *dataLoader);
	UINT8 UnloadFile(void);
	const S98_HEADER* GetFileHeader(void) const;
	
	const char* const* GetTags(void);
	UINT8 GetSongInfo(PLR_SONG_INFO& songInf);
	UINT8 GetSongDeviceInfo(std::vector<PLR_DEV_INFO>& devInfList) const;
	UINT8 SetDeviceOptions(UINT32 id, const PLR_DEV_OPTS& devOpts);
	UINT8 GetDeviceOptions(UINT32 id, PLR_DEV_OPTS& devOpts) const;
	UINT8 SetDeviceMuting(UINT32 id, const PLR_MUTE_OPTS& muteOpts);
	UINT8 GetDeviceMuting(UINT32 id, PLR_MUTE_OPTS& muteOpts) const;
	UINT8 SetPlayerOptions(const S98_PLAY_OPTIONS& playOpts);
	UINT8 GetPlayerOptions(S98_PLAY_OPTIONS& playOpts) const;
	
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
	size_t GetDeviceInstance(size_t id) const;
	size_t DeviceID2OptionID(UINT32 id) const;
	void RefreshMuting(S98_CHIPDEV& chipDev, const PLR_MUTE_OPTS& muteOpts);
	void RefreshPanning(S98_CHIPDEV& chipDev, const PLR_PAN_OPTS& panOpts);
	
	void CalcSongLength(void);
	UINT8 LoadTags(void);
	std::string GetUTF8String(const char* startPtr, const char* endPtr);
	UINT8 ParsePSFTags(const std::string& tagData);
	UINT32 ReadVarInt(UINT32& filePos);
	
	void RefreshTSRates(void);
	
	static void PlayerLogCB(void* userParam, void* source, UINT8 level, const char* message);
	static void SndEmuLogCB(void* userParam, void* source, UINT8 level, const char* message);
	
	void GenerateDeviceConfig(void);
	static void DeviceLinkCallback(void* userParam, VGM_BASEDEV* cDev, DEVLINK_INFO* dLink);
	UINT8 SeekToTick(UINT32 tick);
	UINT8 SeekToFilePos(UINT32 pos);
	void ParseFile(UINT32 ticks);
	void HandleEOF(void);
	void DoCommand(void);
	void DoRegWrite(UINT8 deviceID, UINT8 port, UINT8 reg, UINT8 data);
	
	enum
	{
		_OPT_DEV_COUNT = 0x0A
	};
	
	CPCONV* _cpcSJIS;	// ShiftJIS -> UTF-8 codepage conversion
	DEV_LOGGER _logger;
	DATA_LOADER *_dLoad;
	const UINT8* _fileData;	// data pointer for quick access, equals _dLoad->GetFileData().data()
	
	S98_HEADER _fileHdr;
	std::vector<S98_DEVICE> _devHdrs;
	std::vector<DevCfgBuffer> _devCfgs;
	UINT32 _totalTicks;
	UINT32 _loopTick;
	std::map<std::string, std::string> _tagData;
	std::vector<const char*> _tagList;
	
	//UINT32 _outSmplRate;
	
	// tick/sample conversion rates
	UINT64 _tsMult;
	UINT64 _tsDiv;
	UINT64 _ttMult;
	UINT64 _lastTsMult;
	UINT64 _lastTsDiv;
	
	static const DEV_ID _OPT_DEV_LIST[_OPT_DEV_COUNT];	// list of configurable libvgm devices
	
	S98_PLAY_OPTIONS _playOpts;
	PLR_DEV_OPTS _devOpts[_OPT_DEV_COUNT * 2];	// space for 2 instances per chip
	size_t _devOptMap[0x100][2];	// maps libvgm device ID to _devOpts vector
	std::vector<S98_CHIPDEV> _devices;
	std::vector<std::string> _devNames;
	size_t _optDevMap[_OPT_DEV_COUNT * 2];	// maps _devOpts vector index to _devices vector
	
	UINT32 _filePos;
	UINT32 _fileTick;
	UINT32 _playTick;
	UINT32 _playSmpl;
	UINT32 _curLoop;
	UINT32 _lastLoopTick;
	
	UINT8 _playState;
	UINT8 _psTrigger;	// used to temporarily trigger special commands
	//PLAYER_EVENT_CB _eventCbFunc;
	//void* _eventCbParam;
};

#endif	// __S98PLAYER_HPP__
