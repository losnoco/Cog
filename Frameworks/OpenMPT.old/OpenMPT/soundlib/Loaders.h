/*
 * Loaders.h
 * ---------
 * Purpose: Common functions for module loaders
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "BuildSettings.h"

#include "../common/misc_util.h"
#include "../common/FileReader.h"
#include "Sndfile.h"
#include "SampleIO.h"

OPENMPT_NAMESPACE_BEGIN

// Functions to create 4-byte and 2-byte magic byte identifiers in little-endian format
// Use this together with uint32le/uint16le file members.
constexpr uint32 MagicLE(const char(&id)[5])
{
	return static_cast<uint32>((static_cast<uint8>(id[3]) << 24) | (static_cast<uint8>(id[2]) << 16) | (static_cast<uint8>(id[1]) << 8) | static_cast<uint8>(id[0]));
}
constexpr uint16 MagicLE(const char(&id)[3])
{
	return static_cast<uint16>((static_cast<uint8>(id[1]) << 8) | static_cast<uint8>(id[0]));
}
// Functions to create 4-byte and 2-byte magic byte identifiers in big-endian format
// Use this together with uint32be/uint16be file members.
// Note: Historically, some magic bytes in MPT-specific fields are reversed (due to the use of multi-char literals).
// Such fields turned up reversed in files, so MagicBE is used to keep them readable in the code.
constexpr uint32 MagicBE(const char(&id)[5])
{
	return static_cast<uint32>((static_cast<uint8>(id[0]) << 24) | (static_cast<uint8>(id[1]) << 16) | (static_cast<uint8>(id[2]) << 8) | static_cast<uint8>(id[3]));
}
constexpr uint16 MagicBE(const char(&id)[3])
{
	return static_cast<uint16>((static_cast<uint8>(id[0]) << 8) | static_cast<uint8>(id[1]));
}


// Read 'howMany' order items from an array.
// 'stopIndex' is treated as '---', 'ignoreIndex' is treated as '+++'. If the format doesn't support such indices, just pass uint16_max.
template<typename T, size_t arraySize>
bool ReadOrderFromArray(ModSequence &order, const T(&orders)[arraySize], size_t howMany = arraySize, uint16 stopIndex = uint16_max, uint16 ignoreIndex = uint16_max)
{
	static_assert(mpt::is_binary_safe<T>::value);
	LimitMax(howMany, arraySize);
	LimitMax(howMany, MAX_ORDERS);
	ORDERINDEX readEntries = static_cast<ORDERINDEX>(howMany);

	order.resize(readEntries);
	for(int i = 0; i < readEntries; i++)
	{
		PATTERNINDEX pat = static_cast<PATTERNINDEX>(orders[i]);
		if(pat == stopIndex) pat = order.GetInvalidPatIndex();
		else if(pat == ignoreIndex) pat = order.GetIgnoreIndex();
		order.at(i) = pat;
	}
	return true;
}


// Read 'howMany' order items as integers with defined endianness from a file.
// 'stopIndex' is treated as '---', 'ignoreIndex' is treated as '+++'. If the format doesn't support such indices, just pass uint16_max.
template<typename T>
bool ReadOrderFromFile(ModSequence &order, FileReader &file, size_t howMany, uint16 stopIndex = uint16_max, uint16 ignoreIndex = uint16_max)
{
	static_assert(mpt::is_binary_safe<T>::value);
	if(!file.CanRead(howMany * sizeof(T)))
		return false;
	LimitMax(howMany, MAX_ORDERS);
	ORDERINDEX readEntries = static_cast<ORDERINDEX>(howMany);

	order.resize(readEntries);
	T patF;
	for(auto &pat : order)
	{
		file.ReadStruct(patF);
		pat = static_cast<PATTERNINDEX>(patF);
		if(pat == stopIndex) pat = order.GetInvalidPatIndex();
		else if(pat == ignoreIndex) pat = order.GetIgnoreIndex();
	}
	return true;
}

OPENMPT_NAMESPACE_END
