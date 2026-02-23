/*
	Copyright (C) 2026 DeSmuME team

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

#include "vio2sf.h"

vio2sf_state::vio2sf_state()
: MMU_new(this)
{
    partie = 1;
    _MMU_MAIN_MEM_MASK = 0x3FFFFF;
    _MMU_MAIN_MEM_MASK16 = 0x3FFFFF & ~1;
    _MMU_MAIN_MEM_MASK32 = 0x3FFFFF & ~3;

    SPU_currentCoreNum = SNDCORE_DUMMY;
    volume = 100;
    buffersize = 0;
    synchmode = ESynchMode_Synchronous;
    synchmethod = ESynchMethod_0;
	synchronizer = metaspu_construct(synchmethod);

    SNDCoreId = -1;
    SNDCore = nullptr;
    SNDCoreContext = nullptr;
    SNDCoreList = nullptr;

    init_slot1(this);

    SPU_core = nullptr;
    SetDesmumeSampleRate(this, 48000);
    samples = 0;

    postProcessBuffer = nullptr;
    postProcessBufferSize = 0;
}

vio2sf_state::~vio2sf_state()
{
	delete synchronizer;
	if(postProcessBuffer)
		free(postProcessBuffer);
}
