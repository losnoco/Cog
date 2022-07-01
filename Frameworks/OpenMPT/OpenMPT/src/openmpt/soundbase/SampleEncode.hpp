/* SPDX-License-Identifier: BSD-3-Clause */
/* SPDX-FileCopyrightText: OpenMPT Project Developers and Contributors */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "mpt/base/bit.hpp"
#include "mpt/base/macros.hpp"
#include "mpt/base/memory.hpp"
#include "openmpt/base/Types.hpp"

#include <algorithm>

#include <cmath>
#include <cstddef>
#include <cstdlib>


OPENMPT_NAMESPACE_BEGIN


namespace SC
{  // SC = _S_ample_C_onversion


struct EncodeuLaw
{
	using input_t = int16;
	using output_t = std::byte;
	static constexpr uint8 exp_table[17] = {0, 7 << 4, 6 << 4, 5 << 4, 4 << 4, 3 << 4, 2 << 4, 1 << 4, 0 << 4, 0, 0, 0, 0, 0, 0, 0, 0};
	static constexpr uint8 mant_table[17] = {0, 10, 9, 8, 7, 6, 5, 4, 3, 3, 3, 3, 3, 3, 3, 3, 3};
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		uint16 x = static_cast<uint16>(val);
		uint8 out = (x >> 8) & 0x80;
		uint32 abs = x & 0x7fff;
		if(x & 0x8000)
		{
			abs ^= 0x7fff;
			abs += 1;
		}
		x = static_cast<uint16>(std::clamp(static_cast<uint32>(abs + (33 << 2)), static_cast<uint32>(0), static_cast<uint32>(0x7fff)));
		int index = mpt::countl_zero(x);
		out |= exp_table[index];
		out |= (x >> mant_table[index]) & 0x0f;
		out ^= 0xff;
		return mpt::byte_cast<std::byte>(out);
	}
};


struct EncodeALaw
{
	using input_t = int16;
	using output_t = std::byte;
	static constexpr uint8 exp_table[17] = {0, 7 << 4, 6 << 4, 5 << 4, 4 << 4, 3 << 4, 2 << 4, 1 << 4, 0 << 4, 0, 0, 0, 0, 0, 0, 0, 0};
	static constexpr uint8 mant_table[17] = {0, 10, 9, 8, 7, 6, 5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4};
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		int16 sx = std::clamp(val, static_cast<int16>(-32767), static_cast<int16>(32767));
		uint16 x = static_cast<uint16>(sx);
		uint8 out = ((x & 0x8000) ^ 0x8000) >> 8;
		x = static_cast<uint16>(std::abs(sx));
		int index = mpt::countl_zero(x);
		out |= exp_table[index];
		out |= (x >> mant_table[index]) & 0x0f;
		out ^= 0x55;
		return mpt::byte_cast<std::byte>(out);
	}
};


}  // namespace SC


OPENMPT_NAMESPACE_END
