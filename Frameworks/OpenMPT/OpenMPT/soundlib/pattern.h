/*
 * Pattern.h
 * ---------
 * Purpose: Module Pattern header class
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include <vector>
#include "modcommand.h"
#include "Snd_defs.h"


OPENMPT_NAMESPACE_BEGIN

class CPatternContainer;
class CSoundFile;
class EffectWriter;

class CPattern
{
	friend class CPatternContainer;
	
public:
	CPattern(CPatternContainer &patCont) : m_rPatternContainer{patCont} {}
	CPattern(const CPattern &) = default;
	CPattern(CPattern &&) noexcept = default;

	CPattern& operator= (const CPattern &pat);
	bool operator== (const CPattern &other) const noexcept;
	bool operator!= (const CPattern &other) const noexcept { return !(*this == other); }

public:
	ModCommand* GetpModCommand(const ROWINDEX r, const CHANNELINDEX c) { return &m_ModCommands[r * GetNumChannels() + c]; }
	const ModCommand* GetpModCommand(const ROWINDEX r, const CHANNELINDEX c) const { return &m_ModCommands[r * GetNumChannels() + c]; }
	
	ROWINDEX GetNumRows() const noexcept { return m_Rows; }
	ROWINDEX GetRowsPerBeat() const noexcept { return m_RowsPerBeat; }			// pattern-specific rows per beat
	ROWINDEX GetRowsPerMeasure() const noexcept { return m_RowsPerMeasure; }		// pattern-specific rows per measure
	bool GetOverrideSignature() const noexcept { return (m_RowsPerBeat + m_RowsPerMeasure > 0); }	// override song time signature?

	// Returns true if pattern data can be accessed at given row, false otherwise.
	bool IsValidRow(const ROWINDEX row) const noexcept { return (row < GetNumRows()); }
	// Returns true if any pattern data is present.
	bool IsValid() const noexcept { return !m_ModCommands.empty(); }

	mpt::span<ModCommand> GetRow(const ROWINDEX row) { return mpt::as_span(GetpModCommand(row, 0), GetNumChannels()); }
	mpt::span<const ModCommand> GetRow(const ROWINDEX row) const { return mpt::as_span(GetpModCommand(row, 0), GetNumChannels()); }

	CHANNELINDEX GetNumChannels() const noexcept;

	// Add or remove rows from the pattern.
	bool Resize(const ROWINDEX newRowCount, bool enforceFormatLimits = true, bool resizeAtEnd = true);

	// Check if there is any note data on a given row.
	bool IsEmptyRow(ROWINDEX row) const noexcept;

	// Allocate new pattern memory and replace old pattern data.
	bool AllocatePattern(ROWINDEX rows);
	// Deallocate pattern data.
	void Deallocate();

	// Removes all modcommands from the pattern.
	void ClearCommands() noexcept;

	// Returns associated soundfile.
	CSoundFile& GetSoundFile() noexcept;
	const CSoundFile& GetSoundFile() const noexcept;

	const std::vector<ModCommand> &GetData() const { return m_ModCommands; }
	void SetData(std::vector<ModCommand> &&data) { MPT_ASSERT(data.size() == GetNumRows() * GetNumChannels()); m_ModCommands = std::move(data); }

	// Set pattern signature (rows per beat, rows per measure). Returns true on success.
	bool SetSignature(const ROWINDEX rowsPerBeat, const ROWINDEX rowsPerMeasure) noexcept;
	void RemoveSignature() noexcept { m_RowsPerBeat = m_RowsPerMeasure = 0; }

	bool HasTempoSwing() const noexcept { return !m_tempoSwing.empty(); }
	const TempoSwing& GetTempoSwing() const noexcept { return m_tempoSwing; }
	void SetTempoSwing(const TempoSwing &swing) { m_tempoSwing = swing; m_tempoSwing.Normalize(); }
	void RemoveTempoSwing() noexcept { m_tempoSwing.clear(); }

	// Pattern name functions - bool functions return true on success.
	bool SetName(const std::string &newName);
	bool SetName(const char *newName, size_t maxChars);
	template<size_t bufferSize>
	bool SetName(const char (&buffer)[bufferSize])
	{
		return SetName(buffer, bufferSize);
	}

	std::string GetName() const { return m_PatternName; }

#ifdef MODPLUG_TRACKER
	// Double number of rows
	bool Expand();

	// Halve number of rows
	bool Shrink();
#endif // MODPLUG_TRACKER

	// Write some kind of effect data to the pattern
	bool WriteEffect(EffectWriter &settings);

	using iterator = std::vector<ModCommand>::iterator;
	using const_iterator = std::vector<ModCommand>::const_iterator;

	iterator begin() noexcept { return m_ModCommands.begin(); }
	const_iterator begin() const noexcept { return m_ModCommands.begin(); }
	const_iterator cbegin() const noexcept { return m_ModCommands.cbegin(); }

	iterator end() noexcept { return m_ModCommands.end(); }
	const_iterator end() const noexcept { return m_ModCommands.end(); }
	const_iterator cend() const noexcept { return m_ModCommands.cend(); }

protected:
	ModCommand& GetModCommand(size_t i) { return m_ModCommands[i]; }
	//Returns modcommand from (floor[i/channelCount], i%channelCount) 

	ModCommand& GetModCommand(ROWINDEX r, CHANNELINDEX c) { return m_ModCommands[r * GetNumChannels() + c]; }
	const ModCommand& GetModCommand(ROWINDEX r, CHANNELINDEX c) const { return m_ModCommands[r * GetNumChannels() + c]; }


protected:
	std::vector<ModCommand> m_ModCommands;
	ROWINDEX m_Rows = 0;
	ROWINDEX m_RowsPerBeat = 0;    // patterns-specific time signature. if != 0, this is implicitely set.
	ROWINDEX m_RowsPerMeasure = 0; // ditto
	TempoSwing m_tempoSwing;
	std::string m_PatternName;
	CPatternContainer& m_rPatternContainer;
};


const char FileIdPattern[] = "mptP";

void ReadModPattern(std::istream& iStrm, CPattern& patc, const size_t nSize = 0);
void WriteModPattern(std::ostream& oStrm, const CPattern& patc);


// Class for conveniently writing an effect to the pattern.

class EffectWriter
{
	friend class CPattern;
	
	// Row advance mode
	enum RetryMode
	{
		rmIgnore,			// If effect can't be written, abort.
		rmTryNextRow,		// If effect can't be written, try next row.
		rmTryPreviousRow,	// If effect can't be written, try previous row.
	};

public:
	// Constructors with effect commands
	EffectWriter(EffectCommand cmd, ModCommand::PARAM param) : m_command(cmd), m_param(param), m_isVolEffect(false) { Init(); }
	EffectWriter(VolumeCommand cmd, ModCommand::VOL param) : m_volcmd(cmd), m_vol(param), m_isVolEffect(true) { Init(); }

	// Additional constructors:
	// Set row in which writing should start
	EffectWriter &Row(ROWINDEX row) { m_row = row; return *this; }
	// Set channel to which writing should be restricted to
	EffectWriter &Channel(CHANNELINDEX chn) { m_channel = chn; return *this; }
	// Allow multiple effects of the same kind to be written in the same row.
	EffectWriter &AllowMultiple() { m_allowMultiple = true; return *this; }
	// Set retry mode.
	EffectWriter &RetryNextRow() { m_retryMode = rmTryNextRow; return *this; }
	EffectWriter &RetryPreviousRow() { m_retryMode = rmTryPreviousRow; return *this; }

protected:
	RetryMode m_retryMode;
	ROWINDEX m_row;
	CHANNELINDEX m_channel;

	union
	{
		EffectCommand m_command;
		VolumeCommand m_volcmd;
	};
	union
	{
		ModCommand::PARAM m_param;
		ModCommand::VOL m_vol;
	};

	bool m_retry : 1;
	bool m_allowMultiple : 1;
	bool m_isVolEffect : 1;

	// Common data initialisation
	void Init()
	{
		m_row = 0;
		m_channel = CHANNELINDEX_INVALID;	// Any channel
		m_retryMode = rmIgnore;			// If effect couldn't be written, abort.
		m_retry = true;
		m_allowMultiple = false;		// Stop if same type of effect is encountered
	}
};


OPENMPT_NAMESPACE_END
