/*
 * misc_util.cpp
 * -------------
 * Purpose: Various useful utility functions.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "misc_util.h"


OPENMPT_NAMESPACE_BEGIN



namespace Util
{


static constexpr mpt::uchar EncodeNibble[16] = {
	UC_('0'), UC_('1'), UC_('2'), UC_('3'),
	UC_('4'), UC_('5'), UC_('6'), UC_('7'),
	UC_('8'), UC_('9'), UC_('A'), UC_('B'),
	UC_('C'), UC_('D'), UC_('E'), UC_('F') };

static inline bool DecodeByte(uint8 &byte, mpt::uchar c1, mpt::uchar c2)
{
	byte = 0;
	if(UC_('0') <= c1 && c1 <= UC_('9'))
	{
		byte += static_cast<uint8>((c1 - UC_('0')) << 4);
	} else if(UC_('A') <= c1 && c1 <= UC_('F'))
	{
		byte += static_cast<uint8>((c1 - UC_('A') + 10) << 4);
	} else if(UC_('a') <= c1 && c1 <= UC_('f'))
	{
		byte += static_cast<uint8>((c1 - UC_('a') + 10) << 4);
	} else
	{
		return false;
	}
	if(UC_('0') <= c2 && c2 <= UC_('9'))
	{
		byte += static_cast<uint8>(c2 - UC_('0'));
	} else if(UC_('A') <= c2 && c2 <= UC_('F'))
	{
		byte += static_cast<uint8>(c2 - UC_('A') + 10);
	} else if(UC_('a') <= c2 && c2 <= UC_('f'))
	{
		byte += static_cast<uint8>(c2 - UC_('a') + 10);
	} else
	{
		return false;
	}
	return true;
}

mpt::ustring BinToHex(mpt::const_byte_span src)
{
	mpt::ustring result;
	result.reserve(src.size() * 2);
	for(std::byte byte : src)
	{
		result.push_back(EncodeNibble[(mpt::byte_cast<uint8>(byte) & 0xf0) >> 4]);
		result.push_back(EncodeNibble[mpt::byte_cast<uint8>(byte) & 0x0f]);
	}
	return result;
}

std::vector<std::byte> HexToBin(const mpt::ustring &src)
{
	std::vector<std::byte> result;
	result.reserve(src.size() / 2);
	for(std::size_t i = 0; (i + 1) < src.size(); i += 2)
	{
		uint8 byte = 0;
		if(!DecodeByte(byte, src[i], src[i + 1]))
		{
			return result;
		}
		result.push_back(mpt::byte_cast<std::byte>(byte));
	}
	return result;
}


} // namespace Util


#if defined(MODPLUG_TRACKER) || (defined(LIBOPENMPT_BUILD) && defined(LIBOPENMPT_BUILD_TEST))

namespace mpt
{

std::string getenv(const std::string &env_var, const std::string &def)
{
#if MPT_OS_WINDOWS && MPT_OS_WINDOWS_WINRT
	MPT_UNREFERENCED_PARAMETER(env_var);
	return def;
#else
	const char *val = std::getenv(env_var.c_str());
	if(!val)
	{
		return def;
	}
	return val;
#endif
}

} // namespace mpt

#endif // MODPLUG_TRACKER || (LIBOPENMPT_BUILD && LIBOPENMPT_BUILD_TEST)


OPENMPT_NAMESPACE_END
