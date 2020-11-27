/*
 * mptUUID.h
 * ---------
 * Purpose: UUID utility functions.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"


#include "Endianness.h"

#include <stdexcept>

#if MPT_OS_WINDOWS
#if defined(MODPLUG_TRACKER) || defined(MPT_WITH_DMO)
#include <guiddef.h>
#include <rpc.h>
#endif // MODPLUG_TRACKER || MPT_WITH_DMO
#endif // MPT_OS_WINDOWS


OPENMPT_NAMESPACE_BEGIN

#if MPT_OS_WINDOWS

namespace Util
{

#if defined(MODPLUG_TRACKER) || defined(MPT_WITH_DMO)

// COM CLSID<->string conversion
// A CLSID string is not necessarily a standard UUID string,
// it might also be a symbolic name for the interface.
// (see CLSIDFromString ( http://msdn.microsoft.com/en-us/library/windows/desktop/ms680589%28v=vs.85%29.aspx ))
mpt::winstring CLSIDToString(CLSID clsid);
CLSID StringToCLSID(const mpt::winstring &str);
bool VerifyStringToCLSID(const mpt::winstring &str, CLSID &clsid);
bool IsCLSID(const mpt::winstring &str);

// COM IID<->string conversion
IID StringToIID(const mpt::winstring &str);
mpt::winstring IIDToString(IID iid);

// General GUID<->string conversion.
// The string must/will be in standard GUID format: {4F9A455D-E7EF-4367-B2F0-0C83A38A5C72}
GUID StringToGUID(const mpt::winstring &str);
mpt::winstring GUIDToString(GUID guid);

// Create a COM GUID
GUID CreateGUID();

// Checks the UUID against the NULL UUID. Returns false if it is NULL, true otherwise.
bool IsValid(UUID uuid);

#endif // MODPLUG_TRACKER || MPT_WITH_DMO

} // namespace Util

#endif // MPT_OS_WINDOWS

// Microsoft on-disk layout
struct GUIDms
{
	uint32le Data1;
	uint16le Data2;
	uint16le Data3;
	uint64be Data4; // yes, big endian here
};
MPT_BINARY_STRUCT(GUIDms, 16)

// RFC binary format
struct UUIDbin
{
	uint32be Data1;
	uint16be Data2;
	uint16be Data3;
	uint64be Data4;
};
MPT_BINARY_STRUCT(UUIDbin, 16)

namespace mpt {

struct UUID
{
private:
	uint32 Data1;
	uint16 Data2;
	uint16 Data3;
	uint64 Data4;
public:
	MPT_CONSTEXPR11_FUN uint32 GetData1() const noexcept { return Data1; }
	MPT_CONSTEXPR11_FUN uint16 GetData2() const noexcept { return Data2; }
	MPT_CONSTEXPR11_FUN uint16 GetData3() const noexcept { return Data3; }
	MPT_CONSTEXPR11_FUN uint64 GetData4() const noexcept { return Data4; }
public:
	MPT_CONSTEXPR11_FUN uint64 GetData64_1() const noexcept { return (static_cast<uint64>(Data1) << 32) | (static_cast<uint64>(Data2) << 16) | (static_cast<uint64>(Data3) << 0); }
	MPT_CONSTEXPR11_FUN uint64 GetData64_2() const noexcept { return Data4; }
public:
	// xxxxxxxx-xxxx-Mmxx-Nnxx-xxxxxxxxxxxx
	// <--32-->-<16>-<16>-<-------64------>
	MPT_CONSTEXPR11_FUN bool IsNil() const noexcept { return (Data1 == 0) && (Data2 == 0) && (Data3 == 0) && (Data4 == 0); }
	MPT_CONSTEXPR11_FUN bool IsValid() const noexcept { return (Data1 != 0) || (Data2 != 0) || (Data3 != 0) || (Data4 != 0); }
	MPT_CONSTEXPR11_FUN uint8 Variant() const noexcept { return Nn() >> 4u; }
	MPT_CONSTEXPR11_FUN uint8 Version() const noexcept { return Mm() >> 4u; }
	MPT_CONSTEXPR11_FUN bool IsRFC4122() const noexcept { return (Variant() & 0xcu) == 0x8u; }
private:
	MPT_CONSTEXPR11_FUN uint8 Mm() const noexcept { return static_cast<uint8>((Data3 >> 8) & 0xffu); }
	MPT_CONSTEXPR11_FUN uint8 Nn() const noexcept { return static_cast<uint8>((Data4 >> 56) & 0xffu); }
	void MakeRFC4122(uint8 version) noexcept;
public:
#if MPT_OS_WINDOWS && (defined(MODPLUG_TRACKER) || defined(MPT_WITH_DMO))
	explicit UUID(::UUID uuid);
	operator ::UUID () const;
#endif // MPT_OS_WINDOWS && (MODPLUG_TRACKER || MPT_WITH_DMO)
private:
	static MPT_CONSTEXPR11_FUN uint8 NibbleFromChar(char x)
	{
		return
			('0' <= x && x <= '9') ? static_cast<uint8>(x - '0' +  0) :
			('a' <= x && x <= 'z') ? static_cast<uint8>(x - 'a' + 10) :
			('A' <= x && x <= 'Z') ? static_cast<uint8>(x - 'A' + 10) :
			mpt::constexpr_throw<uint8>(std::domain_error(""));
	}
	static MPT_CONSTEXPR11_FUN uint8 ByteFromHex(char x, char y)
	{
		return static_cast<uint8>(uint8(0)
			| (NibbleFromChar(x) << 4)
			| (NibbleFromChar(y) << 0)
			);
	}
	static MPT_CONSTEXPR11_FUN uint16 ParseHex16(const char * str)
	{
		return static_cast<uint16>(uint16(0)
			| (static_cast<uint16>(ByteFromHex(str[0], str[1])) << 8)
			| (static_cast<uint16>(ByteFromHex(str[2], str[3])) << 0)
			);
	}
	static MPT_CONSTEXPR11_FUN uint32 ParseHex32(const char * str)
	{
		return static_cast<uint32>(uint32(0)
			| (static_cast<uint32>(ByteFromHex(str[0], str[1])) << 24)
			| (static_cast<uint32>(ByteFromHex(str[2], str[3])) << 16)
			| (static_cast<uint32>(ByteFromHex(str[4], str[5])) <<  8)
			| (static_cast<uint32>(ByteFromHex(str[6], str[7])) <<  0)
			);
	}
public:
	static MPT_CONSTEXPR11_FUN UUID ParseLiteral(const char * str, std::size_t len)
	{
		return
			(len == 36 && str[8] == '-' && str[13] == '-' && str[18] == '-' && str[23] == '-') ?
			mpt::UUID(
				ParseHex32(str + 0),
				ParseHex16(str + 9),
				ParseHex16(str + 14),
				uint64(0)
					| (static_cast<uint64>(ParseHex16(str + 19)) << 48)
					| (static_cast<uint64>(ParseHex16(str + 24)) << 32)
					| (static_cast<uint64>(ParseHex32(str + 28)) <<  0)
			)
			: mpt::constexpr_throw<mpt::UUID>(std::domain_error(""));
	}
public:
	MPT_CONSTEXPR11_FUN UUID() noexcept : Data1(0), Data2(0), Data3(0), Data4(0) { }
	MPT_CONSTEXPR11_FUN explicit UUID(uint32 Data1, uint16 Data2, uint16 Data3, uint64 Data4) noexcept : Data1(Data1), Data2(Data2), Data3(Data3), Data4(Data4) { }
	explicit UUID(UUIDbin uuid);
	explicit UUID(GUIDms guid);
	operator UUIDbin () const;
	operator GUIDms () const;
public:
	// Create a UUID
	static UUID Generate();
	// Create a UUID that contains local, traceable information.
	// Safe for local use. May be faster.
	static UUID GenerateLocalUseOnly();
	// Create a RFC4122 Random UUID.
	static UUID RFC4122Random();
public:
	// General UUID<->string conversion.
	// The string must/will be in standard UUID format: 4f9a455d-e7ef-4367-b2f0-0c83a38a5c72
	static UUID FromString(const mpt::ustring &str);
	mpt::ustring ToUString() const;
};

MPT_CONSTEXPR11_FUN bool operator==(const mpt::UUID & a, const mpt::UUID & b) noexcept
{
	return (a.GetData1() == b.GetData1()) && (a.GetData2() == b.GetData2()) && (a.GetData3() == b.GetData3()) && (a.GetData4() == b.GetData4());
}
MPT_CONSTEXPR11_FUN bool operator!=(const mpt::UUID & a, const mpt::UUID & b) noexcept
{
	return (a.GetData1() != b.GetData1()) || (a.GetData2() != b.GetData2()) || (a.GetData3() != b.GetData3()) || (a.GetData4() != b.GetData4());	
}

} // namespace mpt


MPT_CONSTEXPR11_FUN mpt::UUID operator "" _uuid (const char * str, std::size_t len)
{
	return mpt::UUID::ParseLiteral(str, len);
}


OPENMPT_NAMESPACE_END
