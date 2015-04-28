/* Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009 Dean Beeler, Jerome Fisher
 * Copyright (C) 2011, 2012, 2013, 2014 Dean Beeler, Jerome Fisher, Sergey V. Mikayev
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 2.1 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MT32EMU_MIDI_STREAM_PARSER_H
#define MT32EMU_MIDI_STREAM_PARSER_H

#include "mt32emu.h"

namespace MT32Emu {

// Maximum allowed size of streamBuffer
// Should suffice for any reasonable bulk dump SysEx, as the h/w units have only 32K of RAM onboard
const Bit32u MAX_STREAM_BUFFER_SIZE = 32768;

// Provides a context for parsing a stream of MIDI events coming from a single source.
// There can be multiple MIDI sources feeding MIDI events to a single Synth object.
// NOTE: Calls from multiple threads which feed a single Synth object with data must be explicitly synchronised,
// although, no synchronisation is required with the rendering thread.
class MidiStreamParser {
public:
	// The argument specifies streamBuffer initial capacity. The buffer capacity should be large enough to fit the longest SysEx expected.
	// If a longer SysEx occurs, streamBuffer is reallocated to the maximum size of MAX_STREAM_BUFFER_SIZE (32768 bytes).
	// Default capacity is MAX_SYSEX_SIZE (512 bytes) which is enough to fit SysEx messages in common use.
	MidiStreamParser(Bit32u initialStreamBufferCapacity = MAX_SYSEX_SIZE);
	virtual ~MidiStreamParser();

	// Parses a block of raw MIDI bytes. All the parsed MIDI messages are sent in sequence to the user-supplied methods for further processing.
	// SysEx messages are allowed to be fragmented across several calls to this method. Running status is also handled for short messages.
	// NOTE: the total length of a SysEx message being fragmented shall not exceed MAX_STREAM_BUFFER_SIZE (32768 bytes).
	void parseStream(const Bit8u *stream, Bit32u length);

	// Convenience method which accepts a Bit32u-encoded short MIDI message and sends it to the user-supplied method for further processing.
	// The short MIDI message may contain no status byte, the running status is used in this case.
	void processShortMessage(const Bit32u message);

protected:
	// User-supplied method. Invoked when a complete short MIDI message is parsed in the input MIDI stream.
	virtual void handleShortMessage(const Bit32u message) = 0;

	// User-supplied method. Invoked when a complete well-formed System Exclusive MIDI message is parsed in the input MIDI stream.
	virtual void handleSysex(const Bit8u stream[], const Bit32u length) = 0;

	// User-supplied method. Invoked when a System Realtime MIDI message is parsed in the input MIDI stream.
	virtual void handleSytemRealtimeMessage(const Bit8u realtime) = 0;

	// User-supplied method. Invoked when an error occurs during processing the input MIDI stream.
	virtual void printDebug(const char *debugMessage) = 0;

private:
	Bit8u runningStatus;
	Bit8u *streamBuffer;
	Bit32u streamBufferCapacity;
	Bit32u streamBufferSize;

	bool checkStreamBufferCapacity(const bool preserveContent);
	bool processStatusByte(Bit8u &status);
	Bit32u parseShortMessageStatus(const Bit8u stream[]);
	Bit32u parseShortMessageDataBytes(const Bit8u stream[], Bit32u length);
	Bit32u parseSysex(const Bit8u stream[], const Bit32u length);
	Bit32u parseSysexFragment(const Bit8u stream[], const Bit32u length);
};

}
#endif // MT32EMU_MIDI_STREAM_PARSER_H
