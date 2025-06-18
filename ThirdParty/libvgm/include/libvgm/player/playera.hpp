#ifndef __PLAYERA_HPP__
#define __PLAYERA_HPP__

#include <vector>
#include "../stdtype.h"
#include "../utils/DataLoader.h"
#include "../emu/Resampler.h"	// for WAVE_32BS
#include "playerbase.hpp"

#define PLAYSTATE_FADE	0x10	// is fading
#define PLAYSTATE_FIN	0x20	// finished playing (file end + fading + trailing silence)

#define PLAYTIME_LOOP_EXCL	0x00	// excluding loops, jumps back in time when the file loops
#define PLAYTIME_LOOP_INCL	0x01	// including loops, no jumping back
#define PLAYTIME_TIME_FILE	0x00	// file time, progresses slower/faster when playback speed is adjusted
#define PLAYTIME_TIME_PBK	0x02	// playback time, file duration will be longer/shorter when playback speed is adjusted
#define PLAYTIME_WITH_FADE	0x10	// include fade out time (looping songs only)
#define PLAYTIME_WITH_SLNC	0x20	// include silence after songs

// TODO: find a proper name for this class
class PlayerA
{
public:
	struct Config
	{
		INT32 masterVol;	// master volume (16.16 fixed point, negative value = phase inversion)
		bool ignoreVolGain;	// ignore track-specific volume gain
		UINT8 chnInvert;	// channel phase inversion (bit 0 - left, bit 1 - right)
		UINT32 loopCount;
		UINT32 fadeSmpls;
		UINT32 endSilenceSmpls;
		double pbSpeed;
	};
	typedef void (*PLR_SMPL_PACK)(void* buffer, INT32 value);

	PlayerA();
	~PlayerA();
	void RegisterPlayerEngine(PlayerBase* player);
	void UnregisterAllPlayers(void);
	const std::vector<PlayerBase*>& GetRegisteredPlayers(void) const;
	
	UINT8 SetOutputSettings(UINT32 smplRate, UINT8 channels, UINT8 smplBits, UINT32 smplBufferLen);
	UINT32 GetSampleRate(void) const;
	void SetSampleRate(UINT32 sampleRate);
	double GetPlaybackSpeed(void) const;
	void SetPlaybackSpeed(double speed);
	UINT32 GetLoopCount(void) const;
	void SetLoopCount(UINT32 loops);
	INT32 GetMasterVolume(void) const;
	void SetMasterVolume(INT32 volume);
	INT32 GetSongVolume(void) const;
	UINT32 GetFadeSamples(void) const;
	void SetFadeSamples(UINT32 smplCnt);
	UINT32 GetEndSilenceSamples(void) const;
	void SetEndSilenceSamples(UINT32 smplCnt);
	const Config& GetConfiguration(void) const;
	void SetConfiguration(const Config& config);

	void SetEventCallback(PLAYER_EVENT_CB cbFunc, void* cbParam);
	void SetFileReqCallback(PLAYER_FILEREQ_CB cbFunc, void* cbParam);
	void SetLogCallback(PLAYER_LOG_CB cbFunc, void* cbParam);
	UINT8 GetState(void) const;
	UINT32 GetCurPos(UINT8 unit) const;
	double GetCurTime(UINT8 flags) const;
	double GetTotalTime(UINT8 flags) const;
	UINT32 GetCurLoop(void) const;
	double GetLoopTime(void) const;
	PlayerBase* GetPlayer(void);
	const PlayerBase* GetPlayer(void) const;
	
	UINT8 LoadFile(DATA_LOADER* dLoad);
	UINT8 UnloadFile(void);
	UINT32 GetFileSize(void);
	UINT8 Start(void);
	UINT8 Stop(void);
	UINT8 Reset(void);
	UINT8 FadeOut(void);
	UINT8 Seek(UINT8 unit, UINT32 pos);
	UINT32 Render(UINT32 bufSize, void* data);
private:
	void FindPlayerEngine(void);
	INT32 CalcSongVolume(void);
	INT32 CalcCurrentVolume(UINT32 playbackSmpl);
	static UINT8 PlayCallbackS(PlayerBase* player, void* userParam, UINT8 evtType, void* evtParam);
	UINT8 PlayCallback(PlayerBase* player, UINT8 evtType, void* evtParam);
	
	std::vector<PlayerBase*> _avbPlrs;	// available players
	UINT32 _smplRate;
	Config _config;
	PLAYER_EVENT_CB _plrCbFunc;
	void* _plrCbParam;
	UINT8 _myPlayState;
	
	UINT8 _outSmplChns;
	UINT8 _outSmplBits;
	UINT32 _outSmplSize1;	// for 1 channel
	UINT32 _outSmplSizeA;	// for all channels
	PLR_SMPL_PACK _outSmplPack;
	std::vector<WAVE_32BS> _smplBuf;
	PlayerBase* _player;
	DATA_LOADER* _dLoad;
	INT32 _songVolume;
	UINT32 _fadeSmplStart;
	UINT32 _endSilenceStart;
};

#endif	// __PLAYERA_HPP__
