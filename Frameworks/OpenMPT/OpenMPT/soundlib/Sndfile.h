/*
 * Sndfile.h
 * ---------
 * Purpose: Core class of the playback engine. Every song is represented by a CSoundFile object.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"

#include "SoundFilePlayConfig.h"
#include "MixerSettings.h"
#include "../common/misc_util.h"
#include "../common/mptRandom.h"
#include "../common/version.h"
#include <vector>
#include <bitset>
#include <set>
#include "Snd_defs.h"
#include "tuningbase.h"
#include "MIDIMacros.h"
#ifdef MODPLUG_TRACKER
#include "../mptrack/MIDIMapping.h"
#endif // MODPLUG_TRACKER

#include "Mixer.h"
#include "Resampler.h"
#ifndef NO_REVERB
#include "../sounddsp/Reverb.h"
#endif
#ifndef NO_AGC
#include "../sounddsp/AGC.h"
#endif
#ifndef NO_DSP
#include "../sounddsp/DSP.h"
#endif
#ifndef NO_EQ
#include "../sounddsp/EQ.h"
#endif

#include "modcommand.h"
#include "ModSample.h"
#include "ModInstrument.h"
#include "ModChannel.h"
#include "plugins/PluginStructs.h"
#include "RowVisitor.h"
#include "Message.h"
#include "pattern.h"
#include "patternContainer.h"
#include "ModSequence.h"

#include "../common/FileReaderFwd.h"


OPENMPT_NAMESPACE_BEGIN


bool SettingCacheCompleteFileBeforeLoading();


// -----------------------------------------------------------------------------
// MODULAR ModInstrument FIELD ACCESS : body content in InstrumentExtensions.cpp
// -----------------------------------------------------------------------------
#ifndef MODPLUG_NO_FILESAVE
void WriteInstrumentHeaderStructOrField(ModInstrument * input, std::ostream &file, uint32 only_this_code = -1 /* -1 for all */, uint16 fixedsize = 0);
#endif // !MODPLUG_NO_FILESAVE
bool ReadInstrumentHeaderField(ModInstrument * input, uint32 fcode, uint16 fsize, FileReader &file);
// --------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------


#ifdef LIBOPENMPT_BUILD
#ifndef NO_PLUGINS
class CVstPluginManager;
#endif
#endif


using PlayBehaviourSet = std::bitset<kMaxPlayBehaviours>;

#ifdef MODPLUG_TRACKER

// For WAV export (writing pattern positions to file)
struct PatternCuePoint
{
	uint64     offset;    // offset in the file (in samples)
	ORDERINDEX order;     // which order is this?
	bool       processed; // has this point been processed by the main WAV render function yet?
};

#endif // MODPLUG_TRACKER


// Return values for GetLength()
struct GetLengthType
{
	double     duration = 0.0;                 // Total time in seconds
	ROWINDEX   lastRow = ROWINDEX_INVALID;     // Last parsed row (if no target is specified, this is the first row that is parsed twice, i.e. not the *last* played order)
	ROWINDEX   endRow = ROWINDEX_INVALID;      // Last row before module loops (UNDEFINED if a target is specified)
	ROWINDEX   startRow = 0;                   // First row of parsed subsong
	ORDERINDEX lastOrder = ORDERINDEX_INVALID; // Last parsed order (see lastRow remark)
	ORDERINDEX endOrder = ORDERINDEX_INVALID;  // Last order before module loops (UNDEFINED if a target is specified)
	ORDERINDEX startOrder = 0;                 // First order of parsed subsong
	bool       targetReached = false;          // True if the specified order/row combination or duration has been reached while going through the module
};


// Target seek mode for GetLength()
struct GetLengthTarget
{
	ROWINDEX startRow;
	ORDERINDEX startOrder;
	SEQUENCEINDEX sequence;
	
	struct pos_type
	{
		ROWINDEX row;
		ORDERINDEX order;
	};

	union
	{
		double time;
		pos_type pos;
	};

	enum Mode
	{
		NoTarget,       // Don't seek, i.e. return complete length of the first subsong.
		GetAllSubsongs, // Same as NoTarget (i.e. get complete length), but returns the length of all sub songs
		SeekPosition,   // Seek to given pattern position.
		SeekSeconds,    // Seek to given time.
	} mode;

	// Don't seek, i.e. return complete module length.
	GetLengthTarget(bool allSongs = false)
	{
		mode = allSongs ? GetAllSubsongs : NoTarget;
		sequence = SEQUENCEINDEX_INVALID;
		startOrder = 0;
		startRow = 0;
	}

	// Seek to given pattern position if position is valid.
	GetLengthTarget(ORDERINDEX order, ROWINDEX row)
	{
		mode = NoTarget;
		sequence = SEQUENCEINDEX_INVALID;
		startOrder = 0;
		startRow = 0;
		if(order != ORDERINDEX_INVALID && row != ROWINDEX_INVALID)
		{
			mode = SeekPosition;
			pos.row = row;
			pos.order = order;
		}
	}

	// Seek to given time if t is valid (i.e. not negative).
	GetLengthTarget(double t)
	{
		mode = NoTarget;
		sequence = SEQUENCEINDEX_INVALID;
		startOrder = 0;
		startRow = 0;
		if(t >= 0.0)
		{
			mode = SeekSeconds;
			time = t;
		}
	}

	// Set start position from which seeking should begin.
	GetLengthTarget &StartPos(SEQUENCEINDEX seq, ORDERINDEX order, ROWINDEX row)
	{
		sequence = seq;
		startOrder = order;
		startRow = row;
		return *this;
	}
};


// Reset mode for GetLength()
enum enmGetLengthResetMode
{
	// Never adjust global variables / mod parameters
	eNoAdjust = 0x00,
	// Mod parameters (such as global volume, speed, tempo, etc...) will always be memorized if the target was reached (i.e. they won't be reset to the previous values).  If target couldn't be reached, they are reset to their default values.
	eAdjust = 0x01,
	// Same as above, but global variables will only be memorized if the target could be reached. This does *NOT* influence the visited rows vector - it will *ALWAYS* be adjusted in this mode.
	eAdjustOnSuccess = 0x02 | eAdjust,
	// Same as previous option, but will also try to emulate sample playback so that voices from previous patterns will sound when continuing playback at the target position.
	eAdjustSamplePositions = 0x04 | eAdjustOnSuccess,
};


// Delete samples assigned to instrument
enum deleteInstrumentSamples
{
	deleteAssociatedSamples,
	doNoDeleteAssociatedSamples,
};


namespace Tuning {
class CTuningCollection;
} // namespace Tuning
using CTuningCollection = Tuning::CTuningCollection;
struct CModSpecifications;
class OPL;
#ifdef MODPLUG_TRACKER
class CModDoc;
#endif // MODPLUG_TRACKER


/////////////////////////////////////////////////////////////////////////
// File edit history

#define HISTORY_TIMER_PRECISION	18.2

struct FileHistory
{
	// Date when the file was loaded in the the tracker or created.
	tm loadDate = {};
	// Time the file was open in the editor, in 1/18.2th seconds (frequency of a standard DOS timer, to keep compatibility with Impulse Tracker easy).
	uint32 openTime = 0;
	// Return the date as a (possibly truncated if not enough precision is available) ISO 8601 formatted date.
	mpt::ustring AsISO8601() const;
	// Returns true if the date component is valid. Some formats only store edit time, not edit date.
	bool HasValidDate() const { return loadDate.tm_mday != 0; }
};


struct TimingInfo
{
	double InputLatency = 0.0; // seconds
	double OutputLatency = 0.0; // seconds
	int64 StreamFrames = 0;
	uint64 SystemTimestamp = 0; // nanoseconds
	double Speed = 1.0;
};


enum class ModMessageHeuristicOrder
{
	Instruments,
	Samples,
	InstrumentsSamples,
	SamplesInstruments,
	BothInstrumentsSamples,
	BothSamplesInstruments,
	Default = InstrumentsSamples,
};

struct ModFormatDetails
{
	mpt::ustring formatName;         // "FastTracker 2"
	mpt::ustring type;               // "xm"
	mpt::ustring madeWithTracker;    // "OpenMPT 1.28.01.00"
	mpt::ustring originalFormatName; // "FastTracker 2" in the case of converted formats like MO3 or GDM
	mpt::ustring originalType;       // "xm" in the case of converted formats like MO3 or GDM
	mpt::Charset charset = mpt::Charset::UTF8;
};


class IAudioReadTarget
{
protected:
	virtual ~IAudioReadTarget() = default;
public:
	virtual void DataCallback(MixSampleInt *MixSoundBuffer, std::size_t channels, std::size_t countChunk) = 0;
	virtual void DataCallback(MixSampleFloat *MixSoundBuffer, std::size_t channels, std::size_t countChunk) = 0;
};


class IAudioSource
{
public:
	virtual ~IAudioSource() = default;
public:
	virtual void FillCallback(MixSampleInt * const *MixSoundBuffers, std::size_t channels, std::size_t countChunk) = 0;
	virtual void FillCallback(MixSampleFloat * const *MixSoundBuffers, std::size_t channels, std::size_t countChunk) = 0;
};


class AudioSourceNone
	: public IAudioSource
{
public:
	void FillCallback(MixSampleInt * const *MixSoundBuffers, std::size_t channels, std::size_t countChunk) override
	{
		for(std::size_t channel = 0; channel < channels; ++channel)
		{
			for(std::size_t frame = 0; frame < countChunk; ++frame)
			{
				MixSoundBuffers[channel][frame] = 0;
			}
		}
	}
	void FillCallback(MixSampleFloat * const *MixSoundBuffers, std::size_t channels, std::size_t countChunk) override
	{
		for(std::size_t channel = 0; channel < channels; ++channel)
		{
			for(std::size_t frame = 0; frame < countChunk; ++frame)
			{
				MixSoundBuffers[channel][frame] = MixSampleFloat(0.0);
			}
		}
	}
};


using NoteName = mpt::uchar[4];


class CSoundFile
{
	friend class GetLengthMemory;

public:
#ifdef MODPLUG_TRACKER
	void ChangeModTypeTo(const MODTYPE newType);
#endif // MODPLUG_TRACKER

	// Returns value in seconds. If given position won't be played at all, returns -1.
	// If updateVars is true, the state of various playback variables will be updated according to the playback position.
	// If updateSamplePos is also true, the sample positions of samples still playing from previous patterns will be kept in sync.
	double GetPlaybackTimeAt(ORDERINDEX ord, ROWINDEX row, bool updateVars, bool updateSamplePos);

	//Tuning-->
public:
	static std::unique_ptr<CTuning> CreateTuning12TET(const mpt::ustring &name);
	static CTuning *GetDefaultTuning() {return nullptr;}
	CTuningCollection& GetTuneSpecificTunings() {return *m_pTuningsTuneSpecific;}

	mpt::ustring GetNoteName(const ModCommand::NOTE note, const INSTRUMENTINDEX inst) const;
	mpt::ustring GetNoteName(const ModCommand::NOTE note) const;
	static mpt::ustring GetNoteName(const ModCommand::NOTE note, const NoteName *noteNames);
#ifdef MODPLUG_TRACKER
public:
	static void SetDefaultNoteNames();
	static const NoteName *GetDefaultNoteNames();
	static mpt::ustring GetDefaultNoteName(int note)  // note = [0..11]
	{
		return m_NoteNames[note];
	}
private:
	static const NoteName *m_NoteNames;
#else
private:
	const NoteName *m_NoteNames;
#endif

private:
	CTuningCollection* m_pTuningsTuneSpecific = nullptr;

#ifdef MODPLUG_TRACKER
public:
	CMIDIMapper& GetMIDIMapper() {return m_MIDIMapper;}
	const CMIDIMapper& GetMIDIMapper() const {return m_MIDIMapper;}
private:
	CMIDIMapper m_MIDIMapper;

#endif // MODPLUG_TRACKER

private: //Misc private methods.
	static void SetModSpecsPointer(const CModSpecifications*& pModSpecs, const MODTYPE type);

private: //Misc data
	const CModSpecifications *m_pModSpecs;

private:
	// Interleaved Front Mix Buffer (Also room for interleaved rear mix)
	mixsample_t MixSoundBuffer[MIXBUFFERSIZE * 4];
	mixsample_t MixRearBuffer[MIXBUFFERSIZE * 2];
	// Non-interleaved plugin processing buffer
	float MixFloatBuffer[2][MIXBUFFERSIZE];
	mixsample_t MixInputBuffer[NUMMIXINPUTBUFFERS][MIXBUFFERSIZE];

	// End-of-sample pop reduction tail level
	mixsample_t m_dryLOfsVol = 0, m_dryROfsVol = 0;
	mixsample_t m_surroundLOfsVol = 0, m_surroundROfsVol = 0;

public:
	MixerSettings m_MixerSettings;
	CResampler m_Resampler;
#ifndef NO_REVERB
	CReverb m_Reverb;
#endif
#ifndef NO_DSP
	CSurround m_Surround;
	CMegaBass m_MegaBass;
#endif
#ifndef NO_EQ
	CQuadEQ m_EQ;
#endif
#ifndef NO_AGC
	CAGC m_AGC;
#endif
#ifndef NO_DSP
	BitCrush m_BitCrush;
#endif

	using samplecount_t = uint32; // Number of rendered samples

public:	// for Editing
#ifdef MODPLUG_TRACKER
	CModDoc *m_pModDoc = nullptr; // Can be a null pointer for example when previewing samples from the treeview.
#endif // MODPLUG_TRACKER
	Enum<MODTYPE> m_nType;
private:
	MODCONTAINERTYPE m_ContainerType = MOD_CONTAINERTYPE_NONE;
public:
	CHANNELINDEX m_nChannels = 0;
	SAMPLEINDEX m_nSamples = 0;
	INSTRUMENTINDEX m_nInstruments = 0;
	uint32 m_nDefaultSpeed, m_nDefaultGlobalVolume;
	TEMPO m_nDefaultTempo;
	FlagSet<SongFlags> m_SongFlags;
	CHANNELINDEX m_nMixChannels = 0;
private:
	CHANNELINDEX m_nMixStat;
public:
	ROWINDEX m_nDefaultRowsPerBeat, m_nDefaultRowsPerMeasure;	// default rows per beat and measure for this module
	TempoMode m_nTempoMode = tempoModeClassic;

#ifdef MODPLUG_TRACKER
	// Lock playback between two rows. Lock is active if lock start != ROWINDEX_INVALID).
	ROWINDEX m_lockRowStart = ROWINDEX_INVALID, m_lockRowEnd = ROWINDEX_INVALID;
	// Lock playback between two orders. Lock is active if lock start != ORDERINDEX_INVALID).
	ORDERINDEX m_lockOrderStart = ORDERINDEX_INVALID, m_lockOrderEnd = ORDERINDEX_INVALID;
#endif // MODPLUG_TRACKER

	uint32 m_nSamplePreAmp, m_nVSTiVolume;
	uint32 m_OPLVolumeFactor;  // 16.16
	static constexpr uint32 m_OPLVolumeFactorScale = 1 << 16;

	bool IsGlobalVolumeUnset() const { return IsFirstTick(); }
#ifndef MODPLUG_TRACKER
	uint32 m_nFreqFactor = 65536; // Pitch shift factor (65536 = no pitch shifting). Only used in libopenmpt (openmpt::ext::interactive::set_pitch_factor)
	uint32 m_nTempoFactor = 65536; // Tempo factor (65536 = no tempo adjustment). Only used in libopenmpt (openmpt::ext::interactive::set_tempo_factor)
#endif

	// Row swing factors for modern tempo mode
	TempoSwing m_tempoSwing;

	// Min Period = highest possible frequency, Max Period = lowest possible frequency for current format
	// Note: Period is an Amiga metric that is inverse to frequency.
	// Periods in MPT are 4 times as fine as Amiga periods because of extra fine frequency slides (introduced in the S3M format).
	int32 m_nMinPeriod, m_nMaxPeriod;

	ResamplingMode m_nResampling; // Resampling mode (if overriding the globally set resampling)
	int32 m_nRepeatCount = 0;     // -1 means repeat infinitely.
	ORDERINDEX m_nMaxOrderPosition;
	ModChannelSettings ChnSettings[MAX_BASECHANNELS];	// Initial channels settings
	CPatternContainer Patterns;
	ModSequenceSet Order;								// Pattern sequences (order lists)
protected:
	ModSample Samples[MAX_SAMPLES];						// Sample Headers
public:
	ModInstrument *Instruments[MAX_INSTRUMENTS];		// Instrument Headers
	MIDIMacroConfig m_MidiCfg;							// MIDI Macro config table
#ifndef NO_PLUGINS
	SNDMIXPLUGIN m_MixPlugins[MAX_MIXPLUGINS];			// Mix plugins
#endif
	mpt::charbuf<MAX_SAMPLENAME> m_szNames[MAX_SAMPLES];  // Sample names

	Version m_dwCreatedWithVersion;
	Version m_dwLastSavedWithVersion;

	PlayBehaviourSet m_playBehaviour;

protected:

	mpt::fast_prng m_PRNG;
	inline mpt::fast_prng & AccessPRNG() const { return const_cast<CSoundFile*>(this)->m_PRNG; }
	inline mpt::fast_prng & AccessPRNG() { return m_PRNG; }

protected:
	// Mix level stuff
	CSoundFilePlayConfig m_PlayConfig;
	MixLevels m_nMixLevels;

public:
	struct PlayState
	{
		friend class CSoundFile;

	public:
		samplecount_t m_lTotalSampleCount = 0;  // Total number of rendered samples
	protected:
		samplecount_t m_nBufferCount = 0;  // Remaining number samples to render for this tick
		double m_dBufferDiff = 0.0;        // Modern tempo rounding error compensation

	public:
		uint32 m_nTickCount = 0;  // Current tick being processed
	protected:
		uint32 m_nPatternDelay = 0;  // Pattern delay (rows)
		uint32 m_nFrameDelay = 0;    // Fine pattern delay (ticks)
	public:
		uint32 m_nSamplesPerTick = 0;
		ROWINDEX m_nCurrentRowsPerBeat = 0;     // Current time signature
		ROWINDEX m_nCurrentRowsPerMeasure = 0;  // Current time signature
		uint32 m_nMusicSpeed = 0;               // Current speed
		TEMPO m_nMusicTempo;                    // Current tempo

		// Playback position
		ROWINDEX m_nRow = 0;      // Current row being processed
		ROWINDEX m_nNextRow = 0;  // Next row to process
	protected:
		ROWINDEX m_nNextPatStartRow = 0;  // For FT2's E60 bug
	public:
		PATTERNINDEX m_nPattern = 0;                     // Current pattern being processed
		ORDERINDEX m_nCurrentOrder = 0;                  // Current order being processed
		ORDERINDEX m_nNextOrder = 0;                     // Next order to process
		ORDERINDEX m_nSeqOverride = ORDERINDEX_INVALID;  // Queued order to be processed next, regardless of what order would normally follow

		// Global volume
	public:
		int32 m_nGlobalVolume = MAX_GLOBAL_VOLUME;  // Current global volume (0...MAX_GLOBAL_VOLUME)
	protected:
		int32 m_nSamplesToGlobalVolRampDest = 0, m_nGlobalVolumeRampAmount = 0,
		      m_nGlobalVolumeDestination = 0;     // Global volume ramping
		int32 m_lHighResRampingGlobalVolume = 0;  // Global volume ramping

	public:
		bool m_bPositionChanged = true; // Report to plugins that we jumped around in the module

	public:
		CHANNELINDEX ChnMix[MAX_CHANNELS]; // Index of channels in Chn to be actually mixed
		ModChannel Chn[MAX_CHANNELS];      // Mixing channels... First m_nChannels channels are master channels (i.e. they are never NNA channels)!

	public:
		PlayState()
		{
			std::fill(std::begin(Chn), std::end(Chn), ModChannel());
		}

		void ResetGlobalVolumeRamping()
		{
			m_lHighResRampingGlobalVolume = m_nGlobalVolume << VOLUMERAMPPRECISION;
			m_nGlobalVolumeDestination = m_nGlobalVolume;
			m_nSamplesToGlobalVolRampDest = 0;
			m_nGlobalVolumeRampAmount = 0;
		}
	};

	PlayState m_PlayState;

protected:
	// For handling backwards jumps and stuff to prevent infinite loops when counting the mod length or rendering to wav.
	RowVisitor visitedSongRows;

public:
#ifdef MODPLUG_TRACKER
	std::bitset<MAX_BASECHANNELS> m_bChannelMuteTogglePending;

	std::vector<PatternCuePoint> *m_PatternCuePoints = nullptr;	// For WAV export (writing pattern positions to file)
	std::vector<SmpLength> *m_SamplePlayLengths = nullptr;	// For storing the maximum play length of each sample for automatic sample trimming
#endif // MODPLUG_TRACKER

	std::unique_ptr<OPL> m_opl;

public:
#ifdef LIBOPENMPT_BUILD
#ifndef NO_PLUGINS
	std::unique_ptr<CVstPluginManager> m_PluginManager;
#endif
#endif

public:
	std::string m_songName;
	mpt::ustring m_songArtist;
	SongMessage m_songMessage;
	ModFormatDetails m_modFormat;

protected:
	std::vector<FileHistory> m_FileHistory;	// File edit history
public:
	std::vector<FileHistory> &GetFileHistory() { return m_FileHistory; }
	const std::vector<FileHistory> &GetFileHistory() const { return m_FileHistory; }

#ifdef MPT_EXTERNAL_SAMPLES
	// MPTM external on-disk sample paths
protected:
	std::vector<mpt::PathString> m_samplePaths;

public:
	void SetSamplePath(SAMPLEINDEX smp, const mpt::PathString &filename) { if(m_samplePaths.size() < smp) m_samplePaths.resize(smp); m_samplePaths[smp - 1] = filename.Simplify(); }
	void ResetSamplePath(SAMPLEINDEX smp) { if(m_samplePaths.size() >= smp) m_samplePaths[smp - 1] = mpt::PathString(); Samples[smp].uFlags.reset(SMP_KEEPONDISK | SMP_MODIFIED);}
	mpt::PathString GetSamplePath(SAMPLEINDEX smp) const { if(m_samplePaths.size() >= smp) return m_samplePaths[smp - 1]; else return mpt::PathString(); }
	bool SampleHasPath(SAMPLEINDEX smp) const { if(m_samplePaths.size() >= smp) return !m_samplePaths[smp - 1].empty(); else return false; }
	bool IsExternalSampleMissing(SAMPLEINDEX smp) const { return Samples[smp].uFlags[SMP_KEEPONDISK] && !Samples[smp].HasSampleData(); }

	bool LoadExternalSample(SAMPLEINDEX smp, const mpt::PathString &filename);
#endif // MPT_EXTERNAL_SAMPLES

	bool m_bIsRendering = false;
	TimingInfo m_TimingInfo; // only valid if !m_bIsRendering

private:
	// logging
	ILog *m_pCustomLog = nullptr;

public:
	CSoundFile();
	CSoundFile(const CSoundFile &) = delete;
	CSoundFile & operator=(const CSoundFile &) = delete;
	~CSoundFile();

public:
	// logging
	void SetCustomLog(ILog *pLog) { m_pCustomLog = pLog; }
	void AddToLog(LogLevel level, const mpt::ustring &text) const;
	/*[[deprecated]]*/ void AddToLog(const AnyStringLocale &text) const { AddToLog(LogInformation, mpt::ToUnicode(text)); }

public:

	enum ModLoadingFlags
	{
		onlyVerifyHeader   = 0x00,
		loadPatternData    = 0x01, // If unset, advise loaders to not process any pattern data (if possible)
		loadSampleData     = 0x02, // If unset, advise loaders to not process any sample data (if possible)
		loadPluginData     = 0x04, // If unset, plugin data is not loaded (and as a consequence, plugins are not instanciated).
		loadPluginInstance = 0x08, // If unset, plugins are not instanciated.
		skipContainer      = 0x10,
		skipModules        = 0x20,

		// Shortcuts
		loadCompleteModule = loadSampleData | loadPatternData | loadPluginData | loadPluginInstance,
		loadNoPatternOrPluginData	= loadSampleData,
		loadNoPluginInstance = loadSampleData | loadPatternData | loadPluginData,
	};

	#define PROBE_RECOMMENDED_SIZE 2048u

	static constexpr std::size_t ProbeRecommendedSize = PROBE_RECOMMENDED_SIZE;

	enum ProbeFlags
	{
		ProbeModules    = 0x1,
		ProbeContainers = 0x2,

		ProbeFlagsDefault = ProbeModules | ProbeContainers,
		ProbeFlagsNone = 0
	};

	enum ProbeResult
	{
		ProbeSuccess      =  1,
		ProbeFailure      =  0,
		ProbeWantMoreData = -1
	};

	static ProbeResult ProbeAdditionalSize(MemoryFileReader &file, const uint64 *pfilesize, uint64 minimumAdditionalSize);

	static ProbeResult Probe(ProbeFlags flags, mpt::span<const std::byte> data, const uint64 *pfilesize);

public:

#ifdef MODPLUG_TRACKER
	// Get parent CModDoc. Can be nullptr if previewing from tree view, and is always nullptr if we're not actually compiling OpenMPT.
	CModDoc *GetpModDoc() const noexcept { return m_pModDoc; }

	bool Create(FileReader file, ModLoadingFlags loadFlags = loadCompleteModule, CModDoc *pModDoc = nullptr);
#else
	bool Create(FileReader file, ModLoadingFlags loadFlags);
#endif // MODPLUG_TRACKER

	bool Destroy();
	Enum<MODTYPE> GetType() const noexcept { return m_nType; }

	MODCONTAINERTYPE GetContainerType() const noexcept { return m_ContainerType; }

	// rough heuristic, could be improved
	mpt::Charset GetCharsetFile() const // 8bit string encoding of strings in the on-disk file
	{
		return m_modFormat.charset;
	}
	mpt::Charset GetCharsetInternal() const // 8bit string encoding of strings internal in CSoundFile
	{
		#if defined(MODPLUG_TRACKER)
			return mpt::Charset::Locale;
		#else // MODPLUG_TRACKER
			return GetCharsetFile();
		#endif // MODPLUG_TRACKER
	}

	ModMessageHeuristicOrder GetMessageHeuristic() const;

	void SetPreAmp(uint32 vol);
	uint32 GetPreAmp() const { return m_MixerSettings.m_nPreAmp; }

	void SetMixLevels(MixLevels levels);
	MixLevels GetMixLevels() const { return m_nMixLevels; }
	const CSoundFilePlayConfig &GetPlayConfig() const { return m_PlayConfig; }

	INSTRUMENTINDEX GetNumInstruments() const { return m_nInstruments; }
	SAMPLEINDEX GetNumSamples() const { return m_nSamples; }
	PATTERNINDEX GetCurrentPattern() const { return m_PlayState.m_nPattern; }
	ORDERINDEX GetCurrentOrder() const { return m_PlayState.m_nCurrentOrder; }
	CHANNELINDEX GetNumChannels() const { return m_nChannels; }

#ifndef NO_PLUGINS
	IMixPlugin* GetInstrumentPlugin(INSTRUMENTINDEX instr) const;
#endif
	const CModSpecifications& GetModSpecifications() const {return *m_pModSpecs;}
	static const CModSpecifications& GetModSpecifications(const MODTYPE type);

#ifdef MODPLUG_TRACKER
	void PatternTranstionChnSolo(const CHANNELINDEX chnIndex);
	void PatternTransitionChnUnmuteAll();

protected:
	void HandlePatternTransitionEvents();
#endif  // MODPLUG_TRACKER

public:
	double GetCurrentBPM() const;
	void DontLoopPattern(PATTERNINDEX nPat, ROWINDEX nRow = 0);
	CHANNELINDEX GetMixStat() const { return m_nMixStat; }
	void ResetMixStat() { m_nMixStat = 0; }
	void ResetPlayPos();
	void SetCurrentOrder(ORDERINDEX nOrder);
	std::string GetTitle() const { return m_songName; }
	bool SetTitle(const std::string &newTitle); // Return true if title was changed.
	const char *GetSampleName(SAMPLEINDEX nSample) const;
	const char *GetInstrumentName(INSTRUMENTINDEX nInstr) const;
	uint32 GetMusicSpeed() const { return m_PlayState.m_nMusicSpeed; }
	TEMPO GetMusicTempo() const { return m_PlayState.m_nMusicTempo; }
	bool IsFirstTick() const { return (m_PlayState.m_lTotalSampleCount == 0); }

	// Get song duration in various cases: total length, length to specific order & row, etc.
	std::vector<GetLengthType> GetLength(enmGetLengthResetMode adjustMode, GetLengthTarget target = GetLengthTarget());

public:
	void RecalculateSamplesPerTick();
	double GetRowDuration(TEMPO tempo, uint32 speed) const;
	uint32 GetTickDuration(PlayState &playState) const;

	// A repeat count value of -1 means infinite loop
	void SetRepeatCount(int n) { m_nRepeatCount = n; }
	int GetRepeatCount() const { return m_nRepeatCount; }
	bool IsPaused() const { return m_SongFlags[SONG_PAUSED | SONG_STEP]; }	// Added SONG_STEP as it seems to be desirable in most cases to check for this as well.
	void LoopPattern(PATTERNINDEX nPat, ROWINDEX nRow = 0);

	bool InitChannel(CHANNELINDEX nChn);
	void InitAmigaResampler();

	void InitOPL();
	static constexpr bool SupportsOPL(MODTYPE type) { return type & (MOD_TYPE_S3M | MOD_TYPE_MPT); }
	bool SupportsOPL() const noexcept { return SupportsOPL(m_nType); }

	static ProbeResult ProbeFileHeaderMMCMP(MemoryFileReader file, const uint64 *pfilesize);
	static ProbeResult ProbeFileHeaderPP20(MemoryFileReader file, const uint64 *pfilesize);
	static ProbeResult ProbeFileHeaderUMX(MemoryFileReader file, const uint64 *pfilesize);
	static ProbeResult ProbeFileHeaderXPK(MemoryFileReader file, const uint64 *pfilesize);

	static ProbeResult ProbeFileHeader669(MemoryFileReader file, const uint64 *pfilesize);
	static ProbeResult ProbeFileHeaderAM(MemoryFileReader file, const uint64 *pfilesize);
	static ProbeResult ProbeFileHeaderAMF_Asylum(MemoryFileReader file, const uint64 *pfilesize);
	static ProbeResult ProbeFileHeaderAMF_DSMI(MemoryFileReader file, const uint64 *pfilesize);
	static ProbeResult ProbeFileHeaderAMS(MemoryFileReader file, const uint64 *pfilesize);
	static ProbeResult ProbeFileHeaderAMS2(MemoryFileReader file, const uint64 *pfilesize);
	static ProbeResult ProbeFileHeaderC67(MemoryFileReader file, const uint64 *pfilesize);
	static ProbeResult ProbeFileHeaderDBM(MemoryFileReader file, const uint64 *pfilesize);
	static ProbeResult ProbeFileHeaderDTM(MemoryFileReader file, const uint64 *pfilesize);
	static ProbeResult ProbeFileHeaderDIGI(MemoryFileReader file, const uint64 *pfilesize);
	static ProbeResult ProbeFileHeaderDMF(MemoryFileReader file, const uint64 *pfilesize);
	static ProbeResult ProbeFileHeaderDSM(MemoryFileReader file, const uint64 *pfilesize);
	static ProbeResult ProbeFileHeaderFAR(MemoryFileReader file, const uint64 *pfilesize);
	static ProbeResult ProbeFileHeaderGDM(MemoryFileReader file, const uint64 *pfilesize);
	static ProbeResult ProbeFileHeaderICE(MemoryFileReader file, const uint64 *pfilesize);
	static ProbeResult ProbeFileHeaderIMF(MemoryFileReader file, const uint64 *pfilesize);
	static ProbeResult ProbeFileHeaderIT(MemoryFileReader file, const uint64 *pfilesize);
	static ProbeResult ProbeFileHeaderITP(MemoryFileReader file, const uint64 *pfilesize);
	static ProbeResult ProbeFileHeaderJ2B(MemoryFileReader file, const uint64 *pfilesize);
	static ProbeResult ProbeFileHeaderM15(MemoryFileReader file, const uint64 *pfilesize);
	static ProbeResult ProbeFileHeaderMDL(MemoryFileReader file, const uint64 *pfilesize);
	static ProbeResult ProbeFileHeaderMED(MemoryFileReader file, const uint64 *pfilesize);
	static ProbeResult ProbeFileHeaderMO3(MemoryFileReader file, const uint64 *pfilesize);
	static ProbeResult ProbeFileHeaderMOD(MemoryFileReader file, const uint64 *pfilesize);
	static ProbeResult ProbeFileHeaderMT2(MemoryFileReader file, const uint64 *pfilesize);
	static ProbeResult ProbeFileHeaderMTM(MemoryFileReader file, const uint64 *pfilesize);
	static ProbeResult ProbeFileHeaderOKT(MemoryFileReader file, const uint64 *pfilesize);
	static ProbeResult ProbeFileHeaderPLM(MemoryFileReader file, const uint64 *pfilesize);
	static ProbeResult ProbeFileHeaderPSM(MemoryFileReader file, const uint64 *pfilesize);
	static ProbeResult ProbeFileHeaderPSM16(MemoryFileReader file, const uint64 *pfilesize);
	static ProbeResult ProbeFileHeaderPT36(MemoryFileReader file, const uint64 *pfilesize);
	static ProbeResult ProbeFileHeaderPTM(MemoryFileReader file, const uint64 *pfilesize);
	static ProbeResult ProbeFileHeaderS3M(MemoryFileReader file, const uint64 *pfilesize);
	static ProbeResult ProbeFileHeaderSFX(MemoryFileReader file, const uint64 *pfilesize);
	static ProbeResult ProbeFileHeaderSTM(MemoryFileReader file, const uint64 *pfilesize);
	static ProbeResult ProbeFileHeaderSTP(MemoryFileReader file, const uint64 *pfilesize);
	static ProbeResult ProbeFileHeaderULT(MemoryFileReader file, const uint64 *pfilesize);
	static ProbeResult ProbeFileHeaderXM(MemoryFileReader file, const uint64 *pfilesize);

	static ProbeResult ProbeFileHeaderMID(MemoryFileReader file, const uint64 *pfilesize);
	static ProbeResult ProbeFileHeaderUAX(MemoryFileReader file, const uint64 *pfilesize);
	static ProbeResult ProbeFileHeaderWAV(MemoryFileReader file, const uint64 *pfilesize);

	// Module Loaders
	bool Read669(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadAM(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadAMF_Asylum(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadAMF_DSMI(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadAMS(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadAMS2(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadC67(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadDBM(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadDTM(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadDIGI(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadDMF(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadDSM(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadFAR(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadGDM(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadICE(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadIMF(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadIT(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadITP(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadJ2B(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadM15(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadMDL(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadMED(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadMO3(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadMOD(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadMT2(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadMTM(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadOKT(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadPLM(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadPSM(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadPSM16(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadPT36(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadPTM(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadS3M(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadSFX(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadSTM(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadSTP(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadULT(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadXM(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);

	bool ReadMID(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadUAX(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadWAV(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);

	static std::vector<const char *> GetSupportedExtensions(bool otherFormats);
	static bool IsExtensionSupported(std::string_view ext); // UTF8, casing of ext is ignored
	static mpt::ustring ModContainerTypeToString(MODCONTAINERTYPE containertype);
	static mpt::ustring ModContainerTypeToTracker(MODCONTAINERTYPE containertype);

	// Repair non-standard stuff in modules saved with previous ModPlug versions
	void UpgradeModule();

	// Save Functions
#ifndef MODPLUG_NO_FILESAVE
	bool SaveXM(std::ostream &f, bool compatibilityExport = false);
	bool SaveS3M(std::ostream &f) const;
	bool SaveMod(std::ostream &f) const;
	bool SaveIT(std::ostream &f, const mpt::PathString &filename, bool compatibilityExport = false);
	uint32 SaveMixPlugins(std::ostream *file=nullptr, bool bUpdate=true);
	void WriteInstrumentPropertyForAllInstruments(uint32 code,  uint16 size, std::ostream &f, INSTRUMENTINDEX nInstruments) const;
	void SaveExtendedInstrumentProperties(INSTRUMENTINDEX nInstruments, std::ostream &f) const;
	void SaveExtendedSongProperties(std::ostream &f) const;
#endif // MODPLUG_NO_FILESAVE
	void LoadExtendedSongProperties(FileReader &file, bool ignoreChannelCount, bool* pInterpretMptMade = nullptr);
	void LoadMPTMProperties(FileReader &file, uint16 cwtv);

	static mpt::ustring GetSchismTrackerVersion(uint16 cwtv, uint32 reserved);

	// Reads extended instrument properties(XM/IT/MPTM).
	// Returns true if extended instrument properties were found.
	bool LoadExtendedInstrumentProperties(FileReader &file);

	void SetDefaultPlaybackBehaviour(MODTYPE type);
	static PlayBehaviourSet GetSupportedPlaybackBehaviour(MODTYPE type);
	static PlayBehaviourSet GetDefaultPlaybackBehaviour(MODTYPE type);

	// MOD Convert function
	MODTYPE GetBestSaveFormat() const;
	static void ConvertModCommand(ModCommand &m);
	static void S3MConvert(ModCommand &m, bool fromIT);
	void S3MSaveConvert(uint8 &command, uint8 &param, bool toIT, bool compatibilityExport = false) const;
	void ModSaveCommand(uint8 &command, uint8 &param, const bool toXM, const bool compatibilityExport = false) const;
	static void ReadMODPatternEntry(FileReader &file, ModCommand &m);
	static void ReadMODPatternEntry(const std::array<uint8, 4> data, ModCommand &m);

	void SetupMODPanning(bool bForceSetup = false); // Setup LRRL panning, max channel volume

public:
	// Real-time sound functions
	void SuspendPlugins();
	void ResumePlugins();
	void StopAllVsti();
	void RecalculateGainForAllPlugs();
	void ResetChannels();
	samplecount_t Read(samplecount_t count, IAudioReadTarget &target) { AudioSourceNone source; return Read(count, target, source); }
	samplecount_t Read(samplecount_t count, IAudioReadTarget &target, IAudioSource &source);
private:
	void CreateStereoMix(int count);
public:
	bool FadeSong(uint32 msec);
private:
	void ProcessDSP(uint32 countChunk);
	void ProcessPlugins(uint32 nCount);
	void ProcessInputChannels(IAudioSource &source, std::size_t countChunk);
public:
	samplecount_t GetTotalSampleCount() const { return m_PlayState.m_lTotalSampleCount; }
	bool HasPositionChanged() { bool b = m_PlayState.m_bPositionChanged; m_PlayState.m_bPositionChanged = false; return b; }
	bool IsRenderingToDisc() const { return m_bIsRendering; }

	void PrecomputeSampleLoops(bool updateChannels = false);

public:
	// Mixer Config
	void SetMixerSettings(const MixerSettings &mixersettings);
	void SetResamplerSettings(const CResamplerSettings &resamplersettings);
	void InitPlayer(bool bReset=false);
	void SetDspEffects(uint32 DSPMask);
	uint32 GetSampleRate() const { return m_MixerSettings.gdwMixingFreq; }
#ifndef NO_EQ
	void SetEQGains(const uint32 *pGains, uint32 nBands, const uint32 *pFreqs=NULL, bool bReset=false)	{ m_EQ.SetEQGains(pGains, nBands, pFreqs, bReset, m_MixerSettings.gdwMixingFreq); } // 0=-12dB, 32=+12dB
#endif // NO_EQ
public:
	bool ReadNote();
	bool ProcessRow();
	bool ProcessEffects();
	CHANNELINDEX GetNNAChannel(CHANNELINDEX nChn) const;
	CHANNELINDEX CheckNNA(CHANNELINDEX nChn, uint32 instr, int note, bool forceCut);
	void NoteChange(ModChannel &chn, int note, bool bPorta = false, bool bResetEnv = true, bool bManual = false, CHANNELINDEX channelHint = CHANNELINDEX_INVALID) const;
	void InstrumentChange(ModChannel &chn, uint32 instr, bool bPorta = false, bool bUpdVol = true, bool bResetEnv = true) const;
	void ApplyInstrumentPanning(ModChannel &chn, const ModInstrument *instr, const ModSample *smp) const;
	uint32 CalculateXParam(PATTERNINDEX pat, ROWINDEX row, CHANNELINDEX chn, bool *isExtended = nullptr) const;

	// Channel Effects
	void KeyOff(ModChannel &chn) const;
	// Global Effects
	void SetTempo(TEMPO param, bool setAsNonModcommand = false);
	void SetSpeed(PlayState &playState, uint32 param) const;
	static TEMPO ConvertST2Tempo(uint8 tempo);

protected:
	// Global variable initializer for loader functions
	void SetType(MODTYPE type);
	void InitializeGlobals(MODTYPE type = MOD_TYPE_NONE);
	void InitializeChannels();

	// Channel effect processing
	int GetVibratoDelta(int type, int position) const;

	void ProcessVolumeSwing(ModChannel &chn, int &vol) const;
	void ProcessPanningSwing(ModChannel &chn) const;
	void ProcessTremolo(ModChannel &chn, int &vol) const;
	void ProcessTremor(CHANNELINDEX nChn, int &vol);

	bool IsEnvelopeProcessed(const ModChannel &chn, EnvelopeType env) const;
	void ProcessVolumeEnvelope(ModChannel &chn, int &vol) const;
	void ProcessPanningEnvelope(ModChannel &chn) const;
	int ProcessPitchFilterEnvelope(ModChannel &chn, int &period) const;

	void IncrementEnvelopePosition(ModChannel &chn, EnvelopeType envType) const;
	void IncrementEnvelopePositions(ModChannel &chn) const;

	void ProcessInstrumentFade(ModChannel &chn, int &vol) const;

	void ProcessPitchPanSeparation(ModChannel &chn) const;
	void ProcessPanbrello(ModChannel &chn) const;

	void ProcessArpeggio(CHANNELINDEX nChn, int &period, Tuning::NOTEINDEXTYPE &arpeggioSteps);
	void ProcessVibrato(CHANNELINDEX nChn, int &period, Tuning::RATIOTYPE &vibratoFactor);
	void ProcessSampleAutoVibrato(ModChannel &chn, int &period, Tuning::RATIOTYPE &vibratoFactor, int &nPeriodFrac) const;

	void ProcessRamping(ModChannel &chn) const;

	SamplePosition GetChannelIncrement(const ModChannel &chn, uint32 period, int periodFrac) const;

protected:
	// Type of panning command
	enum PanningType
	{
		Pan4bit = 4,
		Pan6bit = 6,
		Pan8bit = 8,
	};
	// Channel Effects
	void UpdateS3MEffectMemory(ModChannel &chn, ModCommand::PARAM param) const;
	void PortamentoUp(CHANNELINDEX nChn, ModCommand::PARAM param, const bool doFinePortamentoAsRegular = false);
	void PortamentoDown(CHANNELINDEX nChn, ModCommand::PARAM param, const bool doFinePortamentoAsRegular = false);
	void MidiPortamento(CHANNELINDEX nChn, int param, bool doFineSlides);
	void FinePortamentoUp(ModChannel &chn, ModCommand::PARAM param) const;
	void FinePortamentoDown(ModChannel &chn, ModCommand::PARAM param) const;
	void ExtraFinePortamentoUp(ModChannel &chn, ModCommand::PARAM param) const;
	void ExtraFinePortamentoDown(ModChannel &chn, ModCommand::PARAM param) const;
	void PortamentoMPT(ModChannel &chn, int);
	void PortamentoFineMPT(ModChannel &chn, int);
	void PortamentoExtraFineMPT(ModChannel &chn, int);
	void NoteSlide(ModChannel &chn, uint32 param, bool slideUp, bool retrig) const;
	std::pair<uint32, bool> GetVolCmdTonePorta(const ModCommand &m, uint32 startTick) const;
	void TonePortamento(ModChannel &chn, uint32 param) const;
	void Vibrato(ModChannel &chn, uint32 param) const;
	void FineVibrato(ModChannel &chn, uint32 param) const;
	void VolumeSlide(ModChannel &chn, ModCommand::PARAM param);
	void PanningSlide(ModChannel &chn, ModCommand::PARAM param, bool memory = true);
	void ChannelVolSlide(ModChannel &chn, ModCommand::PARAM param) const;
	void FineVolumeUp(ModChannel &chn, ModCommand::PARAM param, bool volCol) const;
	void FineVolumeDown(ModChannel &chn, ModCommand::PARAM param, bool volCol) const;
	void Tremolo(ModChannel &chn, uint32 param) const;
	void Panbrello(ModChannel &chn, uint32 param) const;
	void Panning(ModChannel &chn, uint32 param, PanningType panBits) const;
	void RetrigNote(CHANNELINDEX nChn, int param, int offset = 0);
	void SampleOffset(ModChannel &chn, SmpLength param) const;
	void ReverseSampleOffset(ModChannel &chn, ModCommand::PARAM param) const;
	void NoteCut(CHANNELINDEX nChn, uint32 nTick, bool cutSample);
	ROWINDEX PatternLoop(ModChannel &chn, uint32 param);
	void ExtendedMODCommands(CHANNELINDEX nChn, ModCommand::PARAM param);
	void ExtendedS3MCommands(CHANNELINDEX nChn, ModCommand::PARAM param);
	void ExtendedChannelEffect(ModChannel &chn, uint32 param);
	void InvertLoop(ModChannel &chn);
	ROWINDEX PatternBreak(PlayState &state, CHANNELINDEX chn, uint8 param) const;
	void GlobalVolSlide(ModCommand::PARAM param, uint8 &nOldGlobalVolSlide);

	void ProcessMacroOnChannel(CHANNELINDEX nChn);
	void ProcessMIDIMacro(CHANNELINDEX nChn, bool isSmooth, const char *macro, uint8 param = 0, PLUGINDEX plugin = 0);
	float CalculateSmoothParamChange(float currentValue, float param) const;
	uint32 SendMIDIData(CHANNELINDEX nChn, bool isSmooth, const unsigned char *macro, uint32 macroLen, PLUGINDEX plugin);
	void SendMIDINote(CHANNELINDEX chn, uint16 note, uint16 volume);

	int SetupChannelFilter(ModChannel &chn, bool bReset, int envModifier = 256) const;

	// Low-Level effect processing
	void DoFreqSlide(ModChannel &chn, int32 nFreqSlide) const;
	void UpdateTimeSignature();

public:
	// Convert frequency to IT cutoff (0...127)
	uint8 FrequencyToCutOff(double frequency) const;
	// Convert IT cutoff (0...127 + modifier) to frequency
	uint32 CutOffToFrequency(uint32 nCutOff, int envModifier = 256) const; // [0-127] => [1-10KHz]

	// Returns true if periods are actually plain frequency values in Hz.
	bool PeriodsAreFrequencies() const
	{
		return m_SongFlags[SONG_LINEARSLIDES] && m_playBehaviour[kHertzInLinearMode] && GetType() != MOD_TYPE_XM;
	}
	
	// Returns true if the current format uses transpose+finetune rather than frequency in Hz to specify middle-C.
	static constexpr bool UseFinetuneAndTranspose(MODTYPE type)
	{
		return (type & (MOD_TYPE_AMF0 | MOD_TYPE_DIGI | MOD_TYPE_MED | MOD_TYPE_MOD | MOD_TYPE_MTM | MOD_TYPE_OKT | MOD_TYPE_SFX | MOD_TYPE_STP | MOD_TYPE_XM));
	}
	bool UseFinetuneAndTranspose() const
	{
		return UseFinetuneAndTranspose(GetType());
	}

public:
	uint32 GetNumTicksOnCurrentRow() const
	{
		return (m_PlayState.m_nMusicSpeed  + m_PlayState.m_nFrameDelay) * std::max(m_PlayState.m_nPatternDelay, static_cast<uint32>(1));
	}

	bool DestroySample(SAMPLEINDEX nSample);
	bool DestroySampleThreadsafe(SAMPLEINDEX nSample);

	// Find an unused sample slot. If it is going to be assigned to an instrument, targetInstrument should be specified.
	// SAMPLEINDEX_INVLAID is returned if no free sample slot could be found.
	SAMPLEINDEX GetNextFreeSample(INSTRUMENTINDEX targetInstrument = INSTRUMENTINDEX_INVALID, SAMPLEINDEX start = 1) const;
	// Find an unused instrument slot.
	// INSTRUMENTINDEX_INVALID is returned if no free instrument slot could be found.
	INSTRUMENTINDEX GetNextFreeInstrument(INSTRUMENTINDEX start = 1) const;
	// Check whether a given sample is used by a given instrument.
	bool IsSampleReferencedByInstrument(SAMPLEINDEX sample, INSTRUMENTINDEX instr) const;

	ModInstrument *AllocateInstrument(INSTRUMENTINDEX instr, SAMPLEINDEX assignedSample = 0);
	bool DestroyInstrument(INSTRUMENTINDEX nInstr, deleteInstrumentSamples removeSamples);
	bool RemoveInstrumentSamples(INSTRUMENTINDEX nInstr, SAMPLEINDEX keepSample = SAMPLEINDEX_INVALID);
	SAMPLEINDEX DetectUnusedSamples(std::vector<bool> &sampleUsed) const;
	SAMPLEINDEX RemoveSelectedSamples(const std::vector<bool> &keepSamples);

	// Set the autovibrato settings for all samples associated to the given instrument.
	void PropagateXMAutoVibrato(INSTRUMENTINDEX ins, VibratoType type, uint8 sweep, uint8 depth, uint8 rate);

	// Samples file I/O
	bool ReadSampleFromFile(SAMPLEINDEX nSample, FileReader &file, bool mayNormalize = false, bool includeInstrumentFormats = true);
	bool ReadWAVSample(SAMPLEINDEX nSample, FileReader &file, bool mayNormalize = false, FileReader *wsmpChunk = nullptr);
protected:
	bool ReadW64Sample(SAMPLEINDEX nSample, FileReader &file, bool mayNormalize = false);
	bool ReadPATSample(SAMPLEINDEX nSample, FileReader &file);
	bool ReadS3ISample(SAMPLEINDEX nSample, FileReader &file);
	bool ReadSBISample(SAMPLEINDEX sample, FileReader &file);
	bool ReadCAFSample(SAMPLEINDEX nSample, FileReader &file, bool mayNormalize = false);
	bool ReadAIFFSample(SAMPLEINDEX nSample, FileReader &file, bool mayNormalize = false);
	bool ReadAUSample(SAMPLEINDEX nSample, FileReader &file, bool mayNormalize = false);
	bool ReadXISample(SAMPLEINDEX nSample, FileReader &file);
	bool ReadITSSample(SAMPLEINDEX nSample, FileReader &file, bool rewind = true);
	bool ReadITISample(SAMPLEINDEX nSample, FileReader &file);
	bool ReadIFFSample(SAMPLEINDEX sample, FileReader &file);
	bool ReadBRRSample(SAMPLEINDEX sample, FileReader& file);
	bool ReadFLACSample(SAMPLEINDEX sample, FileReader &file);
	bool ReadOpusSample(SAMPLEINDEX sample, FileReader &file);
	bool ReadVorbisSample(SAMPLEINDEX sample, FileReader &file);
	bool ReadMP3Sample(SAMPLEINDEX sample, FileReader &file, bool raw = false, bool mo3Decode = false);  //  raw: ignore all encoder-/decodr-delays, decode just raw frames  ;  mod3Decode: skip metadata and loop-precompute
	bool ReadMediaFoundationSample(SAMPLEINDEX sample, FileReader &file, bool mo3Decode = false);  //  mod3Decode: skip metadata and loop-precompute
public:
#ifdef MODPLUG_TRACKER
	static std::vector<FileType> GetMediaFoundationFileTypes();
#endif // MODPLUG_TRACKER
#ifndef MODPLUG_NO_FILESAVE
	bool SaveWAVSample(SAMPLEINDEX nSample, std::ostream &f) const;
	bool SaveRAWSample(SAMPLEINDEX nSample, std::ostream &f) const;
	bool SaveFLACSample(SAMPLEINDEX nSample, std::ostream &f) const;
	bool SaveS3ISample(SAMPLEINDEX smp, std::ostream &f) const;
#endif
	static bool CanReadMP3();
	static bool CanReadVorbis();
	static bool CanReadMediaFoundation();

	// Instrument file I/O
	bool ReadInstrumentFromFile(INSTRUMENTINDEX nInstr, FileReader &file, bool mayNormalize=false);
	bool ReadSampleAsInstrument(INSTRUMENTINDEX nInstr, FileReader &file, bool mayNormalize=false);
protected:
	bool ReadXIInstrument(INSTRUMENTINDEX nInstr, FileReader &file);
	bool ReadITIInstrument(INSTRUMENTINDEX nInstr, FileReader &file);
	bool ReadPATInstrument(INSTRUMENTINDEX nInstr, FileReader &file);
	bool ReadSFZInstrument(INSTRUMENTINDEX nInstr, FileReader &file);
public:
#ifndef MODPLUG_NO_FILESAVE
	bool SaveXIInstrument(INSTRUMENTINDEX nInstr, std::ostream &f) const;
	bool SaveITIInstrument(INSTRUMENTINDEX nInstr, std::ostream &f, const mpt::PathString &filename, bool compress, bool allowExternal) const;
	bool SaveSFZInstrument(INSTRUMENTINDEX nInstr, std::ostream &f, const mpt::PathString &filename, bool useFLACsamples) const;
#endif

	// I/O from another sound file
	bool ReadInstrumentFromSong(INSTRUMENTINDEX targetInstr, const CSoundFile &srcSong, INSTRUMENTINDEX sourceInstr);
	bool ReadSampleFromSong(SAMPLEINDEX targetSample, const CSoundFile &srcSong, SAMPLEINDEX sourceSample);

	// Period/Note functions
	uint32 GetNoteFromPeriod(uint32 period, int32 nFineTune = 0, uint32 nC5Speed = 0) const;
	uint32 GetPeriodFromNote(uint32 note, int32 nFineTune, uint32 nC5Speed) const;
	uint32 GetFreqFromPeriod(uint32 period, uint32 c5speed, int32 nPeriodFrac = 0) const;
	// Misc functions
	ModSample &GetSample(SAMPLEINDEX sample) { MPT_ASSERT(sample <= m_nSamples && sample < CountOf(Samples)); return Samples[sample]; }
	const ModSample &GetSample(SAMPLEINDEX sample) const { MPT_ASSERT(sample <= m_nSamples && sample < CountOf(Samples)); return Samples[sample]; }

	uint32 MapMidiInstrument(uint8 program, uint16 bank, uint8 midiChannel, uint8 note, bool isXG, std::bitset<16> drumChns);
	size_t ITInstrToMPT(FileReader &file, ModInstrument &ins, uint16 trkvers);
	bool LoadMixPlugins(FileReader &file);
#ifndef NO_PLUGINS
	static void ReadMixPluginChunk(FileReader &file, SNDMIXPLUGIN &plugin);
	void ProcessMidiOut(CHANNELINDEX nChn);
#endif // NO_PLUGINS

	void ProcessGlobalVolume(long countChunk);
	void ProcessStereoSeparation(long countChunk);

private:
	PLUGINDEX GetChannelPlugin(CHANNELINDEX nChn, PluginMutePriority respectMutes) const;
	PLUGINDEX GetActiveInstrumentPlugin(CHANNELINDEX, PluginMutePriority respectMutes) const;
	IMixPlugin *GetChannelInstrumentPlugin(CHANNELINDEX chn) const;

public:
	PLUGINDEX GetBestPlugin(CHANNELINDEX nChn, PluginPriority priority, PluginMutePriority respectMutes) const;
	uint8 GetBestMidiChannel(CHANNELINDEX nChn) const;

};


#ifndef NO_PLUGINS
inline IMixPlugin* CSoundFile::GetInstrumentPlugin(INSTRUMENTINDEX instr) const
{
	if(instr > 0 && instr <= GetNumInstruments() && Instruments[instr] && Instruments[instr]->nMixPlug && Instruments[instr]->nMixPlug <= MAX_MIXPLUGINS)
		return m_MixPlugins[Instruments[instr]->nMixPlug - 1].pMixPlugin;
	else
		return nullptr;
}
#endif // NO_PLUGINS


///////////////////////////////////////////////////////////
// Low-level Mixing functions

#define FADESONGDELAY		100

static MPT_CONSTEXPR11_FUN int8 MOD2XMFineTune(int v) { return static_cast<int8>(static_cast<uint8>(v) << 4); }
static MPT_CONSTEXPR11_FUN int8 XM2MODFineTune(int v) { return static_cast<int8>(static_cast<uint8>(v) >> 4); }

// Read instrument property with 'code' and 'size' from 'file' to instrument 'pIns'.
void ReadInstrumentExtensionField(ModInstrument* pIns, const uint32 code, const uint16 size, FileReader &file);

// Read instrument property with 'code' from 'file' to instrument 'pIns'.
void ReadExtendedInstrumentProperty(ModInstrument* pIns, const uint32 code, FileReader &file);

// Read extended instrument properties from 'file' to instrument 'pIns'.
void ReadExtendedInstrumentProperties(ModInstrument* pIns, FileReader &file);


OPENMPT_NAMESPACE_END
