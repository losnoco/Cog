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

#pragma once

#include <vio2sf/NDSSystem.h>
#include <vio2sf/armcpu.h>
#include <vio2sf/MMU.h>
#include <vio2sf/MMU_timing.h>
#include <vio2sf/FIFO.h>
#include <vio2sf/slot1.h>
#include <vio2sf/cp15.h>
#include <vio2sf/SPU.h>
#include <vio2sf/samplecache.h>

struct vio2sf_state
{
    uint32_t partie;
    uint32_t _MMU_MAIN_MEM_MASK;
    uint32_t _MMU_MAIN_MEM_MASK16;
    uint32_t _MMU_MAIN_MEM_MASK32;

    uint64_t nds_timer;
    uint64_t nds_arm9_timer, nds_arm7_timer;

    NDSSystem nds;
    Sequencer sequencer;

    GameInfo gameInfo;

    std::unique_ptr<CFIRMWARE> firmware;

    TCommonSettings CommonSettings;

    bool execute;

    armcpu_t NDS_ARM7, NDS_ARM9;
    struct armcp15_t cp15;
    MMU_struct MMU;
    MMU_struct_new MMU_new;
    MMU_struct_timing MMU_timing;

    IPC_FIFO ipc_fifo[2];

    uint8_t vram_lcdc_map[VRAM_LCDC_PAGES];
    uint8_t vram_arm9_map[VRAM_ARM9_PAGES];
    uint8_t vram_arm7_map[2];

    VramConfiguration vramConfiguration;

    SLOT1INTERFACE slot1_device;

    double DESMUME_SAMPLE_RATE;
    double samples_per_hline;
    double sampleLength;

    double samples;

    int spu_core_samples;

    SoundInterface_struct *soundInterface;

    SPU_struct *SPU_core;
    int SPU_currentCoreNum;
    int volume;
    SampleCache sampleCache;

    s16 *postProcessBuffer;
    size_t postProcessBufferSize;

    size_t buffersize;
    ESynchMode synchmode;
    ESynchMethod synchmethod;
    ISynchronizingAudioBuffer* synchronizer;

    int SNDCoreId;
    SoundInterface_struct *SNDCore;
    void *SNDCoreContext;
    SoundInterface_struct **SNDCoreList;

    vio2sf_state();
    ~vio2sf_state();
};

