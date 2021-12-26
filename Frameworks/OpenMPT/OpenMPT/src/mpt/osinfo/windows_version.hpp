/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_OSINFO_WINDOWS_VERSION_HPP
#define MPT_OSINFO_WINDOWS_VERSION_HPP



#include "mpt/base/detect.hpp"
#include "mpt/base/integer.hpp"
#include "mpt/base/namespace.hpp"

#if MPT_OS_WINDOWS
#include <windows.h>
#endif // MPT_OS_WINDOWS



namespace mpt {
inline namespace MPT_INLINE_NS {



namespace osinfo {

namespace windows {



class Version {

public:
	enum Number : uint64
	{
		WinNT4 = 0x0000000400000000ull,
		Win2000 = 0x0000000500000000ull,
		WinXP = 0x0000000500000001ull,
		WinXP64 = 0x0000000500000002ull,
		WinVista = 0x0000000600000000ull,
		Win7 = 0x0000000600000001ull,
		Win8 = 0x0000000600000002ull,
		Win81 = 0x0000000600000003ull,
		Win10 = 0x0000000a00000000ull,
		WinNewer = Win10 + 1ull
	};

	struct System {
		uint32 Major = 0;
		uint32 Minor = 0;
		System() = default;
		constexpr System(Number number) noexcept
			: Major(static_cast<uint32>((static_cast<uint64>(number) >> 32) & 0xffffffffu))
			, Minor(static_cast<uint32>((static_cast<uint64>(number) >> 0) & 0xffffffffu)) {
		}
		explicit constexpr System(uint64 number) noexcept
			: Major(static_cast<uint32>((number >> 32) & 0xffffffffu))
			, Minor(static_cast<uint32>((number >> 0) & 0xffffffffu)) {
		}
		explicit constexpr System(uint32 major, uint32 minor) noexcept
			: Major(major)
			, Minor(minor) {
		}
		constexpr operator uint64() const noexcept {
			return (static_cast<uint64>(Major) << 32) | (static_cast<uint64>(Minor) << 0);
		}
	};

	struct ServicePack {
		uint16 Major = 0;
		uint16 Minor = 0;
		ServicePack() = default;
		explicit constexpr ServicePack(uint16 major, uint16 minor) noexcept
			: Major(major)
			, Minor(minor) {
		}
		constexpr bool HasServicePack() const noexcept {
			return Major != 0 || Minor != 0;
		}
		constexpr operator uint32() const noexcept {
			return (static_cast<uint32>(Major) << 16) | (static_cast<uint32>(Minor) << 0);
		}
	};

	typedef uint32 Build;

	typedef uint32 TypeId;

protected:
	bool m_SystemIsWindows;

	System m_System;
	ServicePack m_ServicePack;
	Build m_Build;
	TypeId m_Type;

protected:
	Version() noexcept
		: m_SystemIsWindows(false)
		, m_System()
		, m_ServicePack()
		, m_Build()
		, m_Type() {
	}

public:
	static Version NoWindows() noexcept {
		return Version();
	}

	Version(mpt::osinfo::windows::Version::System system, mpt::osinfo::windows::Version::ServicePack servicePack, mpt::osinfo::windows::Version::Build build, mpt::osinfo::windows::Version::TypeId type) noexcept
		: m_SystemIsWindows(true)
		, m_System(system)
		, m_ServicePack(servicePack)
		, m_Build(build)
		, m_Type(type) {
	}

protected:
#if MPT_OS_WINDOWS

	static mpt::osinfo::windows::Version VersionFromNTDDI_VERSION() noexcept {
		// Initialize to used SDK version
		mpt::osinfo::windows::Version::System System =
#if NTDDI_VERSION >= 0x0A000000 // NTDDI_WIN10
			mpt::osinfo::windows::Version::Win10
#elif NTDDI_VERSION >= 0x06030000 // NTDDI_WINBLUE
			mpt::osinfo::windows::Version::Win81
#elif NTDDI_VERSION >= 0x06020000 // NTDDI_WIN8
			mpt::osinfo::windows::Version::Win8
#elif NTDDI_VERSION >= 0x06010000 // NTDDI_WIN7
			mpt::osinfo::windows::Version::Win7
#elif NTDDI_VERSION >= 0x06000000 // NTDDI_VISTA
			mpt::osinfo::windows::Version::WinVista
#elif NTDDI_VERSION >= 0x05020000 // NTDDI_WS03
			mpt::osinfo::windows::Version::WinXP64
#elif NTDDI_VERSION >= NTDDI_WINXP
			mpt::osinfo::windows::Version::WinXP
#elif NTDDI_VERSION >= NTDDI_WIN2K
			mpt::osinfo::windows::Version::Win2000
#else
			mpt::osinfo::windows::Version::WinNT4
#endif
			;
		return mpt::osinfo::windows::Version(System, mpt::osinfo::windows::Version::ServicePack(((NTDDI_VERSION & 0xffffu) >> 8) & 0xffu, ((NTDDI_VERSION & 0xffffu) >> 0) & 0xffu), 0, 0);
	}

	static mpt::osinfo::windows::Version::System SystemVersionFrom_WIN32_WINNT() noexcept {
#if defined(_WIN32_WINNT)
		return mpt::osinfo::windows::Version::System((static_cast<uint64>(_WIN32_WINNT) & 0xff00u) >> 8, (static_cast<uint64>(_WIN32_WINNT) & 0x00ffu) >> 0);
#else
		return mpt::osinfo::windows::Version::System();
#endif
	}

	static mpt::osinfo::windows::Version GatherWindowsVersion() noexcept {
#if MPT_OS_WINDOWS_WINRT
		return VersionFromNTDDI_VERSION();
#else // !MPT_OS_WINDOWS_WINRT
		OSVERSIONINFOEXW versioninfoex{};
		versioninfoex.dwOSVersionInfoSize = sizeof(versioninfoex);
#if MPT_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable : 4996)  // 'GetVersionExW': was declared deprecated
#pragma warning(disable : 28159) // Consider using 'IsWindows*' instead of 'GetVersionExW'. Reason: Deprecated. Use VerifyVersionInfo* or IsWindows* macros from VersionHelpers.
#endif                           // MPT_COMPILER_MSVC
#if MPT_COMPILER_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif // MPT_COMPILER_CLANG
		if (GetVersionExW((LPOSVERSIONINFOW)&versioninfoex) == FALSE) {
			return VersionFromNTDDI_VERSION();
		}
#if MPT_COMPILER_MSVC
#pragma warning(pop)
#endif // MPT_COMPILER_MSVC
#if MPT_COMPILER_CLANG
#pragma clang diagnostic pop
#endif                       // MPT_COMPILER_CLANG
		if (versioninfoex.dwPlatformId != VER_PLATFORM_WIN32_NT) {
			return VersionFromNTDDI_VERSION();
		}
		DWORD dwProductType = 0;
#if (_WIN32_WINNT >= 0x0600) // _WIN32_WINNT_VISTA
		dwProductType = PRODUCT_UNDEFINED;
		if (GetProductInfo(versioninfoex.dwMajorVersion, versioninfoex.dwMinorVersion, versioninfoex.wServicePackMajor, versioninfoex.wServicePackMinor, &dwProductType) == FALSE) {
			dwProductType = PRODUCT_UNDEFINED;
		}
#endif
		return mpt::osinfo::windows::Version(
			mpt::osinfo::windows::Version::System(versioninfoex.dwMajorVersion, versioninfoex.dwMinorVersion),
			mpt::osinfo::windows::Version::ServicePack(versioninfoex.wServicePackMajor, versioninfoex.wServicePackMinor),
			versioninfoex.dwBuildNumber,
			dwProductType);
#endif // MPT_OS_WINDOWS_WINRT
	}

#endif // MPT_OS_WINDOWS

public:
	static mpt::osinfo::windows::Version Current() noexcept {
#if MPT_OS_WINDOWS
		return GatherWindowsVersion();
#else  // !MPT_OS_WINDOWS
		return mpt::osinfo::windows::Version::NoWindows();
#endif // MPT_OS_WINDOWS
	}

public:
	bool IsWindows() const noexcept {
		return m_SystemIsWindows;
	}

	bool IsBefore(mpt::osinfo::windows::Version::System version) const noexcept {
		if (!m_SystemIsWindows) {
			return false;
		}
		return m_System < version;
	}

	bool IsBefore(mpt::osinfo::windows::Version::System version, mpt::osinfo::windows::Version::ServicePack servicePack) const noexcept {
		if (!m_SystemIsWindows) {
			return false;
		}
		if (m_System > version) {
			return false;
		}
		if (m_System < version) {
			return true;
		}
		return m_ServicePack < servicePack;
	}

	bool IsBefore(mpt::osinfo::windows::Version::System version, mpt::osinfo::windows::Version::Build build) const noexcept {
		if (!m_SystemIsWindows) {
			return false;
		}
		if (m_System > version) {
			return false;
		}
		if (m_System < version) {
			return true;
		}
		return m_Build < build;
	}

	bool IsBefore(mpt::osinfo::windows::Version::System version, mpt::osinfo::windows::Version::ServicePack servicePack, mpt::osinfo::windows::Version::Build build) const noexcept {
		if (!m_SystemIsWindows) {
			return false;
		}
		if (m_System > version) {
			return false;
		}
		if (m_System < version) {
			return true;
		}
		if (m_ServicePack > servicePack) {
			return false;
		}
		if (m_ServicePack < servicePack) {
			return true;
		}
		return m_Build < build;
	}

	bool IsAtLeast(mpt::osinfo::windows::Version::System version) const noexcept {
		if (!m_SystemIsWindows) {
			return false;
		}
		return m_System >= version;
	}

	bool IsAtLeast(mpt::osinfo::windows::Version::System version, mpt::osinfo::windows::Version::ServicePack servicePack) const noexcept {
		if (!m_SystemIsWindows) {
			return false;
		}
		if (m_System < version) {
			return false;
		}
		if (m_System > version) {
			return true;
		}
		return m_ServicePack >= servicePack;
	}

	bool IsAtLeast(mpt::osinfo::windows::Version::System version, mpt::osinfo::windows::Version::Build build) const noexcept {
		if (!m_SystemIsWindows) {
			return false;
		}
		if (m_System < version) {
			return false;
		}
		if (m_System > version) {
			return true;
		}
		return m_Build >= build;
	}

	bool IsAtLeast(mpt::osinfo::windows::Version::System version, mpt::osinfo::windows::Version::ServicePack servicePack, mpt::osinfo::windows::Version::Build build) const noexcept {
		if (!m_SystemIsWindows) {
			return false;
		}
		if (m_System < version) {
			return false;
		}
		if (m_System > version) {
			return true;
		}
		if (m_ServicePack < servicePack) {
			return false;
		}
		if (m_ServicePack > servicePack) {
			return true;
		}
		return m_Build >= build;
	}

	mpt::osinfo::windows::Version::System GetSystem() const noexcept {
		return m_System;
	}

	mpt::osinfo::windows::Version::ServicePack GetServicePack() const noexcept {
		return m_ServicePack;
	}

	mpt::osinfo::windows::Version::Build GetBuild() const noexcept {
		return m_Build;
	}

	mpt::osinfo::windows::Version::TypeId GetTypeId() const noexcept {
		return m_Type;
	}

}; // class Version



namespace wine {

class version {
protected:
	bool valid = false;
	uint8 vmajor = 0;
	uint8 vminor = 0;
	uint8 vupdate = 0;

public:
	version() {
		return;
	}
	version(uint8 vmajor_, uint8 vminor_, uint8 vupdate_)
		: valid(true)
		, vmajor(vmajor_)
		, vminor(vminor_)
		, vupdate(vupdate_) {
		return;
	}

public:
	bool IsValid() const {
		return true;
	}

private:
	static mpt::osinfo::windows::wine::version FromInteger(uint32 version) {
		mpt::osinfo::windows::wine::version result;
		result.valid = (version <= 0xffffff);
		result.vmajor = static_cast<uint8>(version >> 16);
		result.vminor = static_cast<uint8>(version >> 8);
		result.vupdate = static_cast<uint8>(version >> 0);
		return result;
	}
	uint32 AsInteger() const {
		uint32 version = 0;
		version |= static_cast<uint32>(vmajor) << 16;
		version |= static_cast<uint32>(vminor) << 8;
		version |= static_cast<uint32>(vupdate) << 0;
		return version;
	}

public:
	bool IsBefore(mpt::osinfo::windows::wine::version other) const {
		if (!IsValid()) {
			return false;
		}
		return (AsInteger() < other.AsInteger());
	}
	bool IsAtLeast(mpt::osinfo::windows::wine::version other) const {
		if (!IsValid()) {
			return false;
		}
		return (AsInteger() >= other.AsInteger());
	}
	uint8 GetMajor() const {
		return vmajor;
	}
	uint8 GetMinor() const {
		return vminor;
	}
	uint8 GetUpdate() const {
		return vupdate;
	}
};

} // namespace wine



} // namespace windows

} // namespace osinfo



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_OSINFO_WINDOWS_VERSION_HPP
