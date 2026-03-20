#ifndef __GYMPLAYER_HPP__
#define __GYMPLAYER_HPP__

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


#define FCC_GYM 	0x47594D00

// GYMX header (optional, added by YMAMP WinAMP plugin)
//	Ofs	Len	Description
//	000	04	"GYMX"
//	004	20	song title
//	024	20	game name
//	044	20	publisher
//	064	20	emulator ("Dumped with")
//	084	20	file creator ("Dumped by")
//	0A4	100	comments
//	1A4	04	loop start frame (0 = no loop)
//	1A8	04	uncompressed data size (0 = data is uncompressed, 1 = data is zlib compressed)

struct GYM_HEADER
{
	UINT8 hasHeader;
	UINT32 uncomprSize;
	UINT32 loopFrame;
	UINT32 dataOfs;
	UINT32 realFileSize;	// internal file size after possible decompression
};

struct GYM_PLAY_OPTIONS
{
	PLR_GEN_OPTS genOpts;
};


class GYMPlayer : public PlayerBase
{
private:
	struct DevCfg
	{
		DEV_ID type;
		UINT16 volume;
		std::vector<UINT8> data;
	};
	struct DEVLOG_CB_DATA
	{
		GYMPlayer* player;
		size_t chipDevID;
	};
	struct GYM_CHIPDEV
	{
		VGM_BASEDEV base;
		size_t optID;
		DEVFUNC_WRITE_A8D8 write;
		DEVLOG_CB_DATA logCbData;
	};
	
public:
	GYMPlayer();
	~GYMPlayer();
	
	UINT32 GetPlayerType(void) const;
	const char* GetPlayerName(void) const;
	static UINT8 PlayerCanLoadFile(DATA_LOADER *dataLoader);
	UINT8 CanLoadFile(DATA_LOADER *dataLoader) const;
	UINT8 LoadFile(DATA_LOADER *dataLoader);
	UINT8 UnloadFile(void);
	const GYM_HEADER* GetFileHeader(void) const;
	
	const char* const* GetTags(void);
	UINT8 GetSongInfo(PLR_SONG_INFO& songInf);
	UINT8 GetSongDeviceInfo(std::vector<PLR_DEV_INFO>& devInfList) const;
	UINT8 SetDeviceOptions(UINT32 id, const PLR_DEV_OPTS& devOpts);
	UINT8 GetDeviceOptions(UINT32 id, PLR_DEV_OPTS& devOpts) const;
	UINT8 SetDeviceMuting(UINT32 id, const PLR_MUTE_OPTS& muteOpts);
	UINT8 GetDeviceMuting(UINT32 id, PLR_MUTE_OPTS& muteOpts) const;
	UINT8 SetPlayerOptions(const GYM_PLAY_OPTIONS& playOpts);
	UINT8 GetPlayerOptions(GYM_PLAY_OPTIONS& playOpts) const;
	
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
	void RefreshMuting(GYM_CHIPDEV& chipDev, const PLR_MUTE_OPTS& muteOpts);
	void RefreshPanning(GYM_CHIPDEV& chipDev, const PLR_PAN_OPTS& panOpts);
	
	static bool CheckRawGYMFile(UINT32 dataLen, const UINT8* data);
	UINT8 DecompressZlibData(void);
	void CalcSongLength(void);
	UINT8 LoadTags(void);
	void LoadTag(const char* tagName, const void* data, size_t maxlen);
	std::string GetUTF8String(const char* startPtr, const char* endPtr);

	void RefreshTSRates(void);
	
	static void PlayerLogCB(void* userParam, void* source, UINT8 level, const char* message);
	static void SndEmuLogCB(void* userParam, void* source, UINT8 level, const char* message);
	
	void GenerateDeviceConfig(void);
	UINT8 SeekToTick(UINT32 tick);
	UINT8 SeekToFilePos(UINT32 pos);
	void ParseFile(UINT32 ticks);
	void DoCommand(void);
	void DoFileEnd(void);
	
	CPCONV* _cpc1252;	// CP1252 -> UTF-8 codepage conversion
	DEV_LOGGER _logger;
	DATA_LOADER* _dLoad;
	UINT32 _fileLen;
	const UINT8* _fileData;	// data pointer for quick access, equals _dLoad->GetFileData().data()
	std::vector<UINT8> _decFData;
	
	GYM_HEADER _fileHdr;
	std::vector<DevCfg> _devCfgs;
	UINT32 _tickFreq;
	UINT32 _totalTicks;
	UINT32 _loopOfs;
	std::map<std::string, std::string> _tagData;
	std::vector<const char*> _tagList;
	
	std::vector<UINT8> _pcmBuffer;
	UINT32 _pcmBaseTick;
	UINT32 _pcmInPos;	// input position (GYM -> buffer)
	UINT32 _pcmOutPos;	// output position (buffer -> YM2612)
	UINT8 _ymFreqRegs[0x20];	// cache of 0x0A0..0x0AF and 0x1A0..0x1AF frequency registers
	UINT8 _ymLatch[2];	// current latch value ([0] = normal channels, [1] = CH3 multi-freq mode registers]
	
	//UINT32 _outSmplRate;
	
	// tick/sample conversion rates
	UINT64 _tsMult;
	UINT64 _tsDiv;
	UINT64 _ttMult;
	UINT64 _lastTsMult;
	UINT64 _lastTsDiv;
	
	GYM_PLAY_OPTIONS _playOpts;
	PLR_DEV_OPTS _devOpts[2];	// 0 = YM2612, 1 = SEGA PSG
	std::vector<GYM_CHIPDEV> _devices;
	std::vector<std::string> _devNames;
	size_t _optDevMap[2];	// maps _devOpts vector index to _devices vector
	
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

#endif	// __GYMPLAYER_HPP__
