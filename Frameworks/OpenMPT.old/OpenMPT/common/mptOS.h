/*
 * mptOS.h
 * -------
 * Purpose: Operating system version information.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"


#include "mptLibrary.h"


OPENMPT_NAMESPACE_BEGIN


#if defined(MODPLUG_TRACKER)

namespace mpt
{
namespace OS
{

enum class Class
{
	Unknown,
	Windows,
	Linux,
	Darwin,
	BSD,
	Haiku,
	DOS,
};

mpt::OS::Class GetClassFromSysname(mpt::ustring sysname);

mpt::OS::Class GetClass();

}  // namespace OS
}  // namespace mpt

#endif // MODPLUG_TRACKER


namespace mpt
{
namespace Windows
{


class Version
{

public:

	enum Number : uint64
	{
		WinNT4   = 0x0000000400000000ull,
		Win2000  = 0x0000000500000000ull,
		WinXP    = 0x0000000500000001ull,
		WinXP64  = 0x0000000500000002ull,
		WinVista = 0x0000000600000000ull,
		Win7     = 0x0000000600000001ull,
		Win8     = 0x0000000600000002ull,
		Win81    = 0x0000000600000003ull,
		Win10    = 0x0000000a00000000ull,
		WinNewer = Win10 + 1ull
	};

	struct System
	{
		uint32 Major = 0;
		uint32 Minor = 0;
		System() = default;
		constexpr System(Number number) noexcept
			: Major(static_cast<uint32>((static_cast<uint64>(number) >> 32) & 0xffffffffu))
			, Minor(static_cast<uint32>((static_cast<uint64>(number) >>  0) & 0xffffffffu))
		{
		}
		explicit constexpr System(uint64 number) noexcept
			: Major(static_cast<uint32>((number >> 32) & 0xffffffffu))
			, Minor(static_cast<uint32>((number >>  0) & 0xffffffffu))
		{
		}
		explicit constexpr System(uint32 major, uint32 minor) noexcept
			: Major(major)
			, Minor(minor)
		{
		}
		constexpr operator uint64 () const noexcept
		{
			return (static_cast<uint64>(Major) << 32) | (static_cast<uint64>(Minor) << 0);
		}
	};

	struct ServicePack
	{
		uint16 Major = 0;
		uint16 Minor = 0;
		ServicePack() = default;
		explicit constexpr ServicePack(uint16 major, uint16 minor) noexcept
			: Major(major)
			, Minor(minor)
		{
		}
		constexpr bool HasServicePack() const noexcept
		{
			return Major != 0 || Minor != 0;
		}
		constexpr operator uint32 () const noexcept
		{
			return (static_cast<uint32>(Major) << 16) | (static_cast<uint32>(Minor) << 0);
		}
	};

	typedef uint32 Build;

	typedef uint32 TypeId;

	static mpt::ustring VersionToString(mpt::Windows::Version::System version);

private:

	bool m_SystemIsWindows;

	System m_System;
	ServicePack m_ServicePack;
	Build m_Build;
	TypeId m_Type;

private:

	Version() noexcept;

public:

	static Version NoWindows() noexcept;

	Version(mpt::Windows::Version::System system, mpt::Windows::Version::ServicePack servicePack, mpt::Windows::Version::Build build, mpt::Windows::Version::TypeId type) noexcept;

public:

	static mpt::Windows::Version Current() noexcept;

public:

	bool IsWindows() const noexcept;

	bool IsBefore(mpt::Windows::Version::System version) const noexcept;
	bool IsBefore(mpt::Windows::Version::System version, mpt::Windows::Version::ServicePack servicePack) const noexcept;
	bool IsBefore(mpt::Windows::Version::System version, mpt::Windows::Version::Build build) const noexcept;

	bool IsAtLeast(mpt::Windows::Version::System version) const noexcept;
	bool IsAtLeast(mpt::Windows::Version::System version, mpt::Windows::Version::ServicePack servicePack) const noexcept;
	bool IsAtLeast(mpt::Windows::Version::System version, mpt::Windows::Version::Build build) const noexcept;

	mpt::Windows::Version::System GetSystem() const noexcept;
	mpt::Windows::Version::ServicePack GetServicePack() const noexcept;
	mpt::Windows::Version::Build GetBuild() const noexcept;
	mpt::Windows::Version::TypeId GetTypeId() const noexcept;

	mpt::ustring GetName() const;
#ifdef MODPLUG_TRACKER
	mpt::ustring GetNameShort() const;
#endif // MODPLUG_TRACKER

public:

	static mpt::Windows::Version::System GetMinimumKernelLevel() noexcept;
	static mpt::Windows::Version::System GetMinimumAPILevel() noexcept;

}; // class Version

#if defined(MODPLUG_TRACKER) && MPT_OS_WINDOWS

enum class Architecture
{
	unknown = -1,

	x86     = 0x0401,
	amd64   = 0x0801,
	arm     = 0x0402,
	arm64   = 0x0802,

	mips    = 0x0403,
	ppc     = 0x0404,
	shx     = 0x0405,

	alpha   = 0x0406,
	alpha64 = 0x0806,

	ia64    = 0x0807,
};

enum class EmulationLevel
{
	Native,
	Virtual,
	Hardware,
	Software,
	NA,
};

int Bitness(Architecture arch) noexcept;

mpt::ustring Name(Architecture arch);

Architecture GetHostArchitecture() noexcept;
Architecture GetProcessArchitecture() noexcept;

EmulationLevel HostCanRun(Architecture host, Architecture process) noexcept;

std::vector<Architecture> GetSupportedProcessArchitectures(Architecture host);

uint64 GetSystemMemorySize();

#endif // MODPLUG_TRACKER && MPT_OS_WINDOWS


#if defined(MODPLUG_TRACKER)

void PreventWineDetection();

bool IsOriginal();
bool IsWine();

#endif // MODPLUG_TRACKER

} // namespace Windows
} // namespace mpt


#if defined(MODPLUG_TRACKER)

namespace mpt
{

namespace Wine
{

class Version
{
private:
	bool valid;
	uint8 vmajor;
	uint8 vminor;
	uint8 vupdate;
public:
	Version();
	Version(uint8 vmajor, uint8 vminor, uint8 vupdate);
	explicit Version(const mpt::ustring &version);
public:
	bool IsValid() const;
	mpt::ustring AsString() const;
private:
	static mpt::Wine::Version FromInteger(uint32 version);
	uint32 AsInteger() const;
public:
	bool IsBefore(mpt::Wine::Version other) const;
	bool IsAtLeast(mpt::Wine::Version other) const;
	uint8 GetMajor() const;
	uint8 GetMinor() const;
	uint8 GetUpdate() const;
};

mpt::Wine::Version GetMinimumWineVersion();

class VersionContext
{
protected:
	bool m_IsWine;
	mpt::Library m_NTDLL;
	std::string m_RawVersion;
	std::string m_RawBuildID;
	std::string m_RawHostSysName;
	std::string m_RawHostRelease;
	mpt::Wine::Version m_Version;
	mpt::OS::Class m_HostClass;
public:
	VersionContext();
public:
	bool IsWine() const { return m_IsWine; }
	mpt::Library NTDLL() const { return m_NTDLL; }
	std::string RawVersion() const { return m_RawVersion; }
	std::string RawBuildID() const { return m_RawBuildID; }
	std::string RawHostSysName() const { return m_RawHostSysName; }
	std::string RawHostRelease() const { return m_RawHostRelease; }
	mpt::Wine::Version Version() const { return m_Version; }
	mpt::OS::Class HostClass() const { return m_HostClass; }
};

} // namespace Wine

} // namespace mpt

#endif // MODPLUG_TRACKER


OPENMPT_NAMESPACE_END
