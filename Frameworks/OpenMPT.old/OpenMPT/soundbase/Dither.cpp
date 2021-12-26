/*
 * Dither.cpp
 * ----------
 * Purpose: Dithering when converting to lower resolution sample formats.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"

#include "Dither.h"

#include "../common/misc_util.h"


OPENMPT_NAMESPACE_BEGIN


mpt::ustring DitherNames::GetModeName(DitherMode mode)
{
	switch(mode)
	{
		case DitherNone   : return U_("no"     ); break;
		case DitherDefault: return U_("default"); break;
		case DitherModPlug: return U_("0.5 bit"); break;
		case DitherSimple : return U_("1 bit"  ); break;
		default           : return U_(""       ); break;
	}
}


OPENMPT_NAMESPACE_END
