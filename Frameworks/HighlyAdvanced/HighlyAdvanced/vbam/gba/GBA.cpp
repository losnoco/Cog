#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <stdarg.h>
#include <string.h>
#include "GBA.h"
#include "GBAcpu.h"
#include "GBAinline.h"
#include "Globals.h"

#include "Sound.h"
#include "bios.h"

GBASystem::GBASystem()
{
    N_FLAG = false;
    C_FLAG = false;
    Z_FLAG = false;
    V_FLAG = false;
    armState = true;
    armIrqEnable = true;
    armNextPC = 0x00000000;
    armMode = 0x1f;
    stop = 0x08000568;
    saveType = 0;
    useBios = false;
    skipBios = false;
    frameSkip = 1;
    speedup = false;
    synchronize = true;
    cpuDisableSfx = false;
    cpuIsMultiBoot = false;
    parseDebug = true;
    layerSettings = 0xff00;
    layerEnable = 0xff00;
    speedHack = false;
    cpuSaveType = 0;
    cheatsEnabled = true;
    mirroringEnable = false;
    skipSaveGameBattery = false;
    skipSaveGameCheats = false;

    bios = 0;
    rom = 0;
    internalRAM = 0;
    workRAM = 0;
    paletteRAM = 0;
    vram = 0;
    oam = 0;
    ioMem = 0;

    customBackdropColor = -1;

    DISPCNT  = 0x0080;
    DISPSTAT = 0x0000;
    VCOUNT   = 0x0000;
    BG0CNT   = 0x0000;
    BG1CNT   = 0x0000;
    BG2CNT   = 0x0000;
    BG3CNT   = 0x0000;
    BG0HOFS  = 0x0000;
    BG0VOFS  = 0x0000;
    BG1HOFS  = 0x0000;
    BG1VOFS  = 0x0000;
    BG2HOFS  = 0x0000;
    BG2VOFS  = 0x0000;
    BG3HOFS  = 0x0000;
    BG3VOFS  = 0x0000;
    BG2PA    = 0x0100;
    BG2PB    = 0x0000;
    BG2PC    = 0x0000;
    BG2PD    = 0x0100;
    BG2X_L   = 0x0000;
    BG2X_H   = 0x0000;
    BG2Y_L   = 0x0000;
    BG2Y_H   = 0x0000;
    BG3PA    = 0x0100;
    BG3PB    = 0x0000;
    BG3PC    = 0x0000;
    BG3PD    = 0x0100;
    BG3X_L   = 0x0000;
    BG3X_H   = 0x0000;
    BG3Y_L   = 0x0000;
    BG3Y_H   = 0x0000;
    WIN0H    = 0x0000;
    WIN1H    = 0x0000;
    WIN0V    = 0x0000;
    WIN1V    = 0x0000;
    WININ    = 0x0000;
    WINOUT   = 0x0000;
    MOSAIC   = 0x0000;
    BLDMOD   = 0x0000;
    COLEV    = 0x0000;
    COLY     = 0x0000;
    DM0SAD_L = 0x0000;
    DM0SAD_H = 0x0000;
    DM0DAD_L = 0x0000;
    DM0DAD_H = 0x0000;
    DM0CNT_L = 0x0000;
    DM0CNT_H = 0x0000;
    DM1SAD_L = 0x0000;
    DM1SAD_H = 0x0000;
    DM1DAD_L = 0x0000;
    DM1DAD_H = 0x0000;
    DM1CNT_L = 0x0000;
    DM1CNT_H = 0x0000;
    DM2SAD_L = 0x0000;
    DM2SAD_H = 0x0000;
    DM2DAD_L = 0x0000;
    DM2DAD_H = 0x0000;
    DM2CNT_L = 0x0000;
    DM2CNT_H = 0x0000;
    DM3SAD_L = 0x0000;
    DM3SAD_H = 0x0000;
    DM3DAD_L = 0x0000;
    DM3DAD_H = 0x0000;
    DM3CNT_L = 0x0000;
    DM3CNT_H = 0x0000;
    TM0D     = 0x0000;
    TM0CNT   = 0x0000;
    TM1D     = 0x0000;
    TM1CNT   = 0x0000;
    TM2D     = 0x0000;
    TM2CNT   = 0x0000;
    TM3D     = 0x0000;
    TM3CNT   = 0x0000;
    P1       = 0xFFFF;
    IE       = 0x0000;
    IF       = 0x0000;
    IME      = 0x0000;

    gfxBG2Changed = 0;
    gfxBG3Changed = 0;
    eepromInUse = 0;

    SWITicks = 0;
    IRQTicks = 0;

    mastercode = 0;
    layerEnableDelay = 0;
    busPrefetch = false;
    busPrefetchEnable = false;
    busPrefetchCount = 0;
    cpuDmaTicksToUpdate = 0;
    cpuDmaCount = 0;
    cpuDmaHack = false;
    cpuDmaLast = 0;
    dummyAddress = 0;

    cpuBreakLoop = false;
    cpuNextEvent = 0;

    gbaSaveType = 0; // used to remember the save type on reset
    intState = false;
    stopState = false;
    holdState = false;
    holdType = 0;
    cpuSramEnabled = true;
    cpuFlashEnabled = true;
    cpuEEPROMEnabled = true;
    cpuEEPROMSensorEnabled = false;

    cpuTotalTicks = 0;

    lcdTicks = (useBios && !skipBios) ? 1008 : 208;
    timerOnOffDelay = 0;
    timer0Value = 0;
    timer0On = false;
    timer0Ticks = 0;
    timer0Reload = 0;
    timer0ClockReload  = 0;
    timer1Value = 0;
    timer1On = false;
    timer1Ticks = 0;
    timer1Reload = 0;
    timer1ClockReload  = 0;
    timer2Value = 0;
    timer2On = false;
    timer2Ticks = 0;
    timer2Reload = 0;
    timer2ClockReload  = 0;
    timer3Value = 0;
    timer3On = false;
    timer3Ticks = 0;
    timer3Reload = 0;
    timer3ClockReload  = 0;
    dma0Source = 0;
    dma0Dest = 0;
    dma1Source = 0;
    dma1Dest = 0;
    dma2Source = 0;
    dma2Dest = 0;
    dma3Source = 0;
    dma3Dest = 0;
    fxOn = false;
    windowOn = false;
    frameCount = 0;
    lastTime = 0;
    count = 0;

    capture = 0;
    capturePrevious = 0;
    captureNumber = 0;

    static const u8 init_memoryWait[16] =
      { 0, 0, 2, 0, 0, 0, 0, 0, 4, 4, 4, 4, 4, 4, 4, 0 };
    memcpy(memoryWait, init_memoryWait, sizeof(memoryWait));
    static const u8 init_memoryWait32[16] =
      { 0, 0, 5, 0, 0, 1, 1, 0, 7, 7, 9, 9, 13, 13, 4, 0 };
    memcpy(memoryWait32, init_memoryWait32, sizeof(memoryWait32));
    static const u8 init_memoryWaitSeq[16] =
      { 0, 0, 2, 0, 0, 0, 0, 0, 2, 2, 4, 4, 8, 8, 4, 0 };
    memcpy(memoryWaitSeq, init_memoryWaitSeq, sizeof(memoryWaitSeq));
    static const u8 init_memoryWaitSeq32[16] =
      { 0, 0, 5, 0, 0, 1, 1, 0, 5, 5, 9, 9, 17, 17, 4, 0 };
    memcpy(memoryWaitSeq32, init_memoryWaitSeq32, sizeof(memoryWaitSeq32));

    #ifdef WORDS_BIGENDIAN
    cpuBiosSwapped = false;
    #endif

    romSize = 0x2000000;

    soundDeclicking = true;

    soundSampleRate    = 44100;
    soundInterpolation = true;
    soundPaused        = true;
    soundFiltering     = 0.5f;
    SOUND_CLOCK_TICKS  = SOUND_CLOCK_TICKS_;
    soundTicks         = SOUND_CLOCK_TICKS_;

    soundVolume     = 1.0f;
    soundEnableFlag   = 0x3ff; // emulator channels enabled
    soundFiltering_ = -1;
    soundVolume_    = -1;

    gb_apu = 0;
    stereo_buffer = 0;
}

const int TIMER_TICKS[4] = {
  0,
  6,
  8,
  10
};

const u32  objTilesAddress [3] = {0x010000, 0x014000, 0x014000};
const u8 gamepakRamWaitState[4] = { 4, 3, 2, 8 };
const u8 gamepakWaitState[4] =  { 4, 3, 2, 8 };
const u8 gamepakWaitState0[2] = { 2, 1 };
const u8 gamepakWaitState1[2] = { 4, 1 };
const u8 gamepakWaitState2[2] = { 8, 1 };
const bool isInRom [16]=
  { false, false, false, false, false, false, false, false,
    true, true, true, true, true, true, false, false };

const u32 myROM[] = {
0xEA000006,
0xEA000093,
0xEA000006,
0x00000000,
0x00000000,
0x00000000,
0xEA000088,
0x00000000,
0xE3A00302,
0xE1A0F000,
0xE92D5800,
0xE55EC002,
0xE28FB03C,
0xE79BC10C,
0xE14FB000,
0xE92D0800,
0xE20BB080,
0xE38BB01F,
0xE129F00B,
0xE92D4004,
0xE1A0E00F,
0xE12FFF1C,
0xE8BD4004,
0xE3A0C0D3,
0xE129F00C,
0xE8BD0800,
0xE169F00B,
0xE8BD5800,
0xE1B0F00E,
0x0000009C,
0x0000009C,
0x0000009C,
0x0000009C,
0x000001F8,
0x000001F0,
0x000000AC,
0x000000A0,
0x000000FC,
0x00000168,
0xE12FFF1E,
0xE1A03000,
0xE1A00001,
0xE1A01003,
0xE2113102,
0x42611000,
0xE033C040,
0x22600000,
0xE1B02001,
0xE15200A0,
0x91A02082,
0x3AFFFFFC,
0xE1500002,
0xE0A33003,
0x20400002,
0xE1320001,
0x11A020A2,
0x1AFFFFF9,
0xE1A01000,
0xE1A00003,
0xE1B0C08C,
0x22600000,
0x42611000,
0xE12FFF1E,
0xE92D0010,
0xE1A0C000,
0xE3A01001,
0xE1500001,
0x81A000A0,
0x81A01081,
0x8AFFFFFB,
0xE1A0000C,
0xE1A04001,
0xE3A03000,
0xE1A02001,
0xE15200A0,
0x91A02082,
0x3AFFFFFC,
0xE1500002,
0xE0A33003,
0x20400002,
0xE1320001,
0x11A020A2,
0x1AFFFFF9,
0xE0811003,
0xE1B010A1,
0xE1510004,
0x3AFFFFEE,
0xE1A00004,
0xE8BD0010,
0xE12FFF1E,
0xE0010090,
0xE1A01741,
0xE2611000,
0xE3A030A9,
0xE0030391,
0xE1A03743,
0xE2833E39,
0xE0030391,
0xE1A03743,
0xE2833C09,
0xE283301C,
0xE0030391,
0xE1A03743,
0xE2833C0F,
0xE28330B6,
0xE0030391,
0xE1A03743,
0xE2833C16,
0xE28330AA,
0xE0030391,
0xE1A03743,
0xE2833A02,
0xE2833081,
0xE0030391,
0xE1A03743,
0xE2833C36,
0xE2833051,
0xE0030391,
0xE1A03743,
0xE2833CA2,
0xE28330F9,
0xE0000093,
0xE1A00840,
0xE12FFF1E,
0xE3A00001,
0xE3A01001,
0xE92D4010,
0xE3A03000,
0xE3A04001,
0xE3500000,
0x1B000004,
0xE5CC3301,
0xEB000002,
0x0AFFFFFC,
0xE8BD4010,
0xE12FFF1E,
0xE3A0C301,
0xE5CC3208,
0xE15C20B8,
0xE0110002,
0x10222000,
0x114C20B8,
0xE5CC4208,
0xE12FFF1E,
0xE92D500F,
0xE3A00301,
0xE1A0E00F,
0xE510F004,
0xE8BD500F,
0xE25EF004,
0xE59FD044,
0xE92D5000,
0xE14FC000,
0xE10FE000,
0xE92D5000,
0xE3A0C302,
0xE5DCE09C,
0xE35E00A5,
0x1A000004,
0x05DCE0B4,
0x021EE080,
0xE28FE004,
0x159FF018,
0x059FF018,
0xE59FD018,
0xE8BD5000,
0xE169F00C,
0xE8BD5000,
0xE25EF004,
0x03007FF0,
0x09FE2000,
0x09FFC000,
0x03007FE0
};

inline int CPUUpdateTicks(GBASystem * gba)
{
  int cpuLoopTicks = gba->lcdTicks;

  if(gba->soundTicks < cpuLoopTicks)
    cpuLoopTicks = gba->soundTicks;

  if(gba->timer0On && (gba->timer0Ticks < cpuLoopTicks)) {
    cpuLoopTicks = gba->timer0Ticks;
  }
  if(gba->timer1On && !(gba->TM1CNT & 4) && (gba->timer1Ticks < cpuLoopTicks)) {
    cpuLoopTicks = gba->timer1Ticks;
  }
  if(gba->timer2On && !(gba->TM2CNT & 4) && (gba->timer2Ticks < cpuLoopTicks)) {
    cpuLoopTicks = gba->timer2Ticks;
  }
  if(gba->timer3On && !(gba->TM3CNT & 4) && (gba->timer3Ticks < cpuLoopTicks)) {
    cpuLoopTicks = gba->timer3Ticks;
  }

  if (gba->SWITicks) {
    if (gba->SWITicks < cpuLoopTicks)
        cpuLoopTicks = gba->SWITicks;
  }

  if (gba->IRQTicks) {
    if (gba->IRQTicks < cpuLoopTicks)
        cpuLoopTicks = gba->IRQTicks;
  }

  return cpuLoopTicks;
}

void CPUCleanUp(GBASystem * gba)
{
  if(gba->rom != NULL) {
    free(gba->rom);
    gba->rom = NULL;
  }

  if(gba->vram != NULL) {
    free(gba->vram);
    gba->vram = NULL;
  }

  if(gba->paletteRAM != NULL) {
    free(gba->paletteRAM);
    gba->paletteRAM = NULL;
  }

  if(gba->internalRAM != NULL) {
    free(gba->internalRAM);
    gba->internalRAM = NULL;
  }

  if(gba->workRAM != NULL) {
    free(gba->workRAM);
    gba->workRAM = NULL;
  }

  if(gba->bios != NULL) {
    free(gba->bios);
    gba->bios = NULL;
  }

  if(gba->oam != NULL) {
    free(gba->oam);
    gba->oam = NULL;
  }

  if(gba->ioMem != NULL) {
    free(gba->ioMem);
    gba->ioMem = NULL;
  }
}

int CPULoadRom(GBASystem *gba, const void *rom, u32 size)
{
  gba->romSize = 0x2000000;
  if(gba->rom != NULL) {
    CPUCleanUp(gba);
  }

  gba->rom = (u8 *)malloc(0x2000000);
  if(gba->rom == NULL) {
    return 0;
  }
  gba->workRAM = (u8 *)calloc(1, 0x40000);
  if(gba->workRAM == NULL) {
    return 0;
  }

  if (gba->cpuIsMultiBoot)
  {
      if ( size > 0x40000 ) size = 0x40000;
      memcpy( gba->workRAM, rom, size );
      gba->romSize = size;
  }
  else
  {
      if ( size > 0x2000000 ) size = 0x2000000;
      memcpy( gba->rom, rom, size );
      gba->romSize = size;
  }

  u16 *temp = (u16 *)(gba->rom+((gba->romSize+1)&~1));
  int i;
  for(i = (gba->romSize+1)&~1; i < 0x2000000; i+=2) {
    WRITE16LE(temp, (i >> 1) & 0xFFFF);
    temp++;
  }

  gba->bios = (u8 *)calloc(1,0x4000);
  if(gba->bios == NULL) {
    CPUCleanUp(gba);
    return 0;
  }
  gba->internalRAM = (u8 *)calloc(1,0x8000);
  if(gba->internalRAM == NULL) {
    CPUCleanUp(gba);
    return 0;
  }
  gba->paletteRAM = (u8 *)calloc(1,0x400);
  if(gba->paletteRAM == NULL) {
    CPUCleanUp(gba);
    return 0;
  }
  gba->vram = (u8 *)calloc(1, 0x20000);
  if(gba->vram == NULL) {
    CPUCleanUp(gba);
    return 0;
  }
  gba->oam = (u8 *)calloc(1, 0x400);
  if(gba->oam == NULL) {
    CPUCleanUp(gba);
    return 0;
  }
  gba->ioMem = (u8 *)calloc(1, 0x400);
  if(gba->ioMem == NULL) {
    CPUCleanUp(gba);
    return 0;
  }

  return gba->romSize;
}

void doMirroring (GBASystem *gba, bool b)
{
  u32 mirroredRomSize = (((gba->romSize)>>20) & 0x3F)<<20;
  u32 mirroredRomAddress = gba->romSize;
  if ((mirroredRomSize <=0x800000) && (b))
  {
    mirroredRomAddress = mirroredRomSize;
    if (mirroredRomSize==0)
        mirroredRomSize=0x100000;
    while (mirroredRomAddress<0x01000000)
    {
      memcpy ((u16 *)(gba->rom+mirroredRomAddress), (u16 *)(gba->rom), mirroredRomSize);
      mirroredRomAddress+=mirroredRomSize;
    }
  }
}

void CPUUpdateCPSR(GBASystem *gba)
{
  u32 CPSR = gba->reg[16].I & 0x40;
  if(gba->N_FLAG)
    CPSR |= 0x80000000;
  if(gba->Z_FLAG)
    CPSR |= 0x40000000;
  if(gba->C_FLAG)
    CPSR |= 0x20000000;
  if(gba->V_FLAG)
    CPSR |= 0x10000000;
  if(!gba->armState)
    CPSR |= 0x00000020;
  if(!gba->armIrqEnable)
    CPSR |= 0x80;
  CPSR |= (gba->armMode & 0x1F);
  gba->reg[16].I = CPSR;
}

void CPUUpdateFlags(GBASystem *gba, bool breakLoop)
{
  u32 CPSR = gba->reg[16].I;

  gba->N_FLAG = (CPSR & 0x80000000) ? true: false;
  gba->Z_FLAG = (CPSR & 0x40000000) ? true: false;
  gba->C_FLAG = (CPSR & 0x20000000) ? true: false;
  gba->V_FLAG = (CPSR & 0x10000000) ? true: false;
  gba->armState = (CPSR & 0x20) ? false : true;
  gba->armIrqEnable = (CPSR & 0x80) ? false : true;
  if(breakLoop) {
      if (gba->armIrqEnable && (gba->IF & gba->IE) && (gba->IME & 1))
        gba->cpuNextEvent = gba->cpuTotalTicks;
  }
}

void CPUUpdateFlags(GBASystem *gba)
{
  CPUUpdateFlags(gba, true);
}

#ifdef WORDS_BIGENDIAN
static void CPUSwap(volatile u32 *a, volatile u32 *b)
{
  volatile u32 c = *b;
  *b = *a;
  *a = c;
}
#else
static void CPUSwap(u32 *a, u32 *b)
{
  u32 c = *b;
  *b = *a;
  *a = c;
}
#endif

void CPUSwitchMode(GBASystem *gba, int mode, bool saveState, bool breakLoop)
{
  //  if(armMode == mode)
  //    return;

  CPUUpdateCPSR(gba);

  switch(gba->armMode) {
  case 0x10:
  case 0x1F:
    gba->reg[R13_USR].I = gba->reg[13].I;
    gba->reg[R14_USR].I = gba->reg[14].I;
    gba->reg[17].I = gba->reg[16].I;
    break;
  case 0x11:
    CPUSwap(&gba->reg[R8_FIQ].I, &gba->reg[8].I);
    CPUSwap(&gba->reg[R9_FIQ].I, &gba->reg[9].I);
    CPUSwap(&gba->reg[R10_FIQ].I, &gba->reg[10].I);
    CPUSwap(&gba->reg[R11_FIQ].I, &gba->reg[11].I);
    CPUSwap(&gba->reg[R12_FIQ].I, &gba->reg[12].I);
    gba->reg[R13_FIQ].I = gba->reg[13].I;
    gba->reg[R14_FIQ].I = gba->reg[14].I;
    gba->reg[SPSR_FIQ].I = gba->reg[17].I;
    break;
  case 0x12:
    gba->reg[R13_IRQ].I  = gba->reg[13].I;
    gba->reg[R14_IRQ].I  = gba->reg[14].I;
    gba->reg[SPSR_IRQ].I =  gba->reg[17].I;
    break;
  case 0x13:
    gba->reg[R13_SVC].I  = gba->reg[13].I;
    gba->reg[R14_SVC].I  = gba->reg[14].I;
    gba->reg[SPSR_SVC].I =  gba->reg[17].I;
    break;
  case 0x17:
    gba->reg[R13_ABT].I  = gba->reg[13].I;
    gba->reg[R14_ABT].I  = gba->reg[14].I;
    gba->reg[SPSR_ABT].I =  gba->reg[17].I;
    break;
  case 0x1b:
    gba->reg[R13_UND].I  = gba->reg[13].I;
    gba->reg[R14_UND].I  = gba->reg[14].I;
    gba->reg[SPSR_UND].I =  gba->reg[17].I;
    break;
  }

  u32 CPSR = gba->reg[16].I;
  u32 SPSR = gba->reg[17].I;

  switch(mode) {
  case 0x10:
  case 0x1F:
    gba->reg[13].I = gba->reg[R13_USR].I;
    gba->reg[14].I = gba->reg[R14_USR].I;
    gba->reg[16].I = SPSR;
    break;
  case 0x11:
    CPUSwap(&gba->reg[8].I, &gba->reg[R8_FIQ].I);
    CPUSwap(&gba->reg[9].I, &gba->reg[R9_FIQ].I);
    CPUSwap(&gba->reg[10].I, &gba->reg[R10_FIQ].I);
    CPUSwap(&gba->reg[11].I, &gba->reg[R11_FIQ].I);
    CPUSwap(&gba->reg[12].I, &gba->reg[R12_FIQ].I);
    gba->reg[13].I = gba->reg[R13_FIQ].I;
    gba->reg[14].I = gba->reg[R14_FIQ].I;
    if(saveState)
      gba->reg[17].I = CPSR;
    else
      gba->reg[17].I = gba->reg[SPSR_FIQ].I;
    break;
  case 0x12:
    gba->reg[13].I = gba->reg[R13_IRQ].I;
    gba->reg[14].I = gba->reg[R14_IRQ].I;
    gba->reg[16].I = SPSR;
    if(saveState)
      gba->reg[17].I = CPSR;
    else
      gba->reg[17].I = gba->reg[SPSR_IRQ].I;
    break;
  case 0x13:
    gba->reg[13].I = gba->reg[R13_SVC].I;
    gba->reg[14].I = gba->reg[R14_SVC].I;
    gba->reg[16].I = SPSR;
    if(saveState)
      gba->reg[17].I = CPSR;
    else
      gba->reg[17].I = gba->reg[SPSR_SVC].I;
    break;
  case 0x17:
    gba->reg[13].I = gba->reg[R13_ABT].I;
    gba->reg[14].I = gba->reg[R14_ABT].I;
    gba->reg[16].I = SPSR;
    if(saveState)
      gba->reg[17].I = CPSR;
    else
      gba->reg[17].I = gba->reg[SPSR_ABT].I;
    break;
  case 0x1b:
    gba->reg[13].I = gba->reg[R13_UND].I;
    gba->reg[14].I = gba->reg[R14_UND].I;
    gba->reg[16].I = SPSR;
    if(saveState)
      gba->reg[17].I = CPSR;
    else
      gba->reg[17].I = gba->reg[SPSR_UND].I;
    break;
  default:
    break;
  }
  gba->armMode = mode;
  CPUUpdateFlags(gba, breakLoop);
  CPUUpdateCPSR(gba);
}

void CPUSwitchMode(GBASystem *gba, int mode, bool saveState)
{
  CPUSwitchMode(gba, mode, saveState, true);
}

void CPUUndefinedException(GBASystem *gba)
{
  u32 PC = gba->reg[15].I;
  bool savedArmState = gba->armState;
  CPUSwitchMode(gba, 0x1b, true, false);
  gba->reg[14].I = PC - (savedArmState ? 4 : 2);
  gba->reg[15].I = 0x04;
  gba->armState = true;
  gba->armIrqEnable = false;
  gba->armNextPC = 0x04;
  ARM_PREFETCH;
  gba->reg[15].I += 4;
}

void CPUSoftwareInterrupt(GBASystem *gba)
{
  u32 PC = gba->reg[15].I;
  bool savedArmState = gba->armState;
  CPUSwitchMode(gba, 0x13, true, false);
  gba->reg[14].I = PC - (savedArmState ? 4 : 2);
  gba->reg[15].I = 0x08;
  gba->armState = true;
  gba->armIrqEnable = false;
  gba->armNextPC = 0x08;
  ARM_PREFETCH;
  gba->reg[15].I += 4;
}

void CPUSoftwareInterrupt(GBASystem *gba, int comment)
{
  if(gba->armState) comment >>= 16;
  if(comment == 0xfa) {
    return;
  }
  if(gba->useBios) {
    CPUSoftwareInterrupt(gba);
    return;
  }
  switch(comment) {
  case 0x00:
    BIOS_SoftReset(gba);
    ARM_PREFETCH;
    break;
  case 0x01:
    BIOS_RegisterRamReset(gba);
    break;
  case 0x02:
    gba->holdState = true;
    gba->holdType = -1;
    gba->cpuNextEvent = gba->cpuTotalTicks;
    break;
  case 0x03:
    gba->holdState = true;
    gba->holdType = -1;
    gba->stopState = true;
    gba->cpuNextEvent = gba->cpuTotalTicks;
    break;
  case 0x04:
    CPUSoftwareInterrupt(gba);
    break;
  case 0x05:
    CPUSoftwareInterrupt(gba);
    break;
  case 0x06:
    CPUSoftwareInterrupt(gba);
    break;
  case 0x07:
    CPUSoftwareInterrupt(gba);
    break;
  case 0x08:
    BIOS_Sqrt(gba);
    break;
  case 0x09:
    BIOS_ArcTan(gba);
    break;
  case 0x0A:
    BIOS_ArcTan2(gba);
    break;
  case 0x0B:
    {
      int len = (gba->reg[2].I & 0x1FFFFF) >>1;
      if (!(((gba->reg[0].I & 0xe000000) == 0) ||
         ((gba->reg[0].I + len) & 0xe000000) == 0))
      {
        if ((gba->reg[2].I >> 24) & 1)
        {
          if ((gba->reg[2].I >> 26) & 1)
          gba->SWITicks = (7 + gba->memoryWait32[(gba->reg[1].I>>24) & 0xF]) * (len>>1);
          else
          gba->SWITicks = (8 + gba->memoryWait[(gba->reg[1].I>>24) & 0xF]) * (len);
        }
        else
        {
          if ((gba->reg[2].I >> 26) & 1)
          gba->SWITicks = (10 + gba->memoryWait32[(gba->reg[0].I>>24) & 0xF] +
              gba->memoryWait32[(gba->reg[1].I>>24) & 0xF]) * (len>>1);
          else
          gba->SWITicks = (11 + gba->memoryWait[(gba->reg[0].I>>24) & 0xF] +
              gba->memoryWait[(gba->reg[1].I>>24) & 0xF]) * len;
        }
      }
    }
    BIOS_CpuSet(gba);
    break;
  case 0x0C:
    {
      int len = (gba->reg[2].I & 0x1FFFFF) >>5;
      if (!(((gba->reg[0].I & 0xe000000) == 0) ||
         ((gba->reg[0].I + len) & 0xe000000) == 0))
      {
        if ((gba->reg[2].I >> 24) & 1)
          gba->SWITicks = (6 + gba->memoryWait32[(gba->reg[1].I>>24) & 0xF] +
              7 * (gba->memoryWaitSeq32[(gba->reg[1].I>>24) & 0xF] + 1)) * len;
        else
          gba->SWITicks = (9 + gba->memoryWait32[(gba->reg[0].I>>24) & 0xF] +
              gba->memoryWait32[(gba->reg[1].I>>24) & 0xF] +
              7 * (gba->memoryWaitSeq32[(gba->reg[0].I>>24) & 0xF] +
              gba->memoryWaitSeq32[(gba->reg[1].I>>24) & 0xF] + 2)) * len;
      }
    }
    BIOS_CpuFastSet(gba);
    break;
  case 0x0D:
    BIOS_GetBiosChecksum(gba);
    break;
  case 0x0E:
    BIOS_BgAffineSet(gba);
    break;
  case 0x0F:
    BIOS_ObjAffineSet(gba);
    break;
  case 0x10:
    {
      int len = CPUReadHalfWord(gba, gba->reg[2].I);
      if (!(((gba->reg[0].I & 0xe000000) == 0) ||
         ((gba->reg[0].I + len) & 0xe000000) == 0))
        gba->SWITicks = (32 + gba->memoryWait[(gba->reg[0].I>>24) & 0xF]) * len;
    }
    BIOS_BitUnPack(gba);
    break;
  case 0x11:
    {
      u32 len = CPUReadMemory(gba, gba->reg[0].I) >> 8;
      if(!(((gba->reg[0].I & 0xe000000) == 0) ||
          ((gba->reg[0].I + (len & 0x1fffff)) & 0xe000000) == 0))
        gba->SWITicks = (9 + gba->memoryWait[(gba->reg[1].I>>24) & 0xF]) * len;
    }
    BIOS_LZ77UnCompWram(gba);
    break;
  case 0x12:
    {
      u32 len = CPUReadMemory(gba, gba->reg[0].I) >> 8;
      if(!(((gba->reg[0].I & 0xe000000) == 0) ||
          ((gba->reg[0].I + (len & 0x1fffff)) & 0xe000000) == 0))
        gba->SWITicks = (19 + gba->memoryWait[(gba->reg[1].I>>24) & 0xF]) * len;
    }
    BIOS_LZ77UnCompVram(gba);
    break;
  case 0x13:
    {
      u32 len = CPUReadMemory(gba, gba->reg[0].I) >> 8;
      if(!(((gba->reg[0].I & 0xe000000) == 0) ||
          ((gba->reg[0].I + (len & 0x1fffff)) & 0xe000000) == 0))
        gba->SWITicks = (29 + (gba->memoryWait[(gba->reg[0].I>>24) & 0xF]<<1)) * len;
    }
    BIOS_HuffUnComp(gba);
    break;
  case 0x14:
    {
      u32 len = CPUReadMemory(gba, gba->reg[0].I) >> 8;
      if(!(((gba->reg[0].I & 0xe000000) == 0) ||
          ((gba->reg[0].I + (len & 0x1fffff)) & 0xe000000) == 0))
        gba->SWITicks = (11 + gba->memoryWait[(gba->reg[0].I>>24) & 0xF] +
          gba->memoryWait[(gba->reg[1].I>>24) & 0xF]) * len;
    }
    BIOS_RLUnCompWram(gba);
    break;
  case 0x15:
    {
      u32 len = CPUReadMemory(gba, gba->reg[0].I) >> 9;
      if(!(((gba->reg[0].I & 0xe000000) == 0) ||
          ((gba->reg[0].I + (len & 0x1fffff)) & 0xe000000) == 0))
        gba->SWITicks = (34 + (gba->memoryWait[(gba->reg[0].I>>24) & 0xF] << 1) +
          gba->memoryWait[(gba->reg[1].I>>24) & 0xF]) * len;
    }
    BIOS_RLUnCompVram(gba);
    break;
  case 0x16:
    {
      u32 len = CPUReadMemory(gba, gba->reg[0].I) >> 8;
      if(!(((gba->reg[0].I & 0xe000000) == 0) ||
          ((gba->reg[0].I + (len & 0x1fffff)) & 0xe000000) == 0))
        gba->SWITicks = (13 + gba->memoryWait[(gba->reg[0].I>>24) & 0xF] +
          gba->memoryWait[(gba->reg[1].I>>24) & 0xF]) * len;
    }
    BIOS_Diff8bitUnFilterWram(gba);
    break;
  case 0x17:
    {
      u32 len = CPUReadMemory(gba, gba->reg[0].I) >> 9;
      if(!(((gba->reg[0].I & 0xe000000) == 0) ||
          ((gba->reg[0].I + (len & 0x1fffff)) & 0xe000000) == 0))
        gba->SWITicks = (39 + (gba->memoryWait[(gba->reg[0].I>>24) & 0xF]<<1) +
          gba->memoryWait[(gba->reg[1].I>>24) & 0xF]) * len;
    }
    BIOS_Diff8bitUnFilterVram(gba);
    break;
  case 0x18:
    {
      u32 len = CPUReadMemory(gba, gba->reg[0].I) >> 9;
      if(!(((gba->reg[0].I & 0xe000000) == 0) ||
          ((gba->reg[0].I + (len & 0x1fffff)) & 0xe000000) == 0))
        gba->SWITicks = (13 + gba->memoryWait[(gba->reg[0].I>>24) & 0xF] +
          gba->memoryWait[(gba->reg[1].I>>24) & 0xF]) * len;
    }
    BIOS_Diff16bitUnFilter(gba);
    break;
  case 0x19:
    if(gba->reg[0].I)
      soundPause(gba);
    else
      soundResume(gba);
    break;
  case 0x1F:
    BIOS_MidiKey2Freq(gba);
    break;
  case 0x2A:
    BIOS_SndDriverJmpTableCopy(gba);
  default:
    break;
  }
}

void CPUCompareVCOUNT(GBASystem *gba)
{
  if(gba->VCOUNT == (gba->DISPSTAT >> 8)) {
    gba->DISPSTAT |= 4;
    UPDATE_REG(0x04, gba->DISPSTAT);

    if(gba->DISPSTAT & 0x20) {
      gba->IF |= 4;
      UPDATE_REG(0x202, gba->IF);
    }
  } else {
    gba->DISPSTAT &= 0xFFFB;
    UPDATE_REG(0x4, gba->DISPSTAT);
  }
  if (gba->layerEnableDelay>0)
  {
      gba->layerEnableDelay--;
      if (gba->layerEnableDelay==1)
          gba->layerEnable = gba->layerSettings & gba->DISPCNT;
  }

}

void doDMA(GBASystem *gba, u32 &s, u32 &d, u32 si, u32 di, u32 c, int transfer32)
{
  int sm = s >> 24;
  int dm = d >> 24;
  int sw = 0;
  int dw = 0;
  int sc = c;

  gba->cpuDmaCount = c;
  // This is done to get the correct waitstates.
  if (sm>15)
      sm=15;
  if (dm>15)
      dm=15;

  //if ((sm>=0x05) && (sm<=0x07) || (dm>=0x05) && (dm <=0x07))
  //    blank = (((DISPSTAT | ((DISPSTAT>>1)&1))==1) ?  true : false);

  if(transfer32) {
    s &= 0xFFFFFFFC;
    if(s < 0x02000000 && (gba->reg[15].I >> 24)) {
      while(c != 0) {
        CPUWriteMemory(gba, d, 0);
        d += di;
        c--;
      }
    } else {
      while(c != 0) {
        gba->cpuDmaLast = CPUReadMemory(gba, s);
        CPUWriteMemory(gba, d, gba->cpuDmaLast);
        d += di;
        s += si;
        c--;
      }
    }
  } else {
    s &= 0xFFFFFFFE;
    si = (int)si >> 1;
    di = (int)di >> 1;
    if(s < 0x02000000 && (gba->reg[15].I >> 24)) {
      while(c != 0) {
        CPUWriteHalfWord(gba, d, 0);
        d += di;
        c--;
      }
    } else {
      while(c != 0) {
        gba->cpuDmaLast = CPUReadHalfWord(gba, s);
        CPUWriteHalfWord(gba, d, gba->cpuDmaLast);
        gba->cpuDmaLast |= (gba->cpuDmaLast<<16);
        d += di;
        s += si;
        c--;
      }
    }
  }

  gba->cpuDmaCount = 0;

  int totalTicks = 0;

  if(transfer32) {
      sw =1+gba->memoryWaitSeq32[sm & 15];
      dw =1+gba->memoryWaitSeq32[dm & 15];
      totalTicks = (sw+dw)*(sc-1) + 6 + gba->memoryWait32[sm & 15] +
          gba->memoryWaitSeq32[dm & 15];
  }
  else
  {
     sw = 1+gba->memoryWaitSeq[sm & 15];
     dw = 1+gba->memoryWaitSeq[dm & 15];
      totalTicks = (sw+dw)*(sc-1) + 6 + gba->memoryWait[sm & 15] +
          gba->memoryWaitSeq[dm & 15];
  }

  gba->cpuDmaTicksToUpdate += totalTicks;

}

void CPUCheckDMA(GBASystem *gba, int reason, int dmamask)
{
  // DMA 0
  if((gba->DM0CNT_H & 0x8000) && (dmamask & 1)) {
    if(((gba->DM0CNT_H >> 12) & 3) == reason) {
      u32 sourceIncrement = 4;
      u32 destIncrement = 4;
      switch((gba->DM0CNT_H >> 7) & 3) {
      case 0:
        break;
      case 1:
        sourceIncrement = (u32)-4;
        break;
      case 2:
        sourceIncrement = 0;
        break;
      }
      switch((gba->DM0CNT_H >> 5) & 3) {
      case 0:
        break;
      case 1:
        destIncrement = (u32)-4;
        break;
      case 2:
        destIncrement = 0;
        break;
      }
      doDMA(gba, gba->dma0Source, gba->dma0Dest, sourceIncrement, destIncrement,
            gba->DM0CNT_L ? gba->DM0CNT_L : 0x4000,
            gba->DM0CNT_H & 0x0400);
      gba->cpuDmaHack = true;

      if(gba->DM0CNT_H & 0x4000) {
        gba->IF |= 0x0100;
        UPDATE_REG(0x202, gba->IF);
        gba->cpuNextEvent = gba->cpuTotalTicks;
      }

      if(((gba->DM0CNT_H >> 5) & 3) == 3) {
        gba->dma0Dest = gba->DM0DAD_L | (gba->DM0DAD_H << 16);
      }

      if(!(gba->DM0CNT_H & 0x0200) || (reason == 0)) {
        gba->DM0CNT_H &= 0x7FFF;
        UPDATE_REG(0xBA, gba->DM0CNT_H);
      }
    }
  }

  // DMA 1
  if((gba->DM1CNT_H & 0x8000) && (dmamask & 2)) {
    if(((gba->DM1CNT_H >> 12) & 3) == reason) {
      u32 sourceIncrement = 4;
      u32 destIncrement = 4;
      switch((gba->DM1CNT_H >> 7) & 3) {
      case 0:
        break;
      case 1:
        sourceIncrement = (u32)-4;
        break;
      case 2:
        sourceIncrement = 0;
        break;
      }
      switch((gba->DM1CNT_H >> 5) & 3) {
      case 0:
        break;
      case 1:
        destIncrement = (u32)-4;
        break;
      case 2:
        destIncrement = 0;
        break;
      }
      if(reason == 3) {
        doDMA(gba, gba->dma1Source, gba->dma1Dest, sourceIncrement, 0, 4,
              0x0400);
      } else {
        doDMA(gba, gba->dma1Source, gba->dma1Dest, sourceIncrement, destIncrement,
              gba->DM1CNT_L ? gba->DM1CNT_L : 0x4000,
              gba->DM1CNT_H & 0x0400);
      }
      gba->cpuDmaHack = true;

      if(gba->DM1CNT_H & 0x4000) {
        gba->IF |= 0x0200;
        UPDATE_REG(0x202, gba->IF);
        gba->cpuNextEvent = gba->cpuTotalTicks;
      }

      if(((gba->DM1CNT_H >> 5) & 3) == 3) {
        gba->dma1Dest = gba->DM1DAD_L | (gba->DM1DAD_H << 16);
      }

      if(!(gba->DM1CNT_H & 0x0200) || (reason == 0)) {
        gba->DM1CNT_H &= 0x7FFF;
        UPDATE_REG(0xC6, gba->DM1CNT_H);
      }
    }
  }

  // DMA 2
  if((gba->DM2CNT_H & 0x8000) && (dmamask & 4)) {
    if(((gba->DM2CNT_H >> 12) & 3) == reason) {
      u32 sourceIncrement = 4;
      u32 destIncrement = 4;
      switch((gba->DM2CNT_H >> 7) & 3) {
      case 0:
        break;
      case 1:
        sourceIncrement = (u32)-4;
        break;
      case 2:
        sourceIncrement = 0;
        break;
      }
      switch((gba->DM2CNT_H >> 5) & 3) {
      case 0:
        break;
      case 1:
        destIncrement = (u32)-4;
        break;
      case 2:
        destIncrement = 0;
        break;
      }
      if(reason == 3) {
        doDMA(gba, gba->dma2Source, gba->dma2Dest, sourceIncrement, 0, 4,
              0x0400);
      } else {
        doDMA(gba, gba->dma2Source, gba->dma2Dest, sourceIncrement, destIncrement,
              gba->DM2CNT_L ? gba->DM2CNT_L : 0x4000,
              gba->DM2CNT_H & 0x0400);
      }
      gba->cpuDmaHack = true;

      if(gba->DM2CNT_H & 0x4000) {
        gba->IF |= 0x0400;
        UPDATE_REG(0x202, gba->IF);
        gba->cpuNextEvent = gba->cpuTotalTicks;
      }

      if(((gba->DM2CNT_H >> 5) & 3) == 3) {
        gba->dma2Dest = gba->DM2DAD_L | (gba->DM2DAD_H << 16);
      }

      if(!(gba->DM2CNT_H & 0x0200) || (reason == 0)) {
        gba->DM2CNT_H &= 0x7FFF;
        UPDATE_REG(0xD2, gba->DM2CNT_H);
      }
    }
  }

  // DMA 3
  if((gba->DM3CNT_H & 0x8000) && (dmamask & 8)) {
    if(((gba->DM3CNT_H >> 12) & 3) == reason) {
      u32 sourceIncrement = 4;
      u32 destIncrement = 4;
      switch((gba->DM3CNT_H >> 7) & 3) {
      case 0:
        break;
      case 1:
        sourceIncrement = (u32)-4;
        break;
      case 2:
        sourceIncrement = 0;
        break;
      }
      switch((gba->DM3CNT_H >> 5) & 3) {
      case 0:
        break;
      case 1:
        destIncrement = (u32)-4;
        break;
      case 2:
        destIncrement = 0;
        break;
      }
      doDMA(gba, gba->dma3Source, gba->dma3Dest, sourceIncrement, destIncrement,
            gba->DM3CNT_L ? gba->DM3CNT_L : 0x10000,
            gba->DM3CNT_H & 0x0400);
      if(gba->DM3CNT_H & 0x4000) {
        gba->IF |= 0x0800;
        UPDATE_REG(0x202, gba->IF);
        gba->cpuNextEvent = gba->cpuTotalTicks;
      }

      if(((gba->DM3CNT_H >> 5) & 3) == 3) {
        gba->dma3Dest = gba->DM3DAD_L | (gba->DM3DAD_H << 16);
      }

      if(!(gba->DM3CNT_H & 0x0200) || (reason == 0)) {
        gba->DM3CNT_H &= 0x7FFF;
        UPDATE_REG(0xDE, gba->DM3CNT_H);
      }
    }
  }
}

void CPUUpdateRegister(GBASystem *gba, u32 address, u16 value)
{
  switch(address)
  {
  case 0x00:
    { // we need to place the following code in { } because we declare & initialize variables in a case statement
      if((value & 7) > 5) {
        // display modes above 0-5 are prohibited
        gba->DISPCNT = (value & 7);
      }
      bool change = (0 != ((gba->DISPCNT ^ value) & 0x80));
      bool changeBG = (0 != ((gba->DISPCNT ^ value) & 0x0F00));
      u16 changeBGon = ((~gba->DISPCNT) & value) & 0x0F00; // these layers are being activated

      gba->DISPCNT = (value & 0xFFF7); // bit 3 can only be accessed by the BIOS to enable GBC mode
      UPDATE_REG(0x00, gba->DISPCNT);

      if(changeBGon) {
        gba->layerEnableDelay = 4;
        gba->layerEnable = gba->layerSettings & value & (~changeBGon);
      } else {
        gba->layerEnable = gba->layerSettings & value;
        // CPUUpdateTicks();
      }

      gba->windowOn = (gba->layerEnable & 0x6000) ? true : false;
      if(change && !((value & 0x80))) {
        if(!(gba->DISPSTAT & 1)) {
          gba->lcdTicks = 1008;
          //      VCOUNT = 0;
          //      UPDATE_REG(0x06, VCOUNT);
          gba->DISPSTAT &= 0xFFFC;
          UPDATE_REG(0x04, gba->DISPSTAT);
          CPUCompareVCOUNT(gba);
        }
        //        (*renderLine)();
      }
      // we only care about changes in BG0-BG3
      if(changeBG) {
      }
      break;
    }
  case 0x04:
    gba->DISPSTAT = (value & 0xFF38) | (gba->DISPSTAT & 7);
    UPDATE_REG(0x04, gba->DISPSTAT);
    break;
  case 0x06:
    // not writable
    break;
  case 0x08:
    gba->BG0CNT = (value & 0xDFCF);
    UPDATE_REG(0x08, gba->BG0CNT);
    break;
  case 0x0A:
    gba->BG1CNT = (value & 0xDFCF);
    UPDATE_REG(0x0A, gba->BG1CNT);
    break;
  case 0x0C:
    gba->BG2CNT = (value & 0xFFCF);
    UPDATE_REG(0x0C, gba->BG2CNT);
    break;
  case 0x0E:
    gba->BG3CNT = (value & 0xFFCF);
    UPDATE_REG(0x0E, gba->BG3CNT);
    break;
  case 0x10:
    gba->BG0HOFS = value & 511;
    UPDATE_REG(0x10, gba->BG0HOFS);
    break;
  case 0x12:
    gba->BG0VOFS = value & 511;
    UPDATE_REG(0x12, gba->BG0VOFS);
    break;
  case 0x14:
    gba->BG1HOFS = value & 511;
    UPDATE_REG(0x14, gba->BG1HOFS);
    break;
  case 0x16:
    gba->BG1VOFS = value & 511;
    UPDATE_REG(0x16, gba->BG1VOFS);
    break;
  case 0x18:
    gba->BG2HOFS = value & 511;
    UPDATE_REG(0x18, gba->BG2HOFS);
    break;
  case 0x1A:
    gba->BG2VOFS = value & 511;
    UPDATE_REG(0x1A, gba->BG2VOFS);
    break;
  case 0x1C:
    gba->BG3HOFS = value & 511;
    UPDATE_REG(0x1C, gba->BG3HOFS);
    break;
  case 0x1E:
    gba->BG3VOFS = value & 511;
    UPDATE_REG(0x1E, gba->BG3VOFS);
    break;
  case 0x20:
    gba->BG2PA = value;
    UPDATE_REG(0x20, gba->BG2PA);
    break;
  case 0x22:
    gba->BG2PB = value;
    UPDATE_REG(0x22, gba->BG2PB);
    break;
  case 0x24:
    gba->BG2PC = value;
    UPDATE_REG(0x24, gba->BG2PC);
    break;
  case 0x26:
    gba->BG2PD = value;
    UPDATE_REG(0x26, gba->BG2PD);
    break;
  case 0x28:
    gba->BG2X_L = value;
    UPDATE_REG(0x28, gba->BG2X_L);
    gba->gfxBG2Changed |= 1;
    break;
  case 0x2A:
    gba->BG2X_H = (value & 0xFFF);
    UPDATE_REG(0x2A, gba->BG2X_H);
    gba->gfxBG2Changed |= 1;
    break;
  case 0x2C:
    gba->BG2Y_L = value;
    UPDATE_REG(0x2C, gba->BG2Y_L);
    gba->gfxBG2Changed |= 2;
    break;
  case 0x2E:
    gba->BG2Y_H = value & 0xFFF;
    UPDATE_REG(0x2E, gba->BG2Y_H);
    gba->gfxBG2Changed |= 2;
    break;
  case 0x30:
    gba->BG3PA = value;
    UPDATE_REG(0x30, gba->BG3PA);
    break;
  case 0x32:
    gba->BG3PB = value;
    UPDATE_REG(0x32, gba->BG3PB);
    break;
  case 0x34:
    gba->BG3PC = value;
    UPDATE_REG(0x34, gba->BG3PC);
    break;
  case 0x36:
    gba->BG3PD = value;
    UPDATE_REG(0x36, gba->BG3PD);
    break;
  case 0x38:
    gba->BG3X_L = value;
    UPDATE_REG(0x38, gba->BG3X_L);
    gba->gfxBG3Changed |= 1;
    break;
  case 0x3A:
    gba->BG3X_H = value & 0xFFF;
    UPDATE_REG(0x3A, gba->BG3X_H);
    gba->gfxBG3Changed |= 1;
    break;
  case 0x3C:
    gba->BG3Y_L = value;
    UPDATE_REG(0x3C, gba->BG3Y_L);
    gba->gfxBG3Changed |= 2;
    break;
  case 0x3E:
    gba->BG3Y_H = value & 0xFFF;
    UPDATE_REG(0x3E, gba->BG3Y_H);
    gba->gfxBG3Changed |= 2;
    break;
  case 0x40:
    gba->WIN0H = value;
    UPDATE_REG(0x40, gba->WIN0H);
    break;
  case 0x42:
    gba->WIN1H = value;
    UPDATE_REG(0x42, gba->WIN1H);
    break;
  case 0x44:
    gba->WIN0V = value;
    UPDATE_REG(0x44, gba->WIN0V);
    break;
  case 0x46:
    gba->WIN1V = value;
    UPDATE_REG(0x46, gba->WIN1V);
    break;
  case 0x48:
    gba->WININ = value & 0x3F3F;
    UPDATE_REG(0x48, gba->WININ);
    break;
  case 0x4A:
    gba->WINOUT = value & 0x3F3F;
    UPDATE_REG(0x4A, gba->WINOUT);
    break;
  case 0x4C:
    gba->MOSAIC = value;
    UPDATE_REG(0x4C, gba->MOSAIC);
    break;
  case 0x50:
    gba->BLDMOD = value & 0x3FFF;
    UPDATE_REG(0x50, gba->BLDMOD);
    gba->fxOn = ((gba->BLDMOD>>6)&3) != 0;
    break;
  case 0x52:
    gba->COLEV = value & 0x1F1F;
    UPDATE_REG(0x52, gba->COLEV);
    break;
  case 0x54:
    gba->COLY = value & 0x1F;
    UPDATE_REG(0x54, gba->COLY);
    break;
  case 0x60:
  case 0x62:
  case 0x64:
  case 0x68:
  case 0x6c:
  case 0x70:
  case 0x72:
  case 0x74:
  case 0x78:
  case 0x7c:
  case 0x80:
  case 0x84:
    soundEvent(gba, address&0xFF, (u8)(value & 0xFF));
    soundEvent(gba, (address&0xFF)+1, (u8)(value>>8));
    break;
  case 0x82:
  case 0x88:
  case 0xa0:
  case 0xa2:
  case 0xa4:
  case 0xa6:
  case 0x90:
  case 0x92:
  case 0x94:
  case 0x96:
  case 0x98:
  case 0x9a:
  case 0x9c:
  case 0x9e:
    soundEvent(gba, address&0xFF, value);
    break;
  case 0xB0:
    gba->DM0SAD_L = value;
    UPDATE_REG(0xB0, gba->DM0SAD_L);
    break;
  case 0xB2:
    gba->DM0SAD_H = value & 0x07FF;
    UPDATE_REG(0xB2, gba->DM0SAD_H);
    break;
  case 0xB4:
    gba->DM0DAD_L = value;
    UPDATE_REG(0xB4, gba->DM0DAD_L);
    break;
  case 0xB6:
    gba->DM0DAD_H = value & 0x07FF;
    UPDATE_REG(0xB6, gba->DM0DAD_H);
    break;
  case 0xB8:
    gba->DM0CNT_L = value & 0x3FFF;
    UPDATE_REG(0xB8, 0);
    break;
  case 0xBA:
    {
      bool start = ((gba->DM0CNT_H ^ value) & 0x8000) ? true : false;
      value &= 0xF7E0;

      gba->DM0CNT_H = value;
      UPDATE_REG(0xBA, gba->DM0CNT_H);

      if(start && (value & 0x8000)) {
        gba->dma0Source = gba->DM0SAD_L | (gba->DM0SAD_H << 16);
        gba->dma0Dest = gba->DM0DAD_L | (gba->DM0DAD_H << 16);
        CPUCheckDMA(gba, 0, 1);
      }
    }
    break;
  case 0xBC:
    gba->DM1SAD_L = value;
    UPDATE_REG(0xBC, gba->DM1SAD_L);
    break;
  case 0xBE:
    gba->DM1SAD_H = value & 0x0FFF;
    UPDATE_REG(0xBE, gba->DM1SAD_H);
    break;
  case 0xC0:
    gba->DM1DAD_L = value;
    UPDATE_REG(0xC0, gba->DM1DAD_L);
    break;
  case 0xC2:
    gba->DM1DAD_H = value & 0x07FF;
    UPDATE_REG(0xC2, gba->DM1DAD_H);
    break;
  case 0xC4:
    gba->DM1CNT_L = value & 0x3FFF;
    UPDATE_REG(0xC4, 0);
    break;
  case 0xC6:
    {
      bool start = ((gba->DM1CNT_H ^ value) & 0x8000) ? true : false;
      value &= 0xF7E0;

      gba->DM1CNT_H = value;
      UPDATE_REG(0xC6, gba->DM1CNT_H);

      if(start && (value & 0x8000)) {
        gba->dma1Source = gba->DM1SAD_L | (gba->DM1SAD_H << 16);
        gba->dma1Dest = gba->DM1DAD_L | (gba->DM1DAD_H << 16);
        CPUCheckDMA(gba, 0, 2);
      }
    }
    break;
  case 0xC8:
    gba->DM2SAD_L = value;
    UPDATE_REG(0xC8, gba->DM2SAD_L);
    break;
  case 0xCA:
    gba->DM2SAD_H = value & 0x0FFF;
    UPDATE_REG(0xCA, gba->DM2SAD_H);
    break;
  case 0xCC:
    gba->DM2DAD_L = value;
    UPDATE_REG(0xCC, gba->DM2DAD_L);
    break;
  case 0xCE:
    gba->DM2DAD_H = value & 0x07FF;
    UPDATE_REG(0xCE, gba->DM2DAD_H);
    break;
  case 0xD0:
    gba->DM2CNT_L = value & 0x3FFF;
    UPDATE_REG(0xD0, 0);
    break;
  case 0xD2:
    {
      bool start = ((gba->DM2CNT_H ^ value) & 0x8000) ? true : false;

      value &= 0xF7E0;

      gba->DM2CNT_H = value;
      UPDATE_REG(0xD2, gba->DM2CNT_H);

      if(start && (value & 0x8000)) {
        gba->dma2Source = gba->DM2SAD_L | (gba->DM2SAD_H << 16);
        gba->dma2Dest = gba->DM2DAD_L | (gba->DM2DAD_H << 16);

        CPUCheckDMA(gba, 0, 4);
      }
    }
    break;
  case 0xD4:
    gba->DM3SAD_L = value;
    UPDATE_REG(0xD4, gba->DM3SAD_L);
    break;
  case 0xD6:
    gba->DM3SAD_H = value & 0x0FFF;
    UPDATE_REG(0xD6, gba->DM3SAD_H);
    break;
  case 0xD8:
    gba->DM3DAD_L = value;
    UPDATE_REG(0xD8, gba->DM3DAD_L);
    break;
  case 0xDA:
    gba->DM3DAD_H = value & 0x0FFF;
    UPDATE_REG(0xDA, gba->DM3DAD_H);
    break;
  case 0xDC:
    gba->DM3CNT_L = value;
    UPDATE_REG(0xDC, 0);
    break;
  case 0xDE:
    {
      bool start = ((gba->DM3CNT_H ^ value) & 0x8000) ? true : false;

      value &= 0xFFE0;

      gba->DM3CNT_H = value;
      UPDATE_REG(0xDE, gba->DM3CNT_H);

      if(start && (value & 0x8000)) {
        gba->dma3Source = gba->DM3SAD_L | (gba->DM3SAD_H << 16);
        gba->dma3Dest = gba->DM3DAD_L | (gba->DM3DAD_H << 16);
        CPUCheckDMA(gba, 0, 8);
      }
    }
    break;
  case 0x100:
    gba->timer0Reload = value;
    break;
  case 0x102:
    gba->timer0Value = value;
    gba->timerOnOffDelay|=1;
    gba->cpuNextEvent = gba->cpuTotalTicks;
    break;
  case 0x104:
    gba->timer1Reload = value;
    break;
  case 0x106:
    gba->timer1Value = value;
    gba->timerOnOffDelay|=2;
    gba->cpuNextEvent = gba->cpuTotalTicks;
    break;
  case 0x108:
    gba->timer2Reload = value;
    break;
  case 0x10A:
    gba->timer2Value = value;
    gba->timerOnOffDelay|=4;
    gba->cpuNextEvent = gba->cpuTotalTicks;
    break;
  case 0x10C:
    gba->timer3Reload = value;
    break;
  case 0x10E:
    gba->timer3Value = value;
    gba->timerOnOffDelay|=8;
    gba->cpuNextEvent = gba->cpuTotalTicks;
    break;

  case 0x130:
      gba->P1 |= (value & 0x3FF);
      UPDATE_REG(0x130, gba->P1);
	  break;

  case 0x132:
	  UPDATE_REG(0x132, value & 0xC3FF);
	  break;

  case 0x200:
    gba->IE = value & 0x3FFF;
    UPDATE_REG(0x200, gba->IE);
    if ((gba->IME & 1) && (gba->IF & gba->IE) && gba->armIrqEnable)
      gba->cpuNextEvent = gba->cpuTotalTicks;
    break;
  case 0x202:
    gba->IF ^= (value & gba->IF);
    UPDATE_REG(0x202, gba->IF);
    break;
  case 0x204:
    {
      gba->memoryWait[0x0e] = gba->memoryWaitSeq[0x0e] = gamepakRamWaitState[value & 3];

      if(!gba->speedHack) {
        gba->memoryWait[0x08] = gba->memoryWait[0x09] = gamepakWaitState[(value >> 2) & 3];
        gba->memoryWaitSeq[0x08] = gba->memoryWaitSeq[0x09] =
          gamepakWaitState0[(value >> 4) & 1];

        gba->memoryWait[0x0a] = gba->memoryWait[0x0b] = gamepakWaitState[(value >> 5) & 3];
        gba->memoryWaitSeq[0x0a] = gba->memoryWaitSeq[0x0b] =
          gamepakWaitState1[(value >> 7) & 1];

        gba->memoryWait[0x0c] = gba->memoryWait[0x0d] = gamepakWaitState[(value >> 8) & 3];
        gba->memoryWaitSeq[0x0c] = gba->memoryWaitSeq[0x0d] =
          gamepakWaitState2[(value >> 10) & 1];
      } else {
        gba->memoryWait[0x08] = gba->memoryWait[0x09] = 3;
        gba->memoryWaitSeq[0x08] = gba->memoryWaitSeq[0x09] = 1;

        gba->memoryWait[0x0a] = gba->memoryWait[0x0b] = 3;
        gba->memoryWaitSeq[0x0a] = gba->memoryWaitSeq[0x0b] = 1;

        gba->memoryWait[0x0c] = gba->memoryWait[0x0d] = 3;
        gba->memoryWaitSeq[0x0c] = gba->memoryWaitSeq[0x0d] = 1;
      }

      for(int i = 8; i < 15; i++) {
        gba->memoryWait32[i] = gba->memoryWait[i] + gba->memoryWaitSeq[i] + 1;
        gba->memoryWaitSeq32[i] = gba->memoryWaitSeq[i]*2 + 1;
      }

      if((value & 0x4000) == 0x4000) {
        gba->busPrefetchEnable = true;
        gba->busPrefetch = false;
        gba->busPrefetchCount = 0;
      } else {
        gba->busPrefetchEnable = false;
        gba->busPrefetch = false;
        gba->busPrefetchCount = 0;
      }
      UPDATE_REG(0x204, value & 0x7FFF);

    }
    break;
  case 0x208:
    gba->IME = value & 1;
    UPDATE_REG(0x208, gba->IME);
    if ((gba->IME & 1) && (gba->IF & gba->IE) && gba->armIrqEnable)
      gba->cpuNextEvent = gba->cpuTotalTicks;
    break;
  case 0x300:
    if(value != 0)
      value &= 0xFFFE;
    UPDATE_REG(0x300, value);
    break;
  default:
    UPDATE_REG(address&0x3FE, value);
    break;
  }
}

void applyTimer (GBASystem *gba)
{
  if (gba->timerOnOffDelay & 1)
  {
    gba->timer0ClockReload = TIMER_TICKS[gba->timer0Value & 3];
    if(!gba->timer0On && (gba->timer0Value & 0x80)) {
      // reload the counter
      gba->TM0D = gba->timer0Reload;
      gba->timer0Ticks = (0x10000 - gba->TM0D) << gba->timer0ClockReload;
      UPDATE_REG(0x100, gba->TM0D);
    }
    gba->timer0On = gba->timer0Value & 0x80 ? true : false;
    gba->TM0CNT = gba->timer0Value & 0xC7;
    UPDATE_REG(0x102, gba->TM0CNT);
    //    CPUUpdateTicks();
  }
  if (gba->timerOnOffDelay & 2)
  {
    gba->timer1ClockReload = TIMER_TICKS[gba->timer1Value & 3];
    if(!gba->timer1On && (gba->timer1Value & 0x80)) {
      // reload the counter
      gba->TM1D = gba->timer1Reload;
      gba->timer1Ticks = (0x10000 - gba->TM1D) << gba->timer1ClockReload;
      UPDATE_REG(0x104, gba->TM1D);
    }
    gba->timer1On = gba->timer1Value & 0x80 ? true : false;
    gba->TM1CNT = gba->timer1Value & 0xC7;
    UPDATE_REG(0x106, gba->TM1CNT);
  }
  if (gba->timerOnOffDelay & 4)
  {
    gba->timer2ClockReload = TIMER_TICKS[gba->timer2Value & 3];
    if(!gba->timer2On && (gba->timer2Value & 0x80)) {
      // reload the counter
      gba->TM2D = gba->timer2Reload;
      gba->timer2Ticks = (0x10000 - gba->TM2D) << gba->timer2ClockReload;
      UPDATE_REG(0x108, gba->TM2D);
    }
    gba->timer2On = gba->timer2Value & 0x80 ? true : false;
    gba->TM2CNT = gba->timer2Value & 0xC7;
    UPDATE_REG(0x10A, gba->TM2CNT);
  }
  if (gba->timerOnOffDelay & 8)
  {
    gba->timer3ClockReload = TIMER_TICKS[gba->timer3Value & 3];
    if(!gba->timer3On && (gba->timer3Value & 0x80)) {
      // reload the counter
      gba->TM3D = gba->timer3Reload;
      gba->timer3Ticks = (0x10000 - gba->TM3D) << gba->timer3ClockReload;
      UPDATE_REG(0x10C, gba->TM3D);
    }
    gba->timer3On = gba->timer3Value & 0x80 ? true : false;
    gba->TM3CNT = gba->timer3Value & 0xC7;
    UPDATE_REG(0x10E, gba->TM3CNT);
  }
  gba->cpuNextEvent = CPUUpdateTicks(gba);
  gba->timerOnOffDelay = 0;
}

void CPUInit(GBASystem *gba)
{
  gba->gbaSaveType = 0;
  gba->eepromInUse = 0;
  gba->saveType = 0;
  gba->useBios = false;

  if(!gba->useBios) {
    memcpy(gba->bios, myROM, sizeof(myROM));
  }

  int i = 0;

  gba->biosProtected[0] = 0x00;
  gba->biosProtected[1] = 0xf0;
  gba->biosProtected[2] = 0x29;
  gba->biosProtected[3] = 0xe1;

  for(i = 0; i < 256; i++) {
    int count = 0;
    int j;
    for(j = 0; j < 8; j++)
      if(i & (1 << j))
        count++;
    gba->cpuBitsSet[i] = count;

    for(j = 0; j < 8; j++)
      if(i & (1 << j))
        break;
    gba->cpuLowestBitSet[i] = j;
  }

  for(i = 0; i < 0x400; i++)
    gba->ioReadable[i] = true;
  for(i = 0x10; i < 0x48; i++)
    gba->ioReadable[i] = false;
  for(i = 0x4c; i < 0x50; i++)
    gba->ioReadable[i] = false;
  for(i = 0x54; i < 0x60; i++)
    gba->ioReadable[i] = false;
  for(i = 0x8c; i < 0x90; i++)
    gba->ioReadable[i] = false;
  for(i = 0xa0; i < 0xb8; i++)
    gba->ioReadable[i] = false;
  for(i = 0xbc; i < 0xc4; i++)
    gba->ioReadable[i] = false;
  for(i = 0xc8; i < 0xd0; i++)
    gba->ioReadable[i] = false;
  for(i = 0xd4; i < 0xdc; i++)
    gba->ioReadable[i] = false;
  for(i = 0xe0; i < 0x100; i++)
    gba->ioReadable[i] = false;
  for(i = 0x110; i < 0x120; i++)
    gba->ioReadable[i] = false;
  for(i = 0x12c; i < 0x130; i++)
    gba->ioReadable[i] = false;
  for(i = 0x138; i < 0x140; i++)
    gba->ioReadable[i] = false;
  for(i = 0x144; i < 0x150; i++)
    gba->ioReadable[i] = false;
  for(i = 0x15c; i < 0x200; i++)
    gba->ioReadable[i] = false;
  for(i = 0x20c; i < 0x300; i++)
    gba->ioReadable[i] = false;
  for(i = 0x304; i < 0x400; i++)
    gba->ioReadable[i] = false;

  if(gba->romSize < 0x1fe2000) {
    *((u16 *)&gba->rom[0x1fe209c]) = 0xdffa; // SWI 0xFA
    *((u16 *)&gba->rom[0x1fe209e]) = 0x4770; // BX LR
  } else {
  }
}

void CPUReset(GBASystem *gba)
{
  // clean registers
  memset(&gba->reg[0], 0, sizeof(gba->reg));
  // clean OAM
  memset(gba->oam, 0, 0x400);
  // clean palette
  memset(gba->paletteRAM, 0, 0x400);
  // clean vram
  memset(gba->vram, 0, 0x20000);
  // clean io memory
  memset(gba->ioMem, 0, 0x400);

  gba->DISPCNT  = 0x0080;
  gba->DISPSTAT = 0x0000;
  gba->VCOUNT   = (gba->useBios && !gba->skipBios) ? 0 :0x007E;
  gba->BG0CNT   = 0x0000;
  gba->BG1CNT   = 0x0000;
  gba->BG2CNT   = 0x0000;
  gba->BG3CNT   = 0x0000;
  gba->BG0HOFS  = 0x0000;
  gba->BG0VOFS  = 0x0000;
  gba->BG1HOFS  = 0x0000;
  gba->BG1VOFS  = 0x0000;
  gba->BG2HOFS  = 0x0000;
  gba->BG2VOFS  = 0x0000;
  gba->BG3HOFS  = 0x0000;
  gba->BG3VOFS  = 0x0000;
  gba->BG2PA    = 0x0100;
  gba->BG2PB    = 0x0000;
  gba->BG2PC    = 0x0000;
  gba->BG2PD    = 0x0100;
  gba->BG2X_L   = 0x0000;
  gba->BG2X_H   = 0x0000;
  gba->BG2Y_L   = 0x0000;
  gba->BG2Y_H   = 0x0000;
  gba->BG3PA    = 0x0100;
  gba->BG3PB    = 0x0000;
  gba->BG3PC    = 0x0000;
  gba->BG3PD    = 0x0100;
  gba->BG3X_L   = 0x0000;
  gba->BG3X_H   = 0x0000;
  gba->BG3Y_L   = 0x0000;
  gba->BG3Y_H   = 0x0000;
  gba->WIN0H    = 0x0000;
  gba->WIN1H    = 0x0000;
  gba->WIN0V    = 0x0000;
  gba->WIN1V    = 0x0000;
  gba->WININ    = 0x0000;
  gba->WINOUT   = 0x0000;
  gba->MOSAIC   = 0x0000;
  gba->BLDMOD   = 0x0000;
  gba->COLEV    = 0x0000;
  gba->COLY     = 0x0000;
  gba->DM0SAD_L = 0x0000;
  gba->DM0SAD_H = 0x0000;
  gba->DM0DAD_L = 0x0000;
  gba->DM0DAD_H = 0x0000;
  gba->DM0CNT_L = 0x0000;
  gba->DM0CNT_H = 0x0000;
  gba->DM1SAD_L = 0x0000;
  gba->DM1SAD_H = 0x0000;
  gba->DM1DAD_L = 0x0000;
  gba->DM1DAD_H = 0x0000;
  gba->DM1CNT_L = 0x0000;
  gba->DM1CNT_H = 0x0000;
  gba->DM2SAD_L = 0x0000;
  gba->DM2SAD_H = 0x0000;
  gba->DM2DAD_L = 0x0000;
  gba->DM2DAD_H = 0x0000;
  gba->DM2CNT_L = 0x0000;
  gba->DM2CNT_H = 0x0000;
  gba->DM3SAD_L = 0x0000;
  gba->DM3SAD_H = 0x0000;
  gba->DM3DAD_L = 0x0000;
  gba->DM3DAD_H = 0x0000;
  gba->DM3CNT_L = 0x0000;
  gba->DM3CNT_H = 0x0000;
  gba->TM0D     = 0x0000;
  gba->TM0CNT   = 0x0000;
  gba->TM1D     = 0x0000;
  gba->TM1CNT   = 0x0000;
  gba->TM2D     = 0x0000;
  gba->TM2CNT   = 0x0000;
  gba->TM3D     = 0x0000;
  gba->TM3CNT   = 0x0000;
  gba->P1       = 0x03FF;
  gba->IE       = 0x0000;
  gba->IF       = 0x0000;
  gba->IME      = 0x0000;

  gba->armMode = 0x1F;

  if(gba->cpuIsMultiBoot) {
    gba->reg[13].I = 0x03007F00;
    gba->reg[15].I = 0x02000000;
    gba->reg[16].I = 0x00000000;
    gba->reg[R13_IRQ].I = 0x03007FA0;
    gba->reg[R13_SVC].I = 0x03007FE0;
    gba->armIrqEnable = true;
  } else {
    if(gba->useBios && !gba->skipBios) {
      gba->reg[15].I = 0x00000000;
      gba->armMode = 0x13;
      gba->armIrqEnable = false;
    } else {
      gba->reg[13].I = 0x03007F00;
      gba->reg[15].I = 0x08000000;
      gba->reg[16].I = 0x00000000;
      gba->reg[R13_IRQ].I = 0x03007FA0;
      gba->reg[R13_SVC].I = 0x03007FE0;
      gba->armIrqEnable = true;
    }
  }
  gba->armState = true;
  gba->C_FLAG = gba->V_FLAG = gba->N_FLAG = gba->Z_FLAG = false;
  UPDATE_REG(0x00, gba->DISPCNT);
  UPDATE_REG(0x06, gba->VCOUNT);
  UPDATE_REG(0x20, gba->BG2PA);
  UPDATE_REG(0x26, gba->BG2PD);
  UPDATE_REG(0x30, gba->BG3PA);
  UPDATE_REG(0x36, gba->BG3PD);
  UPDATE_REG(0x130, gba->P1);
  UPDATE_REG(0x88, 0x200);

  // disable FIQ
  gba->reg[16].I |= 0x40;

  CPUUpdateCPSR(gba);

  gba->armNextPC = gba->reg[15].I;
  gba->reg[15].I += 4;

  // reset internal state
  gba->holdState = false;
  gba->holdType = 0;

  gba->biosProtected[0] = 0x00;
  gba->biosProtected[1] = 0xf0;
  gba->biosProtected[2] = 0x29;
  gba->biosProtected[3] = 0xe1;

  gba->lcdTicks = (gba->useBios && !gba->skipBios) ? 1008 : 208;
  gba->timer0On = false;
  gba->timer0Ticks = 0;
  gba->timer0Reload = 0;
  gba->timer0ClockReload  = 0;
  gba->timer1On = false;
  gba->timer1Ticks = 0;
  gba->timer1Reload = 0;
  gba->timer1ClockReload  = 0;
  gba->timer2On = false;
  gba->timer2Ticks = 0;
  gba->timer2Reload = 0;
  gba->timer2ClockReload  = 0;
  gba->timer3On = false;
  gba->timer3Ticks = 0;
  gba->timer3Reload = 0;
  gba->timer3ClockReload  = 0;
  gba->dma0Source = 0;
  gba->dma0Dest = 0;
  gba->dma1Source = 0;
  gba->dma1Dest = 0;
  gba->dma2Source = 0;
  gba->dma2Dest = 0;
  gba->dma3Source = 0;
  gba->dma3Dest = 0;
  gba->fxOn = false;
  gba->windowOn = false;
  gba->frameCount = 0;
  gba->saveType = 0;
  gba->layerEnable = gba->DISPCNT & gba->layerSettings;

  for(int i = 0; i < 256; i++) {
    gba->map[i].address = (u8 *)&gba->dummyAddress;
    gba->map[i].mask = 0;
  }

  gba->map[0].address = gba->bios;
  gba->map[0].mask = 0x3FFF;
  gba->map[2].address = gba->workRAM;
  gba->map[2].mask = 0x3FFFF;
  gba->map[3].address = gba->internalRAM;
  gba->map[3].mask = 0x7FFF;
  gba->map[4].address = gba->ioMem;
  gba->map[4].mask = 0x3FF;
  gba->map[5].address = gba->paletteRAM;
  gba->map[5].mask = 0x3FF;
  gba->map[6].address = gba->vram;
  gba->map[6].mask = 0x1FFFF;
  gba->map[7].address = gba->oam;
  gba->map[7].mask = 0x3FF;
  gba->map[8].address = gba->rom;
  gba->map[8].mask = 0x1FFFFFF;
  gba->map[9].address = gba->rom;
  gba->map[9].mask = 0x1FFFFFF;
  gba->map[10].address = gba->rom;
  gba->map[10].mask = 0x1FFFFFF;
  gba->map[12].address = gba->rom;
  gba->map[12].mask = 0x1FFFFFF;
  soundReset(gba);

  // make sure registers are correctly initialized if not using BIOS
  if(!gba->useBios) {
    if(gba->cpuIsMultiBoot)
      BIOS_RegisterRamReset(gba, 0xfe);
    else
      BIOS_RegisterRamReset(gba, 0xff);
  } else {
    if(gba->cpuIsMultiBoot)
      BIOS_RegisterRamReset(gba, 0xfe);
  }

  ARM_PREFETCH;

  gba->cpuDmaHack = false;

  gba->SWITicks = 0;
}

void CPUInterrupt(GBASystem *gba)
{
  u32 PC = gba->reg[15].I;
  bool savedState = gba->armState;
  CPUSwitchMode(gba, 0x12, true, false);
  gba->reg[14].I = PC;
  if(!savedState)
    gba->reg[14].I += 2;
  gba->reg[15].I = 0x18;
  gba->armState = true;
  gba->armIrqEnable = false;

  gba->armNextPC = gba->reg[15].I;
  gba->reg[15].I += 4;
  ARM_PREFETCH;

  //  if(!holdState)
  gba->biosProtected[0] = 0x02;
  gba->biosProtected[1] = 0xc0;
  gba->biosProtected[2] = 0x5e;
  gba->biosProtected[3] = 0xe5;
}

void CPULoop(GBASystem *gba, int ticks)
{
  int clockTicks;
  int timerOverflow = 0;
  // variable used by the CPU core
  gba->cpuTotalTicks = 0;

  gba->cpuBreakLoop = false;
  gba->cpuNextEvent = CPUUpdateTicks(gba);
  if(gba->cpuNextEvent > ticks)
    gba->cpuNextEvent = ticks;


  for(;;) {
    if(!gba->holdState && !gba->SWITicks) {
      if(gba->armState) {
        if (!armExecute(gba))
          return;
      } else {
        if (!thumbExecute(gba))
          return;
      }
      clockTicks = 0;
    } else
      clockTicks = CPUUpdateTicks(gba);

    gba->cpuTotalTicks += clockTicks;


    if(gba->cpuTotalTicks >= gba->cpuNextEvent) {
      int remainingTicks = gba->cpuTotalTicks - gba->cpuNextEvent;

      if (gba->SWITicks)
      {
        gba->SWITicks-=clockTicks;
        if (gba->SWITicks<0)
          gba->SWITicks = 0;
      }

      clockTicks = gba->cpuNextEvent;
      gba->cpuTotalTicks = 0;
      gba->cpuDmaHack = false;

    updateLoop:

      if (gba->IRQTicks)
      {
          gba->IRQTicks -= clockTicks;
        if (gba->IRQTicks<0)
          gba->IRQTicks = 0;
      }

      gba->lcdTicks -= clockTicks;


      if(gba->lcdTicks <= 0) {
        if(gba->DISPSTAT & 1) { // V-BLANK
          // if in V-Blank mode, keep computing...
          if(gba->DISPSTAT & 2) {
            gba->lcdTicks += 1008;
            gba->VCOUNT++;
            UPDATE_REG(0x06, gba->VCOUNT);
            gba->DISPSTAT &= 0xFFFD;
            UPDATE_REG(0x04, gba->DISPSTAT);
            CPUCompareVCOUNT(gba);
          } else {
            gba->lcdTicks += 224;
            gba->DISPSTAT |= 2;
            UPDATE_REG(0x04, gba->DISPSTAT);
            if(gba->DISPSTAT & 16) {
              gba->IF |= 2;
              UPDATE_REG(0x202, gba->IF);
            }
          }

          if(gba->VCOUNT >= 228) { //Reaching last line
            gba->DISPSTAT &= 0xFFFC;
            UPDATE_REG(0x04, gba->DISPSTAT);
            gba->VCOUNT = 0;
            UPDATE_REG(0x06, gba->VCOUNT);
            CPUCompareVCOUNT(gba);
          }
        } else {
          if(gba->DISPSTAT & 2) {
            // if in H-Blank, leave it and move to drawing mode
            gba->VCOUNT++;
            UPDATE_REG(0x06, gba->VCOUNT);

            gba->lcdTicks += 1008;
            gba->DISPSTAT &= 0xFFFD;
            if(gba->VCOUNT == 160) {
              gba->count++;
              if(gba->count == 60) {
                gba->count = 0;
              }

              gba->DISPSTAT |= 1;
              gba->DISPSTAT &= 0xFFFD;
              UPDATE_REG(0x04, gba->DISPSTAT);
              if(gba->DISPSTAT & 0x0008) {
                gba->IF |= 1;
                UPDATE_REG(0x202, gba->IF);
              }
              CPUCheckDMA(gba, 1, 0x0f);
              gba->frameCount++;
            }

            UPDATE_REG(0x04, gba->DISPSTAT);
            CPUCompareVCOUNT(gba);

          } else {
            // entering H-Blank
            gba->DISPSTAT |= 2;
            UPDATE_REG(0x04, gba->DISPSTAT);
            gba->lcdTicks += 224;
            CPUCheckDMA(gba, 2, 0x0f);
            if(gba->DISPSTAT & 16) {
              gba->IF |= 2;
              UPDATE_REG(0x202, gba->IF);
            }
          }
        }
      }

	    // we shouldn't be doing sound in stop state, but we loose synchronization
      // if sound is disabled, so in stop state, soundTick will just produce
      // mute sound
      gba->soundTicks -= clockTicks;
      if(gba->soundTicks <= 0) {
        psoundTickfn(gba);
        gba->soundTicks += gba->SOUND_CLOCK_TICKS;
      }

      if(!gba->stopState) {
        if(gba->timer0On) {
          gba->timer0Ticks -= clockTicks;
          if(gba->timer0Ticks <= 0) {
            gba->timer0Ticks += (0x10000 - gba->timer0Reload) << gba->timer0ClockReload;
            timerOverflow |= 1;
            soundTimerOverflow(gba, 0);
            if(gba->TM0CNT & 0x40) {
              gba->IF |= 0x08;
              UPDATE_REG(0x202, gba->IF);
            }
          }
          gba->TM0D = 0xFFFF - (gba->timer0Ticks >> gba->timer0ClockReload);
          UPDATE_REG(0x100, gba->TM0D);
        }

        if(gba->timer1On) {
          if(gba->TM1CNT & 4) {
            if(timerOverflow & 1) {
              gba->TM1D++;
              if(gba->TM1D == 0) {
                gba->TM1D += gba->timer1Reload;
                timerOverflow |= 2;
                soundTimerOverflow(gba, 1);
                if(gba->TM1CNT & 0x40) {
                  gba->IF |= 0x10;
                  UPDATE_REG(0x202, gba->IF);
                }
              }
              UPDATE_REG(0x104, gba->TM1D);
            }
          } else {
            gba->timer1Ticks -= clockTicks;
            if(gba->timer1Ticks <= 0) {
              gba->timer1Ticks += (0x10000 - gba->timer1Reload) << gba->timer1ClockReload;
              timerOverflow |= 2;
              soundTimerOverflow(gba, 1);
              if(gba->TM1CNT & 0x40) {
                gba->IF |= 0x10;
                UPDATE_REG(0x202, gba->IF);
              }
            }
            gba->TM1D = 0xFFFF - (gba->timer1Ticks >> gba->timer1ClockReload);
            UPDATE_REG(0x104, gba->TM1D);
          }
        }

        if(gba->timer2On) {
          if(gba->TM2CNT & 4) {
            if(timerOverflow & 2) {
              gba->TM2D++;
              if(gba->TM2D == 0) {
                gba->TM2D += gba->timer2Reload;
                timerOverflow |= 4;
                if(gba->TM2CNT & 0x40) {
                  gba->IF |= 0x20;
                  UPDATE_REG(0x202, gba->IF);
                }
              }
              UPDATE_REG(0x108, gba->TM2D);
            }
          } else {
            gba->timer2Ticks -= clockTicks;
            if(gba->timer2Ticks <= 0) {
              gba->timer2Ticks += (0x10000 - gba->timer2Reload) << gba->timer2ClockReload;
              timerOverflow |= 4;
              if(gba->TM2CNT & 0x40) {
                gba->IF |= 0x20;
                UPDATE_REG(0x202, gba->IF);
              }
            }
            gba->TM2D = 0xFFFF - (gba->timer2Ticks >> gba->timer2ClockReload);
            UPDATE_REG(0x108, gba->TM2D);
          }
        }

        if(gba->timer3On) {
          if(gba->TM3CNT & 4) {
            if(timerOverflow & 4) {
              gba->TM3D++;
              if(gba->TM3D == 0) {
                gba->TM3D += gba->timer3Reload;
                if(gba->TM3CNT & 0x40) {
                  gba->IF |= 0x40;
                  UPDATE_REG(0x202, gba->IF);
                }
              }
              UPDATE_REG(0x10C, gba->TM3D);
            }
          } else {
              gba->timer3Ticks -= clockTicks;
            if(gba->timer3Ticks <= 0) {
              gba->timer3Ticks += (0x10000 - gba->timer3Reload) << gba->timer3ClockReload;
              if(gba->TM3CNT & 0x40) {
                gba->IF |= 0x40;
                UPDATE_REG(0x202, gba->IF);
              }
            }
            gba->TM3D = 0xFFFF - (gba->timer3Ticks >> gba->timer3ClockReload);
            UPDATE_REG(0x10C, gba->TM3D);
          }
        }
      }

      timerOverflow = 0;



      ticks -= clockTicks;

      gba->cpuNextEvent = CPUUpdateTicks(gba);

      if(gba->cpuDmaTicksToUpdate > 0) {
        if(gba->cpuDmaTicksToUpdate > gba->cpuNextEvent)
          clockTicks = gba->cpuNextEvent;
        else
          clockTicks = gba->cpuDmaTicksToUpdate;
        gba->cpuDmaTicksToUpdate -= clockTicks;
        if(gba->cpuDmaTicksToUpdate < 0)
          gba->cpuDmaTicksToUpdate = 0;
        gba->cpuDmaHack = true;
        goto updateLoop;
      }

      if(gba->IF && (gba->IME & 1) && gba->armIrqEnable) {
        int res = gba->IF & gba->IE;
        if(gba->stopState)
          res &= 0x3080;
        if(res) {
          if (gba->intState)
          {
            if (!gba->IRQTicks)
            {
              CPUInterrupt(gba);
              gba->intState = false;
              gba->holdState = false;
              gba->stopState = false;
              gba->holdType = 0;
            }
          }
          else
          {
            if (!gba->holdState)
            {
              gba->intState = true;
              gba->IRQTicks=7;
              if (gba->cpuNextEvent> gba->IRQTicks)
                gba->cpuNextEvent = gba->IRQTicks;
            }
            else
            {
              CPUInterrupt(gba);
              gba->holdState = false;
              gba->stopState = false;
              gba->holdType = 0;
            }
          }

          // Stops the SWI Ticks emulation if an IRQ is executed
          //(to avoid problems with nested IRQ/SWI)
          if (gba->SWITicks)
            gba->SWITicks = 0;
        }
      }

      if(remainingTicks > 0) {
        if(remainingTicks > gba->cpuNextEvent)
          clockTicks = gba->cpuNextEvent;
        else
          clockTicks = remainingTicks;
        remainingTicks -= clockTicks;
        if(remainingTicks < 0)
          remainingTicks = 0;
        goto updateLoop;
      }

      if (gba->timerOnOffDelay)
          applyTimer(gba);

      if(gba->cpuNextEvent > ticks)
        gba->cpuNextEvent = ticks;

      if(ticks <= 0 || gba->cpuBreakLoop)
        break;

    }
  }
}
