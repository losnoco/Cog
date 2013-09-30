#ifndef GBACPU_H
#define GBACPU_H

struct GBASystem;

extern int armExecute(GBASystem *);
extern int thumbExecute(GBASystem *);

#ifdef __GNUC__
# define INSN_REGPARM __attribute__((regparm(2)))
# define LIKELY(x) __builtin_expect(!!(x),1)
# define UNLIKELY(x) __builtin_expect(!!(x),0)
#else
# define INSN_REGPARM /*nothing*/
# define LIKELY(x) (x)
# define UNLIKELY(x) (x)
#endif

#define UPDATE_REG(address, value)\
  {\
    WRITE16LE(((u16 *)&gba->ioMem[address]),value);\
  }\

#define ARM_PREFETCH \
  {\
    gba->cpuPrefetch[0] = CPUReadMemoryQuick(gba, gba->armNextPC);\
    gba->cpuPrefetch[1] = CPUReadMemoryQuick(gba, gba->armNextPC+4);\
  }

#define THUMB_PREFETCH \
  {\
    gba->cpuPrefetch[0] = CPUReadHalfWordQuick(gba, gba->armNextPC);\
    gba->cpuPrefetch[1] = CPUReadHalfWordQuick(gba, gba->armNextPC+2);\
  }

#define ARM_PREFETCH_NEXT \
  gba->cpuPrefetch[1] = CPUReadMemoryQuick(gba, gba->armNextPC+4);

#define THUMB_PREFETCH_NEXT\
  gba->cpuPrefetch[1] = CPUReadHalfWordQuick(gba, gba->armNextPC+2);


extern void CPUSwitchMode(GBASystem *, int mode, bool saveState, bool breakLoop);
extern void CPUSwitchMode(GBASystem *, int mode, bool saveState);
extern void CPUUpdateCPSR(GBASystem *);
extern void CPUUpdateFlags(GBASystem *, bool breakLoop);
extern void CPUUpdateFlags(GBASystem *);
extern void CPUUndefinedException(GBASystem *);
extern void CPUSoftwareInterrupt(GBASystem *);
extern void CPUSoftwareInterrupt(GBASystem *, int comment);


// Waitstates when accessing data
inline int dataTicksAccess16(GBASystem *gba, u32 address) // DATA 8/16bits NON SEQ
{
  int addr = (address>>24)&15;
  int value =  gba->memoryWait[addr];

  if ((addr>=0x08) || (addr < 0x02))
  {
    gba->busPrefetchCount=0;
    gba->busPrefetch=false;
  }
  else if (gba->busPrefetch)
  {
    int waitState = value;
    if (!waitState)
      waitState = 1;
    gba->busPrefetchCount = ((gba->busPrefetchCount+1)<<waitState) - 1;
  }

  return value;
}

inline int dataTicksAccess32(GBASystem *gba, u32 address) // DATA 32bits NON SEQ
{
  int addr = (address>>24)&15;
  int value = gba->memoryWait32[addr];

  if ((addr>=0x08) || (addr < 0x02))
  {
    gba->busPrefetchCount=0;
    gba->busPrefetch=false;
  }
  else if (gba->busPrefetch)
  {
    int waitState = value;
    if (!waitState)
      waitState = 1;
    gba->busPrefetchCount = ((gba->busPrefetchCount+1)<<waitState) - 1;
  }

  return value;
}

inline int dataTicksAccessSeq16(GBASystem *gba, u32 address)// DATA 8/16bits SEQ
{
  int addr = (address>>24)&15;
  int value = gba->memoryWaitSeq[addr];

  if ((addr>=0x08) || (addr < 0x02))
  {
    gba->busPrefetchCount=0;
    gba->busPrefetch=false;
  }
  else if (gba->busPrefetch)
  {
    int waitState = value;
    if (!waitState)
      waitState = 1;
    gba->busPrefetchCount = ((gba->busPrefetchCount+1)<<waitState) - 1;
  }

  return value;
}

inline int dataTicksAccessSeq32(GBASystem *gba, u32 address)// DATA 32bits SEQ
{
  int addr = (address>>24)&15;
  int value =  gba->memoryWaitSeq32[addr];

  if ((addr>=0x08) || (addr < 0x02))
  {
    gba->busPrefetchCount=0;
    gba->busPrefetch=false;
  }
  else if (gba->busPrefetch)
  {
    int waitState = value;
    if (!waitState)
      waitState = 1;
    gba->busPrefetchCount = ((gba->busPrefetchCount+1)<<waitState) - 1;
  }

  return value;
}


// Waitstates when executing opcode
inline int codeTicksAccess16(GBASystem *gba, u32 address) // THUMB NON SEQ
{
  int addr = (address>>24)&15;

  if ((addr>=0x08) && (addr<=0x0D))
  {
    if (gba->busPrefetchCount&0x1)
    {
      if (gba->busPrefetchCount&0x2)
      {
        gba->busPrefetchCount = ((gba->busPrefetchCount&0xFF)>>2) | (gba->busPrefetchCount&0xFFFFFF00);
        return 0;
      }
      gba->busPrefetchCount = ((gba->busPrefetchCount&0xFF)>>1) | (gba->busPrefetchCount&0xFFFFFF00);
      return gba->memoryWaitSeq[addr]-1;
    }
    else
    {
      gba->busPrefetchCount=0;
      return gba->memoryWait[addr];
    }
  }
  else
  {
    gba->busPrefetchCount = 0;
    return gba->memoryWait[addr];
  }
}

inline int codeTicksAccess32(GBASystem *gba, u32 address) // ARM NON SEQ
{
  int addr = (address>>24)&15;

  if ((addr>=0x08) && (addr<=0x0D))
  {
    if (gba->busPrefetchCount&0x1)
    {
      if (gba->busPrefetchCount&0x2)
      {
        gba->busPrefetchCount = ((gba->busPrefetchCount&0xFF)>>2) | (gba->busPrefetchCount&0xFFFFFF00);
        return 0;
      }
      gba->busPrefetchCount = ((gba->busPrefetchCount&0xFF)>>1) | (gba->busPrefetchCount&0xFFFFFF00);
      return gba->memoryWaitSeq[addr] - 1;
    }
    else
    {
      gba->busPrefetchCount = 0;
      return gba->memoryWait32[addr];
    }
  }
  else
  {
    gba->busPrefetchCount = 0;
    return gba->memoryWait32[addr];
  }
}

inline int codeTicksAccessSeq16(GBASystem *gba, u32 address) // THUMB SEQ
{
  int addr = (address>>24)&15;

  if ((addr>=0x08) && (addr<=0x0D))
  {
    if (gba->busPrefetchCount&0x1)
    {
      gba->busPrefetchCount = ((gba->busPrefetchCount&0xFF)>>1) | (gba->busPrefetchCount&0xFFFFFF00);
      return 0;
    }
    else
    if (gba->busPrefetchCount>0xFF)
    {
      gba->busPrefetchCount=0;
      return gba->memoryWait[addr];
    }
    else
      return gba->memoryWaitSeq[addr];
  }
  else
  {
    gba->busPrefetchCount = 0;
    return gba->memoryWaitSeq[addr];
  }
}

inline int codeTicksAccessSeq32(GBASystem *gba, u32 address) // ARM SEQ
{
  int addr = (address>>24)&15;

  if ((addr>=0x08) && (addr<=0x0D))
  {
    if (gba->busPrefetchCount&0x1)
    {
      if (gba->busPrefetchCount&0x2)
      {
        gba->busPrefetchCount = ((gba->busPrefetchCount&0xFF)>>2) | (gba->busPrefetchCount&0xFFFFFF00);
        return 0;
      }
      gba->busPrefetchCount = ((gba->busPrefetchCount&0xFF)>>1) | (gba->busPrefetchCount&0xFFFFFF00);
      return gba->memoryWaitSeq[addr];
    }
    else
    if (gba->busPrefetchCount>0xFF)
    {
      gba->busPrefetchCount=0;
      return gba->memoryWait32[addr];
    }
    else
      return gba->memoryWaitSeq32[addr];
  }
  else
  {
    return gba->memoryWaitSeq32[addr];
  }
}

#endif // GBACPU_H
