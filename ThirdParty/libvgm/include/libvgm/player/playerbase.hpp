#ifndef __PLAYERBASE_HPP__
#define __PLAYERBASE_HPP__

#include "../stdtype.h"
#include "../emu/EmuStructs.h"	// for DEV_GEN_CFG
#include "../emu/Resampler.h"	// for WAVE_32BS
#include "../utils/DataLoader.h"
#include <vector>


// GetState() bit masks
#define PLAYSTATE_PLAY	0x01	// is playing
#define PLAYSTATE_END	0x02	// has reached the end of the file
#define PLAYSTATE_PAUSE	0x04	// is paused (render wave, but don't advance in the song)
#define PLAYSTATE_SEEK	0x08	// is seeking

// GetCurPos()/Seek() units
#define PLAYPOS_FILEOFS	0x00	// file offset (in bytes)
#define PLAYPOS_TICK	0x01	// tick position (scale: internal tick rate)
#define PLAYPOS_SAMPLE	0x02	// sample number (scale: rendering sample rate)
#define PLAYPOS_COMMAND	0x03	// internal command ID

// callback functions and event constants
class PlayerBase;
typedef UINT8 (*PLAYER_EVENT_CB)(PlayerBase* player, void* userParam, UINT8 evtType, void* evtParam);
#define PLREVT_NONE		0x00
#define PLREVT_START	0x01	// playback started
#define PLREVT_STOP		0x02	// playback stopped
#define PLREVT_LOOP		0x03	// starting next loop [evtParam: UINT32* loopNumber, ret == 0x01 -> stop processing]
#define PLREVT_END		0x04	// reached the end of the song

typedef DATA_LOADER* (*PLAYER_FILEREQ_CB)(void* userParam, PlayerBase* player, const char* fileName);

typedef void (*PLAYER_LOG_CB)(void* userParam, PlayerBase* player, UINT8 level, UINT8 srcType, const char* srcTag, const char* message);
// log levels
#define PLRLOG_OFF		DEVLOG_OFF
#define PLRLOG_ERROR	DEVLOG_ERROR
#define PLRLOG_WARN		DEVLOG_WARN
#define PLRLOG_INFO		DEVLOG_INFO
#define PLRLOG_DEBUG	DEVLOG_DEBUG
#define PLRLOG_TRACE	DEVLOG_TRACE
// log source types
#define PLRLOGSRC_PLR	0x00	// player
#define PLRLOGSRC_EMU	0x01	// sound emulation


struct PLR_SONG_INFO
{
	UINT32 format;		// four-character-code for file format
	UINT16 fileVerMaj;	// file version (major, encoded as BCD)
	UINT16 fileVerMin;	// file version (minor, encoded as BCD)
	UINT32 tickRateMul;	// internal ticks per second: numerator
	UINT32 tickRateDiv;	// internal ticks per second: denumerator
	// 1 second = 1 tick * tickMult / tickDiv
	UINT32 songLen;		// song length in ticks
	UINT32 loopTick;	// tick position where the loop begins (-1 = no loop)
	INT32 volGain;		// song-specific volume gain, 16.16 fixed point factor (0x10000 = 100%)
	UINT32 deviceCnt;	// number of used sound devices
};

struct PLR_DEV_INFO
{
	UINT32 id;		// device ID
	UINT8 type;		// device type
	UINT8 instance;	// instance ID of this device type (0xFF -> N/A for this format)
	UINT16 volume;	// output volume (0x100 = 100%)
	UINT32 core;	// FCC of device emulation core
	UINT32 smplRate;	// current sample rate (0 if not running)
	const DEV_GEN_CFG* devCfg;	// device configuration parameters
};

struct PLR_MUTE_OPTS
{
	UINT8 disable;		// suspend emulation (0x01 = main device, 0x02 = linked, 0xFF = all)
	UINT32 chnMute[2];	// channel muting mask ([1] is used for linked devices)
};
struct PLR_PAN_OPTS
{
	INT16 chnPan[2][32];	// channel panning [TODO: rethink how this should be really configured]
};

#define PLR_DEV_ID(chip, instance)	(0x80000000 | (instance << 16) | (chip << 0))

struct PLR_DEV_OPTS
{
	UINT32 emuCore[2];	// enforce a certain sound core (0 = use default, [1] is used for linked devices)
	UINT8 srMode;		// sample rate mode (see DEVRI_SRMODE)
	UINT8 resmplMode;	// resampling mode (0 - high quality, 1 - low quality, 2 - LQ down, HQ up)
	UINT32 smplRate;	// emulaiton sample rate
	UINT32 coreOpts;
	PLR_MUTE_OPTS muteOpts;
	PLR_PAN_OPTS panOpts;
};


//	--- concept ---
//	- Player class does file rendering at fixed volume (but changeable speed)
//	- host program handles master volume + fading + stopping after X loops (notified via callback)

// TODO: rename to "PlayerEngine"
class PlayerBase
{
public:
	PlayerBase();
	virtual ~PlayerBase();
	
	virtual UINT32 GetPlayerType(void) const;
	virtual const char* GetPlayerName(void) const;
	static UINT8 PlayerCanLoadFile(DATA_LOADER *dataLoader);
	virtual UINT8 CanLoadFile(DATA_LOADER *dataLoader) const;
	virtual UINT8 LoadFile(DATA_LOADER *dataLoader) = 0;
	virtual UINT8 UnloadFile(void) = 0;
	
	virtual const char* const* GetTags(void) = 0;
	virtual UINT8 GetSongInfo(PLR_SONG_INFO& songInf) = 0;
	virtual UINT8 GetSongDeviceInfo(std::vector<PLR_DEV_INFO>& devInfList) const = 0;
	static UINT8 InitDeviceOptions(PLR_DEV_OPTS& devOpts);
	virtual UINT8 SetDeviceOptions(UINT32 id, const PLR_DEV_OPTS& devOpts) = 0;
	virtual UINT8 GetDeviceOptions(UINT32 id, PLR_DEV_OPTS& devOpts) const = 0;
	virtual UINT8 SetDeviceMuting(UINT32 id, const PLR_MUTE_OPTS& muteOpts) = 0;
	virtual UINT8 GetDeviceMuting(UINT32 id, PLR_MUTE_OPTS& muteOpts) const = 0;
	// player-specific options
	//virtual UINT8 SetPlayerOptions(const ###_PLAY_OPTIONS& playOpts) = 0;
	//virtual UINT8 GetPlayerOptions(###_PLAY_OPTIONS& playOpts) const = 0;
	
	virtual UINT32 GetSampleRate(void) const;
	virtual UINT8 SetSampleRate(UINT32 sampleRate);
	virtual UINT8 SetPlaybackSpeed(double speed);
	virtual void SetEventCallback(PLAYER_EVENT_CB cbFunc, void* cbParam);
	virtual void SetFileReqCallback(PLAYER_FILEREQ_CB cbFunc, void* cbParam);
	virtual void SetLogCallback(PLAYER_LOG_CB cbFunc, void* cbParam);
	virtual UINT32 Tick2Sample(UINT32 ticks) const = 0;
	virtual UINT32 Sample2Tick(UINT32 samples) const = 0;
	virtual double Tick2Second(UINT32 ticks) const = 0;
	virtual double Sample2Second(UINT32 samples) const;
	
	virtual UINT8 GetState(void) const = 0;			// get playback state (playing / paused / ...)
	virtual UINT32 GetCurPos(UINT8 unit) const = 0;	// get current playback position
	virtual UINT32 GetCurLoop(void) const = 0;		// get current loop index (0 = 1st loop, 1 = 2nd loop, ...)
	virtual UINT32 GetTotalTicks(void) const = 0;	// get time for playing once in ticks
	virtual UINT32 GetLoopTicks(void) const = 0;	// get time for one loop in ticks
	virtual UINT32 GetTotalPlayTicks(UINT32 numLoops) const;	// get time for playing + looping (without fading)
	
	virtual UINT8 Start(void) = 0;
	virtual UINT8 Stop(void) = 0;
	virtual UINT8 Reset(void) = 0;
	virtual UINT8 Seek(UINT8 unit, UINT32 pos) = 0; // seek to playback position
	virtual UINT32 Render(UINT32 smplCnt, WAVE_32BS* data) = 0;
	
protected:
	UINT32 _outSmplRate;
	PLAYER_EVENT_CB _eventCbFunc;
	void* _eventCbParam;
	PLAYER_FILEREQ_CB _fileReqCbFunc;
	void* _fileReqCbParam;
	PLAYER_LOG_CB _logCbFunc;
	void* _logCbParam;
};

#endif	// __PLAYERBASE_HPP__
