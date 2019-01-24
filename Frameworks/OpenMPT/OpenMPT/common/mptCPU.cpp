/*
 * mptCPU.cpp
 * ----------
 * Purpose: CPU feature detection.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "mptCPU.h"

#include "mptStringBuffer.h"


OPENMPT_NAMESPACE_BEGIN


#if defined(ENABLE_ASM)


uint32 RealProcSupport = 0;
uint32 ProcSupport = 0;
char ProcVendorID[16+1] = "";
char ProcBrandID[4*4*3+1] = "";
uint16 ProcFamily = 0;
uint8 ProcModel = 0;
uint8 ProcStepping = 0;


#if MPT_COMPILER_MSVC && (defined(ENABLE_X86) || defined(ENABLE_X64))


#include <intrin.h>


typedef char cpuid_result_string[12];


struct cpuid_result {
	uint32 a;
	uint32 b;
	uint32 c;
	uint32 d;
	std::string as_string() const
	{
		cpuid_result_string result;
		result[0+0] = (b >> 0) & 0xff;
		result[0+1] = (b >> 8) & 0xff;
		result[0+2] = (b >>16) & 0xff;
		result[0+3] = (b >>24) & 0xff;
		result[4+0] = (d >> 0) & 0xff;
		result[4+1] = (d >> 8) & 0xff;
		result[4+2] = (d >>16) & 0xff;
		result[4+3] = (d >>24) & 0xff;
		result[8+0] = (c >> 0) & 0xff;
		result[8+1] = (c >> 8) & 0xff;
		result[8+2] = (c >>16) & 0xff;
		result[8+3] = (c >>24) & 0xff;
		return std::string(result, result + 12);
	}
	std::string as_string4() const
	{
		std::string result;
		result.push_back(static_cast<uint8>((a >>  0) & 0xff));
		result.push_back(static_cast<uint8>((a >>  8) & 0xff));
		result.push_back(static_cast<uint8>((a >> 16) & 0xff));
		result.push_back(static_cast<uint8>((a >> 24) & 0xff));
		result.push_back(static_cast<uint8>((b >>  0) & 0xff));
		result.push_back(static_cast<uint8>((b >>  8) & 0xff));
		result.push_back(static_cast<uint8>((b >> 16) & 0xff));
		result.push_back(static_cast<uint8>((b >> 24) & 0xff));
		result.push_back(static_cast<uint8>((c >>  0) & 0xff));
		result.push_back(static_cast<uint8>((c >>  8) & 0xff));
		result.push_back(static_cast<uint8>((c >> 16) & 0xff));
		result.push_back(static_cast<uint8>((c >> 24) & 0xff));
		result.push_back(static_cast<uint8>((d >>  0) & 0xff));
		result.push_back(static_cast<uint8>((d >>  8) & 0xff));
		result.push_back(static_cast<uint8>((d >> 16) & 0xff));
		result.push_back(static_cast<uint8>((d >> 24) & 0xff));
		return result;
	}
};


static cpuid_result cpuid(uint32 function)
{
	cpuid_result result;
	int CPUInfo[4];
	__cpuid(CPUInfo, function);
	result.a = CPUInfo[0];
	result.b = CPUInfo[1];
	result.c = CPUInfo[2];
	result.d = CPUInfo[3];
	return result;
}


#if 0

static cpuid_result cpuidex(uint32 function_a, uint32 function_c)
{
	cpuid_result result;
	int CPUInfo[4];
	__cpuidex(CPUInfo, function_a, function_c);
	result.a = CPUInfo[0];
	result.b = CPUInfo[1];
	result.c = CPUInfo[2];
	result.d = CPUInfo[3];
	return result;
}

#endif


void InitProcSupport()
{

	RealProcSupport = 0;
	ProcSupport = 0;
	MemsetZero(ProcVendorID);
	MemsetZero(ProcBrandID);
	ProcFamily = 0;
	ProcModel = 0;
	ProcStepping = 0;

	{

		cpuid_result VendorString = cpuid(0x00000000u);
		mpt::String::WriteAutoBuf(ProcVendorID) = VendorString.as_string();

		// Cyrix 6x86 and 6x86MX do not specify the value returned in eax.
		// They both support 0x00000001u however.
		if((VendorString.as_string() == "CyrixInstead") || (VendorString.a >= 0x00000001u))
		{
			cpuid_result StandardFeatureFlags = cpuid(0x00000001u);
			uint32 Stepping   = (StandardFeatureFlags.a >>  0) & 0x0f;
			uint32 BaseModel  = (StandardFeatureFlags.a >>  4) & 0x0f;
			uint32 BaseFamily = (StandardFeatureFlags.a >>  8) & 0x0f;
			uint32 ExtModel   = (StandardFeatureFlags.a >> 16) & 0x0f;
			uint32 ExtFamily  = (StandardFeatureFlags.a >> 20) & 0xff;
			if(VendorString.as_string() == "GenuineIntel")
			{
				if(BaseFamily == 0xf)
				{
					ProcFamily = static_cast<uint16>(ExtFamily + BaseFamily);
				} else
				{
					ProcFamily = static_cast<uint16>(BaseFamily);
				}
				if(BaseFamily == 0x6 || BaseFamily == 0xf)
				{
					ProcModel = static_cast<uint8>((ExtModel << 4) | (BaseModel << 0));
				} else
				{
					ProcModel = static_cast<uint8>(BaseModel);
				}
			} else if(VendorString.as_string() == "AuthenticAMD")
			{
				if(BaseFamily == 0xf)
				{
					ProcFamily = static_cast<uint16>(ExtFamily + BaseFamily);
					ProcModel = static_cast<uint8>((ExtModel << 4) | (BaseModel << 0));
				} else
				{
					ProcFamily = static_cast<uint16>(BaseFamily);
					ProcModel = static_cast<uint8>(BaseModel);
				}
			} else
			{
				ProcFamily = static_cast<uint16>(BaseFamily);
				ProcModel = static_cast<uint8>(BaseModel);
			}
			ProcStepping = static_cast<uint8>(Stepping);
			if(StandardFeatureFlags.d & (1<<15)) ProcSupport |= PROCSUPPORT_CMOV;
			if(StandardFeatureFlags.d & (1<<23)) ProcSupport |= PROCSUPPORT_MMX;
			if(StandardFeatureFlags.d & (1<<25)) ProcSupport |= PROCSUPPORT_SSE;
			if(StandardFeatureFlags.d & (1<<26)) ProcSupport |= PROCSUPPORT_SSE2;
			if(StandardFeatureFlags.c & (1<< 0)) ProcSupport |= PROCSUPPORT_SSE3;
			if(StandardFeatureFlags.c & (1<< 9)) ProcSupport |= PROCSUPPORT_SSSE3;
			if(StandardFeatureFlags.c & (1<<19)) ProcSupport |= PROCSUPPORT_SSE4_1;
			if(StandardFeatureFlags.c & (1<<20)) ProcSupport |= PROCSUPPORT_SSE4_2;
		}

		bool canExtended = false;
		// 3DNow! manual recommends to just execute 0x80000000u.
		// It is totally unknown how earlier CPUs from other vendors
		// would behave.
		// Thus we only execute 0x80000000u on other vendors CPUs for the earliest
		// that we found it documented for and that actually supports 3DNow!.
		// We only need 0x80000000u in order to detect 3DNow!.
		// Thus, this is enough for us.
		if(VendorString.as_string() == "GenuineIntel")
		{ // Intel

			// 5.9.x : Quark
			// 6.11.x: P3-S (Tualatin)
			if((ProcFamily > 6) || ((ProcFamily == 6) && (ProcModel >= 11)) || ((ProcFamily == 5) && (ProcModel >= 9)))
			{
				canExtended = true;
			}

		} else if((VendorString.as_string() == "AuthenticAMD") || (VendorString.as_string() == "AMDisbetter!"))
		{ // AMD

			if((ProcFamily > 5) || ((ProcFamily == 5) && (ProcModel >= 8)))
			{ // >= K6-2 (K6 = Family 5, K6-2 = Model 8)
				// Not sure if earlier AMD CPUs support 0x80000000u.
				// AMD 5k86 and AMD K5 manuals do not mention it.
				canExtended = true;
			}

		} else if(VendorString.as_string() == "CentaurHauls")
		{ // Centaur (IDT WinChip or VIA C3)

			if(ProcFamily == 5)
			{ // IDT

				if(ProcModel >= 8)
				{ // >= WinChip 2
					canExtended = true;
				}

			} else if(ProcFamily >= 6)
			{ // VIA

				if((ProcFamily >= 7) || ((ProcFamily == 6) && (ProcModel >= 7)))
				{ // >= C3 Samuel 2
					canExtended = true;
				}

			}

		} else if(VendorString.as_string() == "CyrixInstead")
		{ // Cyrix

			// 6x86    : 5.2.x
			// 6x86L   : 5.2.x
			// MediaGX : 4.4.x
			// 6x86MX  : 6.0.x
			// MII     : 6.0.x
			// MediaGXm: 5.4.x
			// well, doh ...

			if((ProcFamily == 5) && (ProcModel >= 4))
			{ // Cyrix MediaGXm
				canExtended = true;
			}

		} else if(VendorString.as_string() == "Geode by NSC")
		{ // National Semiconductor

			if((ProcFamily > 5) || ((ProcFamily == 5) && (ProcModel >= 5)))
			{ // >= Geode GX2
				canExtended = true;
			}

		} else
		{ // unknown, which nowadays most likely means some virtualized CPU

			// we assume extended flags present in this case
			canExtended = true;

		}

		if(canExtended)
		{
			cpuid_result ExtendedVendorString = cpuid(0x80000000u);
			if(ExtendedVendorString.a >= 0x80000001u)
			{
				cpuid_result ExtendedFeatureFlags = cpuid(0x80000001u);
				if(ExtendedFeatureFlags.d & (1<<29)) ProcSupport |= PROCSUPPORT_LM;
				if((VendorString.as_string() == "AuthenticAMD") || (VendorString.as_string() == "AMDisbetter!"))
				{
					if(ExtendedFeatureFlags.d & (1<<15)) ProcSupport |= PROCSUPPORT_CMOV;
					if(ExtendedFeatureFlags.d & (1<<23)) ProcSupport |= PROCSUPPORT_MMX;
				}
				if(ExtendedFeatureFlags.d & (1<<22)) ProcSupport |= PROCSUPPORT_AMD_MMXEXT;
				if(ExtendedFeatureFlags.d & (1<<31)) ProcSupport |= PROCSUPPORT_AMD_3DNOW;
				if(ExtendedFeatureFlags.d & (1<<30)) ProcSupport |= PROCSUPPORT_AMD_3DNOWEXT;
			}
			if(ExtendedVendorString.a >= 0x80000004u)
			{
				mpt::String::WriteAutoBuf(ProcBrandID) = cpuid(0x80000002u).as_string4() + cpuid(0x80000003u).as_string4() + cpuid(0x80000004u).as_string4();
			}
		}

	}

	// We do not have to check if SSE got enabled by the OS because we only do
	// support Windows >= XP. Windows will always enable SSE since Windows 98 SE.

	RealProcSupport = ProcSupport;

}


#else // !( MPT_COMPILER_MSVC && ENABLE_X86 )


void InitProcSupport()
{
	ProcSupport = 0;
}


#endif // MPT_COMPILER_MSVC && ENABLE_X86

#endif // ENABLE_ASM


#ifdef MODPLUG_TRACKER


uint32 GetMinimumProcSupportFlags()
{
	uint32 flags = 0;
	#ifdef ENABLE_ASM
		#if MPT_COMPILER_MSVC
			#if defined(_M_X64)
				flags |= PROCSUPPORT_AMD64;
			#elif defined(_M_IX86)
				#if defined(_M_IX86_FP)
					#if (_M_IX86_FP >= 2)
						flags |= PROCSUPPORT_x86_SSE2;
					#elif (_M_IX86_FP == 1)
						flags |= PROCSUPPORT_x86_SSE;
					#endif
				#else
					flags |= PROCSUPPORT_i586;
				#endif
			#endif
		#endif	
	#endif // ENABLE_ASM
	return flags;
}



int GetMinimumSSEVersion()
{
	int minimumSSEVersion = 0;
	#if MPT_COMPILER_MSVC
		#if defined(_M_X64)
			minimumSSEVersion = 2;
		#elif defined(_M_IX86)
			#if defined(_M_IX86_FP)
				#if (_M_IX86_FP >= 2)
					minimumSSEVersion = 2;
				#elif (_M_IX86_FP == 1)
					minimumSSEVersion = 1;
				#endif
			#endif
		#endif
	#endif
	return minimumSSEVersion;
}


int GetMinimumAVXVersion()
{
	int minimumAVXVersion = 0;
	#if MPT_COMPILER_MSVC
		#if defined(_M_IX86_FP)
			#if defined(__AVX2__)
				minimumAVXVersion = 2;
			#elif defined(__AVX__)
				minimumAVXVersion = 1;
			#endif
		#endif
	#endif
	return minimumAVXVersion;
}


#endif


#if !defined(MODPLUG_TRACKER) && !defined(ENABLE_ASM)

MPT_MSVC_WORKAROUND_LNK4221(mptCPU)

#endif


OPENMPT_NAMESPACE_END
