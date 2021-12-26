/*
 * mptOS.cpp
 * ---------
 * Purpose: Operating system version information.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "mptOS.h"

#if MPT_OS_WINDOWS
#include <windows.h>
#endif

#if defined(MODPLUG_TRACKER)
#if !MPT_OS_WINDOWS
#include <sys/utsname.h>
#endif // !MPT_OS_WINDOWS
#endif // MODPLUG_TRACKER


OPENMPT_NAMESPACE_BEGIN


#if defined(MODPLUG_TRACKER)

namespace mpt
{
namespace OS
{

mpt::OS::Class GetClassFromSysname(mpt::ustring sysname)
{
	mpt::OS::Class result = mpt::OS::Class::Unknown;
	if(sysname == U_(""))
	{
		result = mpt::OS::Class::Unknown;
	} else if(sysname == U_("Windows") || sysname == U_("WindowsNT") || sysname == U_("Windows_NT"))
	{
		result = mpt::OS::Class::Windows;
	} else if(sysname == U_("Linux"))
	{
		result = mpt::OS::Class::Linux;
	} else if(sysname == U_("Darwin"))
	{
		result = mpt::OS::Class::Darwin;
	} else if(sysname == U_("FreeBSD") || sysname == U_("DragonFly") || sysname == U_("NetBSD") || sysname == U_("OpenBSD") || sysname == U_("MidnightBSD"))
	{
		result = mpt::OS::Class::BSD;
	} else if(sysname == U_("Haiku"))
	{
		result = mpt::OS::Class::Haiku;
	} else if(sysname == U_("MS-DOS"))
	{
		result = mpt::OS::Class::DOS;
	}
	return result;
}

mpt::OS::Class GetClass()
{
	#if MPT_OS_WINDOWS
		return mpt::OS::Class::Windows;
	#else // !MPT_OS_WINDOWS
		utsname uname_result;
		if(uname(&uname_result) != 0)
		{
			return mpt::OS::Class::Unknown;
		}
		return mpt::OS::GetClassFromSysname(mpt::ToUnicode(mpt::Charset::ASCII, mpt::String::ReadAutoBuf(uname_result.sysname)));
	#endif // MPT_OS_WINDOWS
}

}  // namespace OS
}  // namespace mpt

#endif // MODPLUG_TRACKER


namespace mpt
{
namespace Windows
{


#if MPT_OS_WINDOWS


static mpt::Windows::Version VersionFromNTDDI_VERSION() noexcept
{
	// Initialize to used SDK version
	mpt::Windows::Version::System System =
		#if NTDDI_VERSION >= 0x0A000000 // NTDDI_WIN10
			mpt::Windows::Version::Win10
		#elif NTDDI_VERSION >= 0x06030000 // NTDDI_WINBLUE
			mpt::Windows::Version::Win81
		#elif NTDDI_VERSION >= 0x06020000 // NTDDI_WIN8
			mpt::Windows::Version::Win8
		#elif NTDDI_VERSION >= 0x06010000 // NTDDI_WIN7
			mpt::Windows::Version::Win7
		#elif NTDDI_VERSION >= 0x06000000 // NTDDI_VISTA
			mpt::Windows::Version::WinVista
		#elif NTDDI_VERSION >= 0x05020000 // NTDDI_WS03
			mpt::Windows::Version::WinXP64
		#elif NTDDI_VERSION >= NTDDI_WINXP
			mpt::Windows::Version::WinXP
		#elif NTDDI_VERSION >= NTDDI_WIN2K
			mpt::Windows::Version::Win2000
		#else
			mpt::Windows::Version::WinNT4
		#endif
		;
	return mpt::Windows::Version(System, mpt::Windows::Version::ServicePack(((NTDDI_VERSION & 0xffffu) >> 8) & 0xffu, ((NTDDI_VERSION & 0xffffu) >> 0) & 0xffu), 0, 0);
}


static mpt::Windows::Version::System SystemVersionFrom_WIN32_WINNT() noexcept
{
	#if defined(_WIN32_WINNT)
		return mpt::Windows::Version::System((static_cast<uint64>(_WIN32_WINNT) & 0xff00u) >> 8, (static_cast<uint64>(_WIN32_WINNT) & 0x00ffu) >> 0);
	#else
		return mpt::Windows::Version::System();
	#endif
}


static mpt::Windows::Version GatherWindowsVersion() noexcept
{
#if MPT_OS_WINDOWS_WINRT
	return VersionFromNTDDI_VERSION();
#else // !MPT_OS_WINDOWS_WINRT
	OSVERSIONINFOEXW versioninfoex;
	MemsetZero(versioninfoex);
	versioninfoex.dwOSVersionInfoSize = sizeof(versioninfoex);
#if MPT_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable:4996) // 'GetVersionExW': was declared deprecated
#pragma warning(disable:28159) // Consider using 'IsWindows*' instead of 'GetVersionExW'. Reason: Deprecated. Use VerifyVersionInfo* or IsWindows* macros from VersionHelpers.
#endif // MPT_COMPILER_MSVC
#if MPT_COMPILER_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif // MPT_COMPILER_CLANG
	if(GetVersionExW((LPOSVERSIONINFOW)&versioninfoex) == FALSE)
	{
		return VersionFromNTDDI_VERSION();
	}
#if MPT_COMPILER_MSVC
#pragma warning(pop)
#endif // MPT_COMPILER_MSVC
#if MPT_COMPILER_CLANG
#pragma clang diagnostic pop
#endif // MPT_COMPILER_CLANG
	if(versioninfoex.dwPlatformId != VER_PLATFORM_WIN32_NT)
	{
		return VersionFromNTDDI_VERSION();
	}
	DWORD dwProductType = 0;
	dwProductType = PRODUCT_UNDEFINED;
	if(GetProductInfo(versioninfoex.dwMajorVersion, versioninfoex.dwMinorVersion, versioninfoex.wServicePackMajor, versioninfoex.wServicePackMinor, &dwProductType) == FALSE)
	{
		dwProductType = PRODUCT_UNDEFINED;
	}
	return mpt::Windows::Version(
		mpt::Windows::Version::System(versioninfoex.dwMajorVersion, versioninfoex.dwMinorVersion),
		mpt::Windows::Version::ServicePack(versioninfoex.wServicePackMajor, versioninfoex.wServicePackMinor),
		versioninfoex.dwBuildNumber,
		dwProductType
		);
#endif // MPT_OS_WINDOWS_WINRT
}


#ifdef MODPLUG_TRACKER

namespace {
struct WindowsVersionCache
{
	mpt::Windows::Version version;
	WindowsVersionCache() noexcept
		: version(GatherWindowsVersion())
	{
	}
};
}

static mpt::Windows::Version GatherWindowsVersionFromCache() noexcept
{
	static WindowsVersionCache gs_WindowsVersionCache;
	return gs_WindowsVersionCache.version;
}

#endif // MODPLUG_TRACKER


#endif // MPT_OS_WINDOWS


Version::Version() noexcept
	: m_SystemIsWindows(false)
	, m_System()
	, m_ServicePack()
	, m_Build()
	, m_Type()
{
}


Version Version::NoWindows() noexcept
{
	return Version();
}


Version::Version(mpt::Windows::Version::System system, mpt::Windows::Version::ServicePack servicePack, mpt::Windows::Version::Build build, mpt::Windows::Version::TypeId type) noexcept
	: m_SystemIsWindows(true)
	, m_System(system)
	, m_ServicePack(servicePack)
	, m_Build(build)
	, m_Type(type)
{
}


mpt::Windows::Version Version::Current() noexcept
{
	#if MPT_OS_WINDOWS
		#ifdef MODPLUG_TRACKER
			return GatherWindowsVersionFromCache();
		#else // !MODPLUG_TRACKER
			return GatherWindowsVersion();
		#endif // MODPLUG_TRACKER
	#else // !MPT_OS_WINDOWS
		return mpt::Windows::Version::NoWindows();
	#endif // MPT_OS_WINDOWS
}


bool Version::IsWindows() const noexcept
{
	return m_SystemIsWindows;
}


bool Version::IsBefore(mpt::Windows::Version::System version) const noexcept
{
	if(!m_SystemIsWindows)
	{
		return false;
	}
	return m_System < version;
}


bool Version::IsBefore(mpt::Windows::Version::System version, mpt::Windows::Version::ServicePack servicePack) const noexcept
{
	if(!m_SystemIsWindows)
	{
		return false;
	}
	if(m_System > version)
	{
		return false;
	}
	if(m_System < version)
	{
		return true;
	}
	return m_ServicePack < servicePack;
}


bool Version::IsBefore(mpt::Windows::Version::System version, mpt::Windows::Version::Build build) const noexcept
{
	if(!m_SystemIsWindows)
	{
		return false;
	}
	if(m_System > version)
	{
		return false;
	}
	if(m_System < version)
	{
		return true;
	}
	return m_Build < build;
}


bool Version::IsAtLeast(mpt::Windows::Version::System version) const noexcept
{
	if(!m_SystemIsWindows)
	{
		return false;
	}
	return m_System >= version;
}


bool Version::IsAtLeast(mpt::Windows::Version::System version, mpt::Windows::Version::ServicePack servicePack) const noexcept
{
	if(!m_SystemIsWindows)
	{
		return false;
	}
	if(m_System < version)
	{
		return false;
	}
	if(m_System > version)
	{
		return true;
	}
	return m_ServicePack >= servicePack;
}


bool Version::IsAtLeast(mpt::Windows::Version::System version, mpt::Windows::Version::Build build) const noexcept
{
	if(!m_SystemIsWindows)
	{
		return false;
	}
	if(m_System < version)
	{
		return false;
	}
	if(m_System > version)
	{
		return true;
	}
	return m_Build >= build;
}


mpt::Windows::Version::System Version::GetSystem() const noexcept
{
	return m_System;
}


mpt::Windows::Version::ServicePack Version::GetServicePack() const noexcept
{
	return m_ServicePack;
}


mpt::Windows::Version::Build Version::GetBuild() const noexcept
{
	return m_Build;
}


mpt::Windows::Version::TypeId Version::GetTypeId() const noexcept
{
	return m_Type;
}


static constexpr struct { Version::System version; const mpt::uchar * name; bool showDetails; } versionMap[] =
{
	{ mpt::Windows::Version::WinNewer, UL_("Windows 10 (or newer)"), false },
	{ mpt::Windows::Version::Win10, UL_("Windows 10"), true },
	{ mpt::Windows::Version::Win81, UL_("Windows 8.1"), true },
	{ mpt::Windows::Version::Win8, UL_("Windows 8"), true },
	{ mpt::Windows::Version::Win7, UL_("Windows 7"), true },
	{ mpt::Windows::Version::WinVista, UL_("Windows Vista"), true },
	{ mpt::Windows::Version::WinXP64, UL_("Windows XP x64 / Windows Server 2003"), true },
	{ mpt::Windows::Version::WinXP, UL_("Windows XP"), true },
	{ mpt::Windows::Version::Win2000, UL_("Windows 2000"), true },
	{ mpt::Windows::Version::WinNT4, UL_("Windows NT4"), true }
};


mpt::ustring Version::VersionToString(mpt::Windows::Version::System version)
{
	mpt::ustring result;
	for(const auto &v : versionMap)
	{
		if(version > v.version)
		{
			result = U_("> ") + v.name;
			break;
		} else if(version == v.version)
		{
			result = v.name;
			break;
		}
	}
	if(result.empty())
	{
		result = mpt::format(U_("0x%1"))(mpt::ufmt::hex0<16>(static_cast<uint64>(version)));
	}
	return result;
}


mpt::ustring Version::GetName() const
{
	mpt::ustring name = U_("Generic Windows NT");
	bool showDetails = false;
	for(const auto &v : versionMap)
	{
		if(IsAtLeast(v.version))
		{
			name = v.name;
			showDetails = v.showDetails;
			break;
		}
	}
	name += U_(" (");
	name += mpt::format(U_("Version %1.%2"))(m_System.Major, m_System.Minor);
	if(showDetails)
	{
		if(m_ServicePack.HasServicePack())
		{
			if(m_ServicePack.Minor)
			{
				name += mpt::format(U_(" Service Pack %1.%2"))(m_ServicePack.Major, m_ServicePack.Minor);
			} else
			{
				name += mpt::format(U_(" Service Pack %1"))(m_ServicePack.Major);
			}
		}
		if(m_Build != 0)
		{
			name += mpt::format(U_(" (Build %1)"))(m_Build);
		}
	}
	name += U_(")");
	mpt::ustring result = name;
	#if defined(MODPLUG_TRACKER) && MPT_OS_WINDOWS
		if(mpt::Windows::IsWine())
		{
			mpt::Wine::VersionContext v;
			if(v.Version().IsValid())
			{
				result = mpt::format(U_("Wine %1 (%2)"))(
					  v.Version().AsString()
					, name
					);
			} else
			{
				result = mpt::format(U_("Wine (unknown version: '%1') (%2)"))(
					  mpt::ToUnicode(mpt::Charset::UTF8, v.RawVersion())
					, name
					);
			}
		}
	#endif // MODPLUG_TRACKER && MPT_OS_WINDOWS
	return result;
}


#ifdef MODPLUG_TRACKER
mpt::ustring Version::GetNameShort() const
{
	mpt::ustring name;
	if(mpt::Windows::IsWine())
	{
		mpt::Wine::VersionContext v;
		if(v.Version().IsValid())
		{
			name = mpt::format(U_("wine-%1"))(v.Version().AsString());
		} else if(v.RawVersion().length() > 0)
		{
			name = U_("wine-") + Util::BinToHex(mpt::as_span(v.RawVersion()));
		} else
		{
			name = U_("wine-");
		}
		name += U_("-") + Util::BinToHex(mpt::as_span(v.RawHostSysName()));
	} else
	{
		name = mpt::format(U_("%1.%2"))(mpt::ufmt::dec(m_System.Major), mpt::ufmt::dec0<2>(m_System.Minor));
	}
	return name;
}
#endif // MODPLUG_TRACKER


mpt::Windows::Version::System Version::GetMinimumKernelLevel() noexcept
{
	uint64 minimumKernelVersion = 0;
	#if MPT_OS_WINDOWS && MPT_COMPILER_MSVC
		minimumKernelVersion = std::max(minimumKernelVersion, static_cast<uint64>(mpt::Windows::Version::WinVista));
	#endif
	return mpt::Windows::Version::System(minimumKernelVersion);
}


mpt::Windows::Version::System Version::GetMinimumAPILevel() noexcept
{
	#if MPT_OS_WINDOWS
		return SystemVersionFrom_WIN32_WINNT();
	#else // !MPT_OS_WINDOWS
		return mpt::Windows::Version::System();
	#endif // MPT_OS_WINDOWS
}


#if defined(MODPLUG_TRACKER) && MPT_OS_WINDOWS


#ifndef PROCESSOR_ARCHITECTURE_NEUTRAL
#define PROCESSOR_ARCHITECTURE_NEUTRAL          11
#endif
#ifndef PROCESSOR_ARCHITECTURE_ARM64
#define PROCESSOR_ARCHITECTURE_ARM64            12
#endif
#ifndef PROCESSOR_ARCHITECTURE_ARM32_ON_WIN64
#define PROCESSOR_ARCHITECTURE_ARM32_ON_WIN64   13
#endif
#ifndef PROCESSOR_ARCHITECTURE_IA32_ON_ARM64
#define PROCESSOR_ARCHITECTURE_IA32_ON_ARM64    14
#endif


struct OSArchitecture
{
	uint16 ProcessorArchitectur;
	Architecture Host;
	Architecture Process;
};
static constexpr OSArchitecture architectures [] = {
	{ PROCESSOR_ARCHITECTURE_INTEL         , Architecture::x86    , Architecture::x86     },
	{ PROCESSOR_ARCHITECTURE_AMD64         , Architecture::amd64  , Architecture::amd64   },
	{ PROCESSOR_ARCHITECTURE_IA32_ON_WIN64 , Architecture::amd64  , Architecture::x86     },
	{ PROCESSOR_ARCHITECTURE_ARM           , Architecture::arm    , Architecture::arm     },
	{ PROCESSOR_ARCHITECTURE_ARM64         , Architecture::arm64  , Architecture::arm64   },
	{ PROCESSOR_ARCHITECTURE_ARM32_ON_WIN64, Architecture::arm64  , Architecture::arm     },
	{ PROCESSOR_ARCHITECTURE_IA32_ON_ARM64 , Architecture::arm64  , Architecture::x86     },
	{ PROCESSOR_ARCHITECTURE_MIPS          , Architecture::mips   , Architecture::mips    },
	{ PROCESSOR_ARCHITECTURE_PPC           , Architecture::ppc    , Architecture::ppc     },
	{ PROCESSOR_ARCHITECTURE_SHX           , Architecture::shx    , Architecture::shx     },
	{ PROCESSOR_ARCHITECTURE_ALPHA         , Architecture::alpha  , Architecture::alpha   },
	{ PROCESSOR_ARCHITECTURE_ALPHA64       , Architecture::alpha64, Architecture::alpha64 },
	{ PROCESSOR_ARCHITECTURE_IA64          , Architecture::ia64   , Architecture::ia64    },
	{ PROCESSOR_ARCHITECTURE_MSIL          , Architecture::unknown, Architecture::unknown },
	{ PROCESSOR_ARCHITECTURE_NEUTRAL       , Architecture::unknown, Architecture::unknown },
	{ PROCESSOR_ARCHITECTURE_UNKNOWN       , Architecture::unknown, Architecture::unknown }
};


struct HostArchitecture
{
	Architecture Host;
	Architecture Process;
	EmulationLevel Emulation;
};
static constexpr HostArchitecture hostArchitectureCanRun [] = {
	{ Architecture::x86    , Architecture::x86    , EmulationLevel::Native   },
	{ Architecture::amd64  , Architecture::amd64  , EmulationLevel::Native   },
	{ Architecture::amd64  , Architecture::x86    , EmulationLevel::Virtual  },
	{ Architecture::arm    , Architecture::arm    , EmulationLevel::Native   },
	{ Architecture::arm64  , Architecture::arm64  , EmulationLevel::Native   },
	{ Architecture::arm64  , Architecture::arm    , EmulationLevel::Virtual  },
	{ Architecture::arm64  , Architecture::x86    , EmulationLevel::Software },
	{ Architecture::mips   , Architecture::mips   , EmulationLevel::Native   },
	{ Architecture::ppc    , Architecture::ppc    , EmulationLevel::Native   },
	{ Architecture::shx    , Architecture::shx    , EmulationLevel::Native   },
	{ Architecture::alpha  , Architecture::alpha  , EmulationLevel::Native   },
	{ Architecture::alpha64, Architecture::alpha64, EmulationLevel::Native   },
	{ Architecture::alpha64, Architecture::alpha  , EmulationLevel::Virtual  },
	{ Architecture::ia64   , Architecture::ia64   , EmulationLevel::Native   },
	{ Architecture::ia64   , Architecture::x86    , EmulationLevel::Hardware }
};


struct ArchitectureInfo
{
	Architecture Arch;
	int Bitness;
	const mpt::uchar * Name;
};
static constexpr ArchitectureInfo architectureInfo [] = {
	{ Architecture::x86    , 32, UL_("x86")     },
	{ Architecture::amd64  , 64, UL_("amd64")   },
	{ Architecture::arm    , 32, UL_("arm")     },
	{ Architecture::arm64  , 64, UL_("arm64")   },
	{ Architecture::mips   , 32, UL_("mips")    },
	{ Architecture::ppc    , 32, UL_("ppc")     },
	{ Architecture::shx    , 32, UL_("shx")     },
	{ Architecture::alpha  , 32, UL_("alpha")   },
	{ Architecture::alpha64, 64, UL_("alpha64") },
	{ Architecture::ia64   , 64, UL_("ia64")    }
};


int Bitness(Architecture arch) noexcept
{
	for(const auto &info : architectureInfo)
	{
		if(arch == info.Arch)
		{
			return info.Bitness;
		}
	}
	return 0;
}


mpt::ustring Name(Architecture arch)
{
	for(const auto &info : architectureInfo)
	{
		if(arch == info.Arch)
		{
			return info.Name;
		}
	}
	return mpt::ustring();
}


Architecture GetHostArchitecture() noexcept
{
	SYSTEM_INFO systemInfo;
	MemsetZero(systemInfo);
	GetNativeSystemInfo(&systemInfo);
	for(const auto &arch : architectures)
	{
		if(systemInfo.wProcessorArchitecture == arch.ProcessorArchitectur)
		{
			return arch.Host;
		}
	}
	return Architecture::unknown;
}


Architecture GetProcessArchitecture() noexcept
{
	SYSTEM_INFO systemInfo;
	MemsetZero(systemInfo);
	GetSystemInfo(&systemInfo);
	for(const auto &arch : architectures)
	{
		if(systemInfo.wProcessorArchitecture == arch.ProcessorArchitectur)
		{
			return arch.Process;
		}
	}
	return Architecture::unknown;
}


EmulationLevel HostCanRun(Architecture host, Architecture process) noexcept
{
	for(const auto & can : hostArchitectureCanRun)
	{
		if(can.Host == host && can.Process == process)
		{
			return can.Emulation;
		}
	}
	return EmulationLevel::NA;
}


std::vector<Architecture> GetSupportedProcessArchitectures(Architecture host)
{
	std::vector<Architecture> result;
	for(const auto & entry : hostArchitectureCanRun)
	{
		if(entry.Host == host)
		{
			result.push_back(entry.Process);
		}
	}
	return result;
}


uint64 GetSystemMemorySize()
{
	MEMORYSTATUSEX memoryStatus;
	MemsetZero(memoryStatus);
	memoryStatus.dwLength = sizeof(MEMORYSTATUSEX);
	if(GlobalMemoryStatusEx(&memoryStatus) == 0)
	{
		return 0;
	}
	return memoryStatus.ullTotalPhys;
}


#endif // MODPLUG_TRACKER && MPT_OS_WINDOWS


#if defined(MODPLUG_TRACKER)


#if MPT_OS_WINDOWS

static bool GatherSystemIsWine()
{
	bool SystemIsWine = false;
	HMODULE hNTDLL = LoadLibrary(TEXT("ntdll.dll"));
	if(hNTDLL)
	{
		SystemIsWine = (GetProcAddress(hNTDLL, "wine_get_version") != NULL);
		FreeLibrary(hNTDLL);
		hNTDLL = NULL;
	}
	return SystemIsWine;
}

namespace {
struct SystemIsWineCache
{
	bool SystemIsWine;
	SystemIsWineCache()
		: SystemIsWine(GatherSystemIsWine())
	{
		return;
	}
	SystemIsWineCache(bool isWine)
		: SystemIsWine(isWine)
	{
		return;
	}
};
}

#endif // MPT_OS_WINDOWS

static bool SystemIsWine(bool allowDetection = true)
{
	#if MPT_OS_WINDOWS
		static SystemIsWineCache gs_SystemIsWineCache = allowDetection ? SystemIsWineCache() : SystemIsWineCache(false);
		if(!allowDetection)
		{ // catch too late calls of PreventWineDetection
			MPT_ASSERT(!gs_SystemIsWineCache.SystemIsWine);
		}
		return gs_SystemIsWineCache.SystemIsWine;
	#else
		MPT_UNREFERENCED_PARAMETER(allowDetection);
		return false;
	#endif
}

void PreventWineDetection()
{
	SystemIsWine(false);
}

bool IsOriginal()
{
	return mpt::Windows::Version::Current().IsWindows() && !SystemIsWine();
}

bool IsWine()
{
	return mpt::Windows::Version::Current().IsWindows() && SystemIsWine();
}


#endif // MODPLUG_TRACKER


} // namespace Windows
} // namespace mpt



#if defined(MODPLUG_TRACKER)

namespace mpt
{
namespace Wine
{


Version::Version()
	: valid(false)
	, vmajor(0)
	, vminor(0)
	, vupdate(0)
{
	return;
}


Version::Version(const mpt::ustring &rawVersion)
	: valid(false)
	, vmajor(0)
	, vminor(0)
	, vupdate(0)
{
	if(rawVersion.empty())
	{
		return;
	}
	std::vector<uint8> version = mpt::String::Split<uint8>(rawVersion, U_("."));
	if(version.size() < 2)
	{
		return;
	}
	mpt::ustring parsedVersion = mpt::String::Combine(version, U_("."));
	std::size_t len = std::min(parsedVersion.length(), rawVersion.length());
	if(len == 0)
	{
		return;
	}
	if(parsedVersion.substr(0, len) != rawVersion.substr(0, len))
	{
		return;
	}
	valid = true;
	vmajor = version[0];
	vminor = version[1];
	vupdate = (version.size() >= 3) ? version[2] : 0;
}


Version::Version(uint8 vmajor, uint8 vminor, uint8 vupdate)
	: valid((vmajor > 0) || (vminor > 0) || (vupdate > 0)) 
	, vmajor(vmajor)
	, vminor(vminor)
	, vupdate(vupdate)
{
	return;
}


mpt::Wine::Version Version::FromInteger(uint32 version)
{
	mpt::Wine::Version result;
	result.valid = (version <= 0xffffff);
	result.vmajor = static_cast<uint8>(version >> 16);
	result.vminor = static_cast<uint8>(version >> 8);
	result.vupdate = static_cast<uint8>(version >> 0);
	return result;
}


bool Version::IsValid() const
{
	return valid;
}


mpt::ustring Version::AsString() const
{
	return mpt::ufmt::dec(vmajor) + U_(".") + mpt::ufmt::dec(vminor) + U_(".") + mpt::ufmt::dec(vupdate);
}


uint32 Version::AsInteger() const
{
	uint32 version = 0;
	version |= static_cast<uint32>(vmajor) << 16;
	version |= static_cast<uint32>(vminor) << 8;
	version |= static_cast<uint32>(vupdate) << 0;
	return version;
}


bool Version::IsBefore(mpt::Wine::Version other) const
{
	if(!IsValid())
	{
		return false;
	}
	return (AsInteger() < other.AsInteger());
}


bool Version::IsAtLeast(mpt::Wine::Version other) const
{
	if(!IsValid())
	{
		return false;
	}
	return (AsInteger() >= other.AsInteger());
}


uint8 Version::GetMajor() const
{
	return vmajor;
}

uint8 Version::GetMinor() const
{
	return vminor;
}

uint8 Version::GetUpdate() const
{
	return vupdate;
}


mpt::Wine::Version GetMinimumWineVersion()
{
	mpt::Wine::Version minimumWineVersion = mpt::Wine::Version(0,0,0);
	#if MPT_OS_WINDOWS && MPT_COMPILER_MSVC
		minimumWineVersion = mpt::Wine::Version(1,8,0);
	#endif
	return minimumWineVersion;
}


VersionContext::VersionContext()
	: m_IsWine(false)
	, m_HostClass(mpt::OS::Class::Unknown)
{
	#if MPT_OS_WINDOWS
		m_IsWine = mpt::Windows::IsWine();
		if(!m_IsWine)
		{
			return;
		}
		m_NTDLL = mpt::Library(mpt::LibraryPath::FullPath(P_("ntdll.dll")));
		if(m_NTDLL.IsValid())
		{
			const char * (__cdecl * wine_get_version)(void) = nullptr;
			const char * (__cdecl * wine_get_build_id)(void) = nullptr;
			void (__cdecl * wine_get_host_version)(const char * *, const char * *) = nullptr;
			m_NTDLL.Bind(wine_get_version, "wine_get_version");
			m_NTDLL.Bind(wine_get_build_id, "wine_get_build_id");
			m_NTDLL.Bind(wine_get_host_version, "wine_get_host_version");
			const char * wine_version = nullptr;
			const char * wine_build_id = nullptr;
			const char * wine_host_sysname = nullptr;
			const char * wine_host_release = nullptr;
			wine_version = wine_get_version ? wine_get_version() : "";
			wine_build_id = wine_get_build_id ? wine_get_build_id() : "";
			if(wine_get_host_version)
			{
				wine_get_host_version(&wine_host_sysname, &wine_host_release);
			}
			m_RawVersion = wine_version ? wine_version : "";
			m_RawBuildID = wine_build_id ? wine_build_id : "";
			m_RawHostSysName = wine_host_sysname ? wine_host_sysname : "";
			m_RawHostRelease = wine_host_release ? wine_host_release : "";
		}
		m_Version = mpt::Wine::Version(mpt::ToUnicode(mpt::Charset::UTF8, m_RawVersion));
		m_HostClass = mpt::OS::GetClassFromSysname(mpt::ToUnicode(mpt::Charset::UTF8, m_RawHostSysName));
	#endif // MPT_OS_WINDOWS
}


} // namespace Wine
} // namespace mpt

#endif // MODPLUG_TRACKER



OPENMPT_NAMESPACE_END
