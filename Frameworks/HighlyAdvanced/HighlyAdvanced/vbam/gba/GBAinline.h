#ifndef GBAINLINE_H
#define GBAINLINE_H

#include "../common/Port.h"
#include "../common/Types.h"
#include "Sound.h"

#define eepromRead(a) 0
#define eepromWrite(a, v) 1
#define flashRead(a) 0
#define rtcRead(a) 0
#define systemGetSensorX() 0
#define systemGetSensorY() 0
#define agbPrintWrite(a, v) 1
#define rtcWrite(a, v) 1

extern const u32 objTilesAddress[3];

#define CPUReadByteQuick(gba, addr) \
  (gba)->map[(addr)>>24].address[(addr) & (gba)->map[(addr)>>24].mask]

#define CPUReadHalfWordQuick(gba, addr) \
  READ16LE(((u16*)&(gba)->map[(addr)>>24].address[(addr) & (gba)->map[(addr)>>24].mask]))

#define CPUReadMemoryQuick(gba, addr) \
  READ32LE(((u32*)&(gba)->map[(addr)>>24].address[(addr) & (gba)->map[(addr)>>24].mask]))

static inline u32 CPUReadMemory(GBASystem *gba, u32 address)
{
  u32 value;
  switch(address >> 24) {
  case 0:
    if(gba->reg[15].I >> 24) {
      if(address < 0x4000) {
        value = READ32LE(((u32 *)&gba->biosProtected));
      }
      else goto unreadable;
    } else
      value = READ32LE(((u32 *)&gba->bios[address & 0x3FFC]));
    break;
  case 2:
    value = READ32LE(((u32 *)&gba->workRAM[address & 0x3FFFC]));
    break;
  case 3:
    value = READ32LE(((u32 *)&gba->internalRAM[address & 0x7ffC]));
    break;
  case 4:
      if((address < 0x4000400) && gba->ioReadable[address & 0x3fc]) {
          if(gba->ioReadable[(address & 0x3fc) + 2]) {
              value = READ32LE(((u32 *)&gba->ioMem[address & 0x3fC]));
		  } else {
              value = READ16LE(((u16 *)&gba->ioMem[address & 0x3fc]));
		  }
	  }
	  else
		  goto unreadable;
	  break;
  case 5:
    value = READ32LE(((u32 *)&gba->paletteRAM[address & 0x3fC]));
    break;
  case 6:
    address = (address & 0x1fffc);
    if (((gba->DISPCNT & 7) >2) && ((address & 0x1C000) == 0x18000))
    {
      value = 0;
      break;
    }
    if ((address & 0x18000) == 0x18000)
      address &= 0x17fff;
    value = READ32LE(((u32 *)&gba->vram[address]));
    break;
  case 7:
    value = READ32LE(((u32 *)&gba->oam[address & 0x3FC]));
    break;
  case 8:
  case 9:
  case 10:
  case 11:
  case 12:
    value = READ32LE(((u32 *)&gba->rom[address&0x1FFFFFC]));
    break;
  case 13:
    if(gba->cpuEEPROMEnabled)
      // no need to swap this
      return eepromRead(address);
    goto unreadable;
  case 14:
    if(gba->cpuFlashEnabled | gba->cpuSramEnabled)
      // no need to swap this
      return flashRead(address);
    // default
  default:
unreadable:

    if(gba->cpuDmaHack) {
      value = gba->cpuDmaLast;
    } else {
      if(gba->armState) {
        value = CPUReadMemoryQuick(gba, gba->reg[15].I);
      } else {
        value = CPUReadHalfWordQuick(gba, gba->reg[15].I) |
          CPUReadHalfWordQuick(gba, gba->reg[15].I) << 16;
      }
    }
  }

  if(address & 3) {
    int shift = (address & 3) << 3;
    value = (value >> shift) | (value << (32 - shift));
  }
  return value;
}

extern const u32 myROM[];

static inline u32 CPUReadHalfWord(GBASystem *gba, u32 address)
{
  u32 value;

  switch(address >> 24) {
  case 0:
    if (gba->reg[15].I >> 24) {
      if(address < 0x4000) {
        value = READ16LE(((u16 *)&gba->biosProtected[address&2]));
      } else goto unreadable;
    } else
      value = READ16LE(((u16 *)&gba->bios[address & 0x3FFE]));
    break;
  case 2:
    value = READ16LE(((u16 *)&gba->workRAM[address & 0x3FFFE]));
    break;
  case 3:
    value = READ16LE(((u16 *)&gba->internalRAM[address & 0x7ffe]));
    break;
  case 4:
    if((address < 0x4000400) && gba->ioReadable[address & 0x3fe])
    {
      value =  READ16LE(((u16 *)&gba->ioMem[address & 0x3fe]));
      if (((address & 0x3fe)>0xFF) && ((address & 0x3fe)<0x10E))
      {
        if (((address & 0x3fe) == 0x100) && gba->timer0On)
          value = 0xFFFF - ((gba->timer0Ticks-gba->cpuTotalTicks) >> gba->timer0ClockReload);
        else
          if (((address & 0x3fe) == 0x104) && gba->timer1On && !(gba->TM1CNT & 4))
            value = 0xFFFF - ((gba->timer1Ticks-gba->cpuTotalTicks) >> gba->timer1ClockReload);
          else
            if (((address & 0x3fe) == 0x108) && gba->timer2On && !(gba->TM2CNT & 4))
              value = 0xFFFF - ((gba->timer2Ticks-gba->cpuTotalTicks) >> gba->timer2ClockReload);
            else
              if (((address & 0x3fe) == 0x10C) && gba->timer3On && !(gba->TM3CNT & 4))
                value = 0xFFFF - ((gba->timer3Ticks-gba->cpuTotalTicks) >> gba->timer3ClockReload);
      }
    }
    else goto unreadable;
    break;
  case 5:
    value = READ16LE(((u16 *)&gba->paletteRAM[address & 0x3fe]));
    break;
  case 6:
    address = (address & 0x1fffe);
    if (((gba->DISPCNT & 7) >2) && ((address & 0x1C000) == 0x18000))
    {
      value = 0;
      break;
    }
    if ((address & 0x18000) == 0x18000)
      address &= 0x17fff;
    value = READ16LE(((u16 *)&gba->vram[address]));
    break;
  case 7:
    value = READ16LE(((u16 *)&gba->oam[address & 0x3fe]));
    break;
  case 8:
  case 9:
  case 10:
  case 11:
  case 12:
    if(address == 0x80000c4 || address == 0x80000c6 || address == 0x80000c8)
      value = rtcRead(address);
    else
      value = READ16LE(((u16 *)&gba->rom[address & 0x1FFFFFE]));
    break;
  case 13:
    if(gba->cpuEEPROMEnabled)
      // no need to swap this
      return  eepromRead(address);
    goto unreadable;
  case 14:
    if(gba->cpuFlashEnabled | gba->cpuSramEnabled)
      // no need to swap this
      return flashRead(address);
    // default
  default:
unreadable:
    if(gba->cpuDmaHack) {
      value = gba->cpuDmaLast & 0xFFFF;
    } else {
      if(gba->armState) {
        value = CPUReadHalfWordQuick(gba, gba->reg[15].I + (address & 2));
      } else {
        value = CPUReadHalfWordQuick(gba, gba->reg[15].I);
      }
    }
    break;
  }

  if(address & 1) {
    value = (value >> 8) | (value << 24);
  }

  return value;
}

static inline u16 CPUReadHalfWordSigned(GBASystem *gba, u32 address)
{
  u16 value = CPUReadHalfWord(gba, address);
  if((address & 1))
    value = (s8)value;
  return value;
}

static inline u8 CPUReadByte(GBASystem *gba, u32 address)
{
  switch(address >> 24) {
  case 0:
    if (gba->reg[15].I >> 24) {
      if(address < 0x4000) {
        return gba->biosProtected[address & 3];
      } else goto unreadable;
    }
    return gba->bios[address & 0x3FFF];
  case 2:
    return gba->workRAM[address & 0x3FFFF];
  case 3:
    return gba->internalRAM[address & 0x7fff];
  case 4:
    if((address < 0x4000400) && gba->ioReadable[address & 0x3ff])
      return gba->ioMem[address & 0x3ff];
    else goto unreadable;
  case 5:
    return gba->paletteRAM[address & 0x3ff];
  case 6:
    address = (address & 0x1ffff);
    if (((gba->DISPCNT & 7) >2) && ((address & 0x1C000) == 0x18000))
      return 0;
    if ((address & 0x18000) == 0x18000)
      address &= 0x17fff;
    return gba->vram[address];
  case 7:
    return gba->oam[address & 0x3ff];
  case 8:
  case 9:
  case 10:
  case 11:
  case 12:
    return gba->rom[address & 0x1FFFFFF];
  case 13:
    if(gba->cpuEEPROMEnabled)
      return eepromRead(address);
    goto unreadable;
  case 14:
    if(gba->cpuSramEnabled | gba->cpuFlashEnabled)
      return flashRead(address);
    if(gba->cpuEEPROMSensorEnabled) {
      switch(address & 0x00008f00) {
  case 0x8200:
    return systemGetSensorX() & 255;
  case 0x8300:
    return (systemGetSensorX() >> 8)|0x80;
  case 0x8400:
    return systemGetSensorY() & 255;
  case 0x8500:
    return systemGetSensorY() >> 8;
      }
    }
    // default
  default:
unreadable:
    if(gba->cpuDmaHack) {
      return gba->cpuDmaLast & 0xFF;
    } else {
      if(gba->armState) {
        return CPUReadByteQuick(gba, gba->reg[15].I+(address & 3));
      } else {
        return CPUReadByteQuick(gba, gba->reg[15].I+(address & 1));
      }
    }
    break;
  }
}

static inline void CPUWriteMemory(GBASystem *gba, u32 address, u32 value)
{
  switch(address >> 24) {
  case 0x02:
      WRITE32LE(((u32 *)&gba->workRAM[address & 0x3FFFC]), value);
    break;
  case 0x03:
      WRITE32LE(((u32 *)&gba->internalRAM[address & 0x7ffC]), value);
    break;
  case 0x04:
    if(address < 0x4000400) {
      CPUUpdateRegister(gba, (address & 0x3FC), value & 0xFFFF);
      CPUUpdateRegister(gba, (address & 0x3FC) + 2, (value >> 16));
    } else goto unwritable;
    break;
  case 0x05:
      WRITE32LE(((u32 *)&gba->paletteRAM[address & 0x3FC]), value);
    break;
  case 0x06:
    address = (address & 0x1fffc);
    if (((gba->DISPCNT & 7) >2) && ((address & 0x1C000) == 0x18000))
      return;
    if ((address & 0x18000) == 0x18000)
      address &= 0x17fff;

      WRITE32LE(((u32 *)&gba->vram[address]), value);
    break;
  case 0x07:
      WRITE32LE(((u32 *)&gba->oam[address & 0x3fc]), value);
    break;
  case 0x0D:
    if(gba->cpuEEPROMEnabled) {
      eepromWrite(address, value);
      break;
    }
    goto unwritable;
  case 0x0E:
    // default
  default:
unwritable:
    break;
  }
}

static inline void CPUWriteHalfWord(GBASystem *gba, u32 address, u16 value)
{
  switch(address >> 24) {
  case 2:
      WRITE16LE(((u16 *)&gba->workRAM[address & 0x3FFFE]),value);
    break;
  case 3:
      WRITE16LE(((u16 *)&gba->internalRAM[address & 0x7ffe]), value);
    break;
  case 4:
    if(address < 0x4000400)
      CPUUpdateRegister(gba, address & 0x3fe, value);
    else goto unwritable;
    break;
  case 5:
      WRITE16LE(((u16 *)&gba->paletteRAM[address & 0x3fe]), value);
    break;
  case 6:
    address = (address & 0x1fffe);
    if (((gba->DISPCNT & 7) >2) && ((address & 0x1C000) == 0x18000))
      return;
    if ((address & 0x18000) == 0x18000)
      address &= 0x17fff;
      WRITE16LE(((u16 *)&gba->vram[address]), value);
    break;
  case 7:
      WRITE16LE(((u16 *)&gba->oam[address & 0x3fe]), value);
    break;
  case 8:
  case 9:
    if(address == 0x80000c4 || address == 0x80000c6 || address == 0x80000c8) {
      if(!rtcWrite(address, value))
        goto unwritable;
    } else if(!agbPrintWrite(address, value)) goto unwritable;
    break;
  case 13:
    if(gba->cpuEEPROMEnabled) {
      eepromWrite(address, (u8)value);
      break;
    }
    goto unwritable;
  case 14:
    goto unwritable;
  default:
unwritable:
    break;
  }
}

static inline void CPUWriteByte(GBASystem *gba, u32 address, u8 b)
{
  switch(address >> 24) {
  case 2:
      gba->workRAM[address & 0x3FFFF] = b;
    break;
  case 3:
      gba->internalRAM[address & 0x7fff] = b;
    break;
  case 4:
    if(address < 0x4000400) {
      switch(address & 0x3FF) {
      case 0x60:
      case 0x61:
      case 0x62:
      case 0x63:
      case 0x64:
      case 0x65:
      case 0x68:
      case 0x69:
      case 0x6c:
      case 0x6d:
      case 0x70:
      case 0x71:
      case 0x72:
      case 0x73:
      case 0x74:
      case 0x75:
      case 0x78:
      case 0x79:
      case 0x7c:
      case 0x7d:
      case 0x80:
      case 0x81:
      case 0x84:
      case 0x85:
      case 0x90:
      case 0x91:
      case 0x92:
      case 0x93:
      case 0x94:
      case 0x95:
      case 0x96:
      case 0x97:
      case 0x98:
      case 0x99:
      case 0x9a:
      case 0x9b:
      case 0x9c:
      case 0x9d:
      case 0x9e:
      case 0x9f:
        soundEvent(gba, address&0xFF, b);
        break;
      case 0x301: // HALTCNT, undocumented
        if(b == 0x80)
          gba->stopState = true;
        gba->holdState = 1;
        gba->holdType = -1;
        gba->cpuNextEvent = gba->cpuTotalTicks;
        break;
      default: // every other register
        u32 lowerBits = address & 0x3fe;
        if(address & 1) {
          CPUUpdateRegister(gba, lowerBits, (READ16LE(&gba->ioMem[lowerBits]) & 0x00FF) | (b << 8));
        } else {
          CPUUpdateRegister(gba, lowerBits, (READ16LE(&gba->ioMem[lowerBits]) & 0xFF00) | b);
        }
      }
      break;
    } else goto unwritable;
    break;
  case 5:
    // no need to switch
    *((u16 *)&gba->paletteRAM[address & 0x3FE]) = (b << 8) | b;
    break;
  case 6:
    address = (address & 0x1fffe);
    if (((gba->DISPCNT & 7) >2) && ((address & 0x1C000) == 0x18000))
      return;
    if ((address & 0x18000) == 0x18000)
      address &= 0x17fff;

    // no need to switch
    // byte writes to OBJ VRAM are ignored
    if ((address) < objTilesAddress[((gba->DISPCNT&7)+1)>>2])
    {
        *((u16 *)&gba->vram[address]) = (b << 8) | b;
    }
    break;
  case 7:
    // no need to switch
    // byte writes to OAM are ignored
    //    *((u16 *)&oam[address & 0x3FE]) = (b << 8) | b;
    break;
  case 13:
    if(gba->cpuEEPROMEnabled) {
      eepromWrite(address, b);
      break;
    }
    goto unwritable;
  case 14:
    // default
  default:
unwritable:
    break;
  }
}

#endif // GBAINLINE_H
