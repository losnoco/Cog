/*
	Copyright (C) 2006-2007 shash
	Copyright (C) 2007-2012 DeSmuME team

	This file is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	This file is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with the this software.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <cmath>
#include <cstring>
#include <vio2sf/types.h>
#include <vio2sf/mem.h>

#ifdef __SSE2__
#include <emmintrin.h>
#endif

// these functions are an unreliable, inaccurate floor.
// it should only be used for positive numbers
// this isnt as fast as it could be if we used a visual c++ intrinsic, but those appear not to be universally available
inline uint32_t u32floor(float f)
{
#ifdef __SSE2__
	return static_cast<uint32_t>(_mm_cvtt_ss2si(_mm_set_ss(f)));
#else
	return static_cast<uint32_t>(f);
#endif
}
inline uint32_t u32floor(double d)
{
#ifdef __SSE2__
	return static_cast<uint32_t>(_mm_cvttsd_si32(_mm_set_sd(d)));
#else
	return static_cast<uint32_t>(d);
#endif
}

// same as above but works for negative values too.
// be sure that the results are the same thing as floorf!
inline int32_t s32floor(float f)
{
#ifdef __SSE2__
	return _mm_cvtss_si32(_mm_add_ss(_mm_set_ss(-0.5f),_mm_add_ss(_mm_set_ss(f), _mm_set_ss(f)))) >> 1;
#else
	return static_cast<int32_t>(floorf(f));
#endif
}
inline int32_t s32floor(double d)
{
	return s32floor(static_cast<float>(d));
}
