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

#include "SidTuneInfo.h"

uint_least16_t SidTuneInfo::loadAddr() const { return getLoadAddr(); }

uint_least16_t SidTuneInfo::initAddr() const { return getInitAddr(); }

uint_least16_t SidTuneInfo::playAddr() const { return getPlayAddr(); }

unsigned int SidTuneInfo::songs() const { return getSongs(); }

unsigned int SidTuneInfo::startSong() const { return getStartSong(); }

unsigned int SidTuneInfo::currentSong() const { return getCurrentSong(); }

// deprecated
uint_least16_t SidTuneInfo::sidChipBase1() const { return getSidChipBase(0); }
uint_least16_t SidTuneInfo::sidChipBase2() const { return getSidChipBase(1); }

uint_least16_t SidTuneInfo::sidChipBase(unsigned int i) const { return getSidChipBase(i); }

// deprecated
bool SidTuneInfo::isStereo() const { return getSidChips() > 1; }

int SidTuneInfo::sidChips() const { return getSidChips(); }

int SidTuneInfo::songSpeed() const { return getSongSpeed(); }

uint_least8_t SidTuneInfo::relocStartPage() const { return getRelocStartPage(); }

uint_least8_t SidTuneInfo::relocPages() const { return getRelocPages(); }

// deprecated
SidTuneInfo::model_t SidTuneInfo::sidModel1() const { return getSidModel(0); }
SidTuneInfo::model_t SidTuneInfo::sidModel2() const { return getSidModel(1); }

SidTuneInfo::model_t SidTuneInfo::sidModel(unsigned int i) const { return getSidModel(i); }

SidTuneInfo::compatibility_t SidTuneInfo::compatibility() const { return getCompatibility(); }

unsigned int SidTuneInfo::numberOfInfoStrings() const { return getNumberOfInfoStrings(); }
const char* SidTuneInfo::infoString(unsigned int i) const { return getInfoString(i); }


unsigned int SidTuneInfo::numberOfCommentStrings() const{ return getNumberOfCommentStrings(); }
const char* SidTuneInfo::commentString(unsigned int i) const{ return getCommentString(i); }

uint_least32_t SidTuneInfo::dataFileLen() const { return getDataFileLen(); }

uint_least32_t SidTuneInfo::c64dataLen() const { return getC64dataLen(); }

SidTuneInfo::clock_t SidTuneInfo::clockSpeed() const { return getClockSpeed(); }

const char* SidTuneInfo::formatString() const { return getFormatString(); }

bool SidTuneInfo::fixLoad() const { return getFixLoad(); }

const char* SidTuneInfo::path() const { return getPath(); }

const char* SidTuneInfo::dataFileName() const { return getDataFileName(); }

const char* SidTuneInfo::infoFileName() const { return getInfoFileName(); }
