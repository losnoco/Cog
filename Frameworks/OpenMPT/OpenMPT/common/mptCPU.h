/*
 * mptCPU.h
 * --------
 * Purpose: CPU feature detection.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"


OPENMPT_NAMESPACE_BEGIN


#ifdef MODPLUG_TRACKER

#define PROCSUPPORT_ASM_INTRIN   0x00001 // assembly and intrinsics are enabled at runtime
#define PROCSUPPORT_CPUID        0x00002 // Processor supports modern cpuid
#define PROCSUPPORT_LM           0x00004 // Processor supports long mode (amd64)
#define PROCSUPPORT_MMX          0x00010 // Processor supports MMX instructions
#define PROCSUPPORT_SSE          0x00100 // Processor supports SSE instructions
#define PROCSUPPORT_SSE2         0x00200 // Processor supports SSE2 instructions
#define PROCSUPPORT_SSE3         0x00400 // Processor supports SSE3 instructions
#define PROCSUPPORT_SSSE3        0x00800 // Processor supports SSSE3 instructions
#define PROCSUPPORT_SSE4_1       0x01000 // Processor supports SSE4.1 instructions
#define PROCSUPPORT_SSE4_2       0x02000 // Processor supports SSE4.2 instructions
#define PROCSUPPORT_AVX          0x10000 // Processor supports AVX instructions
#define PROCSUPPORT_AVX2         0x20000 // Processor supports AVX2 instructions

static constexpr uint32 PROCSUPPORT_i586     = 0u                                                      ;
static constexpr uint32 PROCSUPPORT_x86_SSE  = 0u | PROCSUPPORT_SSE                                    ;
static constexpr uint32 PROCSUPPORT_x86_SSE2 = 0u | PROCSUPPORT_SSE | PROCSUPPORT_SSE2                 ;
static constexpr uint32 PROCSUPPORT_AMD64    = 0u | PROCSUPPORT_SSE | PROCSUPPORT_SSE2 | PROCSUPPORT_LM;

#endif


#ifdef ENABLE_ASM

extern uint32 RealProcSupport;
extern uint32 ProcSupport;
extern char ProcVendorID[16+1];
extern char ProcBrandID[4*4*3+1];
extern uint16 ProcFamily;
extern uint8 ProcModel;
extern uint8 ProcStepping;

void InitProcSupport();

// enabled processor features for inline asm and intrinsics
static inline uint32 GetProcSupport()
{
	return ProcSupport;
}

// available processor features
static inline uint32 GetRealProcSupport()
{
	return RealProcSupport;
}

#endif // ENABLE_ASM


#ifdef MODPLUG_TRACKER
uint32 GetMinimumProcSupportFlags();
int GetMinimumSSEVersion();
int GetMinimumAVXVersion();
#endif


OPENMPT_NAMESPACE_END
