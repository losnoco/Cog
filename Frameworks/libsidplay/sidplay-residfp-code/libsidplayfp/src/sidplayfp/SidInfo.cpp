/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 *  Copyright 2011-2015 Leandro Nini
 *  Copyright 2007-2010 Antti Lankila
 *  Copyright 2000 Simon White
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "SidInfo.h"


const char *SidInfo::name() const { return getName(); }

const char *SidInfo::version() const { return getVersion(); }

unsigned int SidInfo::numberOfCredits() const { return getNumberOfCredits(); }
const char *SidInfo::credits(unsigned int i) const { return getCredits(i); }

unsigned int SidInfo::maxsids() const { return getMaxsids(); }

unsigned int SidInfo::channels() const { return getChannels(); }

uint_least16_t SidInfo::driverAddr() const { return getDriverAddr(); }

uint_least16_t SidInfo::driverLength() const { return getDriverLength(); }

const char *SidInfo::speedString() const { return getSpeedString(); }

const char *SidInfo::kernalDesc() const { return getKernalDesc(); }
const char *SidInfo::basicDesc() const { return getBasicDesc(); }
const char *SidInfo::chargenDesc() const { return getChargenDesc(); }

// deprecated
uint_least16_t SidInfo::powerOnDelay() const { return 0; }
