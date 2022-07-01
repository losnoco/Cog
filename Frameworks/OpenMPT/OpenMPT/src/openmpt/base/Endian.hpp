/* SPDX-License-Identifier: BSD-3-Clause */
/* SPDX-FileCopyrightText: OpenMPT Project Developers and Contributors */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "mpt/base/bit.hpp"
#include "mpt/base/memory.hpp"
#include "mpt/endian/floatingpoint.hpp"
#include "mpt/endian/integer.hpp"
#include "openmpt/base/Types.hpp"



OPENMPT_NAMESPACE_BEGIN



using int64le = mpt::packed<int64, mpt::LittleEndian_tag>;
using int32le = mpt::packed<int32, mpt::LittleEndian_tag>;
using int16le = mpt::packed<int16, mpt::LittleEndian_tag>;
using int8le = mpt::packed<int8, mpt::LittleEndian_tag>;
using uint64le = mpt::packed<uint64, mpt::LittleEndian_tag>;
using uint32le = mpt::packed<uint32, mpt::LittleEndian_tag>;
using uint16le = mpt::packed<uint16, mpt::LittleEndian_tag>;
using uint8le = mpt::packed<uint8, mpt::LittleEndian_tag>;

using int64be = mpt::packed<int64, mpt::BigEndian_tag>;
using int32be = mpt::packed<int32, mpt::BigEndian_tag>;
using int16be = mpt::packed<int16, mpt::BigEndian_tag>;
using int8be = mpt::packed<int8, mpt::BigEndian_tag>;
using uint64be = mpt::packed<uint64, mpt::BigEndian_tag>;
using uint32be = mpt::packed<uint32, mpt::BigEndian_tag>;
using uint16be = mpt::packed<uint16, mpt::BigEndian_tag>;
using uint8be = mpt::packed<uint8, mpt::BigEndian_tag>;



using IEEE754binary32LE = mpt::IEEE754binary_types<mpt::float_traits<float32>::is_ieee754_binary32ne, mpt::endian::native>::IEEE754binary32LE;
using IEEE754binary32BE = mpt::IEEE754binary_types<mpt::float_traits<float32>::is_ieee754_binary32ne, mpt::endian::native>::IEEE754binary32BE;
using IEEE754binary64LE = mpt::IEEE754binary_types<mpt::float_traits<float64>::is_ieee754_binary64ne, mpt::endian::native>::IEEE754binary64LE;
using IEEE754binary64BE = mpt::IEEE754binary_types<mpt::float_traits<float64>::is_ieee754_binary64ne, mpt::endian::native>::IEEE754binary64BE;


// unaligned

using float32le = mpt::IEEE754binary32EmulatedLE;
using float32be = mpt::IEEE754binary32EmulatedBE;
using float64le = mpt::IEEE754binary64EmulatedLE;
using float64be = mpt::IEEE754binary64EmulatedBE;


// potentially aligned

using float32le_fast = mpt::IEEE754binary32LE;
using float32be_fast = mpt::IEEE754binary32BE;
using float64le_fast = mpt::IEEE754binary64LE;
using float64be_fast = mpt::IEEE754binary64BE;



#define MPT_BINARY_STRUCT(type, size) \
	constexpr bool declare_binary_safe(const type &) { return true; } \
	static_assert(mpt::check_binary_size<type>(size)); \
	/**/



OPENMPT_NAMESPACE_END
