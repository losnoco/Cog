/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_BASE_DETECT_OS_HPP
#define MPT_BASE_DETECT_OS_HPP



#define MPT_WIN_MAKE_VERSION(major, minor, sp, build) ((major << 24) + (minor << 16) + (sp << 8) + (build << 0))

// clang-format off

#define MPT_WIN_WIN32S   MPT_WIN_MAKE_VERSION(0x03, 0x00, 0x00, 0x00)

#define MPT_WIN_WIN95    MPT_WIN_MAKE_VERSION(0x04, 0x00, 0x00, 0x00)
#define MPT_WIN_WIN98    MPT_WIN_MAKE_VERSION(0x04, 0x10, 0x00, 0x00)
#define MPT_WIN_WINME    MPT_WIN_MAKE_VERSION(0x04, 0x90, 0x00, 0x00)

#define MPT_WIN_NT3      MPT_WIN_MAKE_VERSION(0x03, 0x00, 0x00, 0x00)
#define MPT_WIN_NT4      MPT_WIN_MAKE_VERSION(0x04, 0x00, 0x00, 0x00)
#define MPT_WIN_2000     MPT_WIN_MAKE_VERSION(0x05, 0x00, 0x00, 0x00)
#define MPT_WIN_2000SP1  MPT_WIN_MAKE_VERSION(0x05, 0x00, 0x01, 0x00)
#define MPT_WIN_2000SP2  MPT_WIN_MAKE_VERSION(0x05, 0x00, 0x02, 0x00)
#define MPT_WIN_2000SP3  MPT_WIN_MAKE_VERSION(0x05, 0x00, 0x03, 0x00)
#define MPT_WIN_2000SP4  MPT_WIN_MAKE_VERSION(0x05, 0x00, 0x04, 0x00)
#define MPT_WIN_XP       MPT_WIN_MAKE_VERSION(0x05, 0x01, 0x00, 0x00)
#define MPT_WIN_XPSP1    MPT_WIN_MAKE_VERSION(0x05, 0x01, 0x01, 0x00)
#define MPT_WIN_XPSP2    MPT_WIN_MAKE_VERSION(0x05, 0x01, 0x02, 0x00)
#define MPT_WIN_XPSP3    MPT_WIN_MAKE_VERSION(0x05, 0x01, 0x03, 0x00)
#define MPT_WIN_XPSP4    MPT_WIN_MAKE_VERSION(0x05, 0x01, 0x04, 0x00) // unused
#define MPT_WIN_XP64     MPT_WIN_MAKE_VERSION(0x05, 0x02, 0x00, 0x00) // unused
#define MPT_WIN_XP64SP1  MPT_WIN_MAKE_VERSION(0x05, 0x02, 0x01, 0x00)
#define MPT_WIN_XP64SP2  MPT_WIN_MAKE_VERSION(0x05, 0x02, 0x02, 0x00)
#define MPT_WIN_XP64SP3  MPT_WIN_MAKE_VERSION(0x05, 0x02, 0x03, 0x00) // unused
#define MPT_WIN_XP64SP4  MPT_WIN_MAKE_VERSION(0x05, 0x02, 0x04, 0x00) // unused
#define MPT_WIN_VISTA    MPT_WIN_MAKE_VERSION(0x06, 0x00, 0x00, 0x00)
#define MPT_WIN_VISTASP1 MPT_WIN_MAKE_VERSION(0x06, 0x00, 0x01, 0x00)
#define MPT_WIN_VISTASP2 MPT_WIN_MAKE_VERSION(0x06, 0x00, 0x02, 0x00)
#define MPT_WIN_VISTASP3 MPT_WIN_MAKE_VERSION(0x06, 0x00, 0x03, 0x00) // unused
#define MPT_WIN_VISTASP4 MPT_WIN_MAKE_VERSION(0x06, 0x00, 0x04, 0x00) // unused
#define MPT_WIN_7        MPT_WIN_MAKE_VERSION(0x06, 0x01, 0x00, 0x00)
#define MPT_WIN_8        MPT_WIN_MAKE_VERSION(0x06, 0x02, 0x00, 0x00)
#define MPT_WIN_81       MPT_WIN_MAKE_VERSION(0x06, 0x03, 0x00, 0x00)

#define MPT_WIN_10_PRE   MPT_WIN_MAKE_VERSION(0x06, 0x04, 0x00, 0x00)
#define MPT_WIN_10       MPT_WIN_MAKE_VERSION(0x0a, 0x00, 0x00, 0x00) // NTDDI_WIN10      1507
#define MPT_WIN_10_1511  MPT_WIN_MAKE_VERSION(0x0a, 0x00, 0x00, 0x01) // NTDDI_WIN10_TH2  1511
#define MPT_WIN_10_1607  MPT_WIN_MAKE_VERSION(0x0a, 0x00, 0x00, 0x02) // NTDDI_WIN10_RS1  1607
#define MPT_WIN_10_1703  MPT_WIN_MAKE_VERSION(0x0a, 0x00, 0x00, 0x03) // NTDDI_WIN10_RS2  1703
#define MPT_WIN_10_1709  MPT_WIN_MAKE_VERSION(0x0a, 0x00, 0x00, 0x04) // NTDDI_WIN10_RS3  1709
#define MPT_WIN_10_1803  MPT_WIN_MAKE_VERSION(0x0a, 0x00, 0x00, 0x05) // NTDDI_WIN10_RS4  1803
#define MPT_WIN_10_1809  MPT_WIN_MAKE_VERSION(0x0a, 0x00, 0x00, 0x06) // NTDDI_WIN10_RS5  1809
#define MPT_WIN_10_1903  MPT_WIN_MAKE_VERSION(0x0a, 0x00, 0x00, 0x07) // NTDDI_WIN10_19H1 1903/19H1
#define MPT_WIN_10_1909  MPT_WIN_MAKE_VERSION(0x0a, 0x00, 0x00, 0x08) // NTDDI_WIN10_VB   1909/19H2
#define MPT_WIN_10_2004  MPT_WIN_MAKE_VERSION(0x0a, 0x00, 0x00, 0x09) // NTDDI_WIN10_MN   2004/20H1
#define MPT_WIN_10_20H2  MPT_WIN_MAKE_VERSION(0x0a, 0x00, 0x00, 0x0a) // NTDDI_WIN10_FE   20H2
#define MPT_WIN_10_21H1  MPT_WIN_MAKE_VERSION(0x0a, 0x00, 0x00, 0x0b) // NTDDI_WIN10_CO   21H1
#define MPT_WIN_10_21H2  MPT_WIN_MAKE_VERSION(0x0a, 0x00, 0x00, 0x0c) // NTDDI_WIN10_NI   21H2
#define MPT_WIN_10_22H2  MPT_WIN_MAKE_VERSION(0x0a, 0x00, 0x00, 0x0d) // NTDDI_WIN10_CU   22H2

#define MPT_WIN_11       MPT_WIN_MAKE_VERSION(0x0a, 0x00, 0x00, 0x0e) // NTDDI_WIN11_ZN   21H2
#define MPT_WIN_11_22H2  MPT_WIN_MAKE_VERSION(0x0a, 0x00, 0x00, 0x0f) // NTDDI_WIN11_GA   22H2
#define MPT_WIN_11_23H2  MPT_WIN_MAKE_VERSION(0x0a, 0x00, 0x00, 0x10) // NTDDI_WIN11_GE   23H2

// MPT_WIN_API_DESKTOP     : Windows 8/10 Desktop Application (Win32)
// MPT_WIN_API_UNIVERSAL   : Windows 10 Store App / Universal App
// MPT_WIN_API_STORE_PC    : Windows 8 Store Desktop App
// MPT_WIN_API_STORE_PHONE : Windows 8 Store Phone App

// clang-format on



// The order of the checks matters!
#if defined(__DJGPP__)
#define MPT_OS_DJGPP 1


#elif defined(__EMSCRIPTEN__)
#define MPT_OS_EMSCRIPTEN 1
#if !defined(__EMSCRIPTEN_major__) || !defined(__EMSCRIPTEN_minor__) || !defined(__EMSCRIPTEN_tiny__)
#include <emscripten/version.h>
#endif
#if defined(__EMSCRIPTEN_major__) && defined(__EMSCRIPTEN_minor__)
#if (__EMSCRIPTEN_major__ > 3)
// ok
#elif (__EMSCRIPTEN_major__ == 3) && (__EMSCRIPTEN_minor__ > 1)
// ok
#elif (__EMSCRIPTEN_major__ == 3) && (__EMSCRIPTEN_minor__ == 1) && (__EMSCRIPTEN_tiny__ >= 1)
// ok
#else
#error "Emscripten >= 3.1.1 is required."
#endif
#endif


#elif defined(_WIN32)
#define MPT_OS_WINDOWS 1
#if !defined(_WIN32_WINDOWS) && !defined(WINVER)
// include modern SDK version header if not targeting Win9x
#include <sdkddkver.h>
#ifdef _WIN32_WINNT_NT4
static_assert((_WIN32_WINNT_NT4 << 16) == MPT_WIN_NT4);
#endif
#ifdef _WIN32_WINNT_WIN2K
static_assert((_WIN32_WINNT_WIN2K << 16) == MPT_WIN_2000);
#endif
#ifdef _WIN32_WINNT_WINXP
static_assert((_WIN32_WINNT_WINXP << 16) == MPT_WIN_XP);
#endif
#ifdef _WIN32_WINNT_WS03
static_assert((_WIN32_WINNT_WS03 << 16) == MPT_WIN_XP64);
#endif
#ifdef _WIN32_WINNT_VISTA
static_assert((_WIN32_WINNT_VISTA << 16) == MPT_WIN_VISTA);
#endif
#ifdef _WIN32_WINNT_WIN7
static_assert((_WIN32_WINNT_WIN7 << 16) == MPT_WIN_7);
#endif
#ifdef _WIN32_WINNT_WIN8
static_assert((_WIN32_WINNT_WIN8 << 16) == MPT_WIN_8);
#endif
#ifdef _WIN32_WINNT_WINBLUE
static_assert((_WIN32_WINNT_WINBLUE << 16) == MPT_WIN_81);
#endif
#ifdef _WIN32_WINNT_WIN10
static_assert((_WIN32_WINNT_WIN10 << 16) == MPT_WIN_10);
#endif
#ifdef NTDDI_WIN4
static_assert(NTDDI_WIN4 == MPT_WIN_NT4);
#endif
#ifdef NTDDI_WIN2K
static_assert(NTDDI_WIN2K == MPT_WIN_2000);
#endif
#ifdef NTDDI_WIN2KSP1
static_assert(NTDDI_WIN2KSP1 == MPT_WIN_2000SP1);
#endif
#ifdef NTDDI_WIN2KSP2
static_assert(NTDDI_WIN2KSP2 == MPT_WIN_2000SP2);
#endif
#ifdef NTDDI_WIN2KSP3
static_assert(NTDDI_WIN2KSP3 == MPT_WIN_2000SP3);
#endif
#ifdef NTDDI_WIN2KSP4
static_assert(NTDDI_WIN2KSP4 == MPT_WIN_2000SP4);
#endif
#ifdef NTDDI_WINXP
static_assert(NTDDI_WINXP == MPT_WIN_XP);
#endif
#ifdef NTDDI_WINXPSP1
static_assert(NTDDI_WINXPSP1 == MPT_WIN_XPSP1);
#endif
#ifdef NTDDI_WINXPSP2
static_assert(NTDDI_WINXPSP2 == MPT_WIN_XPSP2);
#endif
#ifdef NTDDI_WINXPSP3
static_assert(NTDDI_WINXPSP3 == MPT_WIN_XPSP3);
#endif
#ifdef NTDDI_WINXPSP4
static_assert(NTDDI_WINXPSP4 == MPT_WIN_XPSP4);
#endif
#ifdef NTDDI_WS03
static_assert(NTDDI_WS03 == MPT_WIN_XP64);
#endif
#ifdef NTDDI_WS03SP1
static_assert(NTDDI_WS03SP1 == MPT_WIN_XP64SP1);
#endif
#ifdef NTDDI_WS03SP2
static_assert(NTDDI_WS03SP2 == MPT_WIN_XP64SP2);
#endif
#ifdef NTDDI_WS03SP3
static_assert(NTDDI_WS03SP3 == MPT_WIN_XP64SP3);
#endif
#ifdef NTDDI_WS03SP4
static_assert(NTDDI_WS03SP4 == MPT_WIN_XP64SP4);
#endif
#ifdef NTDDI_VISTA
static_assert(NTDDI_VISTA == MPT_WIN_VISTA);
#endif
#ifdef NTDDI_VISTASP1
static_assert(NTDDI_VISTASP1 == MPT_WIN_VISTASP1);
#endif
#ifdef NTDDI_VISTASP2
static_assert(NTDDI_VISTASP2 == MPT_WIN_VISTASP2);
#endif
#ifdef NTDDI_VISTASP3
static_assert(NTDDI_VISTASP3 == MPT_WIN_VISTASP3);
#endif
#ifdef NTDDI_VISTASP4
static_assert(NTDDI_VISTASP4 == MPT_WIN_VISTASP4);
#endif
#ifdef NTDDI_WIN7
static_assert(NTDDI_WIN7 == MPT_WIN_7);
#endif
#ifdef NTDDI_WIN8
static_assert(NTDDI_WIN8 == MPT_WIN_8);
#endif
#ifdef NTDDI_WINBLUE
static_assert(NTDDI_WINBLUE == MPT_WIN_81);
#endif
#ifdef NTDDI_WIN10
static_assert(NTDDI_WIN10 == MPT_WIN_10);
#endif
#ifdef NTDDI_WIN10_TH2
static_assert(NTDDI_WIN10_TH2 == MPT_WIN_10_1511);
#endif
#ifdef NTDDI_WIN10_RS1
static_assert(NTDDI_WIN10_RS1 == MPT_WIN_10_1607);
#endif
#ifdef NTDDI_WIN10_RS2
static_assert(NTDDI_WIN10_RS2 == MPT_WIN_10_1703);
#endif
#ifdef NTDDI_WIN10_RS3
static_assert(NTDDI_WIN10_RS3 == MPT_WIN_10_1709);
#endif
#ifdef NTDDI_WIN10_RS4
static_assert(NTDDI_WIN10_RS4 == MPT_WIN_10_1803);
#endif
#ifdef NTDDI_WIN10_RS5
static_assert(NTDDI_WIN10_RS5 == MPT_WIN_10_1809);
#endif
#ifdef NTDDI_WIN10_19H1
static_assert(NTDDI_WIN10_19H1 == MPT_WIN_10_1903);
#endif
#ifdef NTDDI_WIN10_VB
static_assert(NTDDI_WIN10_VB == MPT_WIN_10_1909);
#endif
#ifdef NTDDI_WIN10_MN
static_assert(NTDDI_WIN10_MN == MPT_WIN_10_2004);
#endif
#ifdef NTDDI_WIN10_FE
static_assert(NTDDI_WIN10_FE == MPT_WIN_10_20H2);
#endif
#ifdef NTDDI_WIN10_CO
static_assert(NTDDI_WIN10_CO == MPT_WIN_10_21H1);
#endif
#ifdef NTDDI_WIN10_NI
static_assert(NTDDI_WIN10_NI == MPT_WIN_10_21H2);
#endif
#ifdef NTDDI_WIN10_CU
static_assert(NTDDI_WIN10_CU == MPT_WIN_10_22H2);
#endif
#ifdef NTDDI_WIN11_ZN
static_assert(NTDDI_WIN11_ZN == MPT_WIN_11);
#endif
#ifdef NTDDI_WIN11_GA
static_assert(NTDDI_WIN11_GA == MPT_WIN_11_22H2);
#endif
#ifdef NTDDI_WIN11_GE
static_assert(NTDDI_WIN11_GE == MPT_WIN_11_23H2);
#endif
#endif
#if defined(WINAPI_FAMILY)
#include <winapifamily.h>
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#define MPT_OS_WINDOWS_WINRT 0
#else
#define MPT_OS_WINDOWS_WINRT 1
#endif
#define MPT_WIN_API_DESKTOP     WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#define MPT_WIN_API_UNIVERSAL   WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_PC_APP)
#define MPT_WIN_API_STORE_PC    WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)
#define MPT_WIN_API_STORE_PHONE WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_PHONE_APP)
#else // !WINAPI_FAMILY
#define MPT_OS_WINDOWS_WINRT    0
#define MPT_WIN_API_DESKTOP     1
#define MPT_WIN_API_UNIVERSAL   0
#define MPT_WIN_API_STORE_PC    0
#define MPT_WIN_API_STORE_PHONE 0
#endif // WINAPI_FAMILY
#if defined(NTDDI_VERSION) || defined(_WIN32_WINNT)
#define MPT_OS_WINDOWS_WINNT 1
#define MPT_OS_WINDOWS_WIN9X 0
#define MPT_OS_WINDOWS_WIN32 0
#if defined(NTDDI_VERSION)
#define MPT_WIN_VERSION NTDDI_VERSION
#else
#define MPT_WIN_VERSION (_WIN32_WINNT << 16)
#endif
#elif defined(_WIN32_WINDOWS)
#define MPT_OS_WINDOWS_WINNT 0
#define MPT_OS_WINDOWS_WIN9X 1
#define MPT_OS_WINDOWS_WIN32 0
#define MPT_WIN_VERSION      (_WIN32_WINDOWS << 16)
#elif defined(WINVER)
#define MPT_OS_WINDOWS_WINNT 0
#define MPT_OS_WINDOWS_WIN9X 0
#define MPT_OS_WINDOWS_WIN32 1
#define MPT_WIN_VERSION      (WINVER << 16)
#else
// assume modern
#define MPT_OS_WINDOWS_WINNT 1
#define MPT_OS_WINDOWS_WIN9X 0
#define MPT_OS_WINDOWS_WIN32 0
#define MPT_WIN_VERSION      MPT_WIN_NT4
#endif
#define MPT_WINRT_AT_LEAST(v) (MPT_OS_WINDOWS_WINRT && MPT_OS_WINDOWS_WINNT && (MPT_WIN_VERSION >= (v)))
#define MPT_WINRT_BEFORE(v)   (MPT_OS_WINDOWS_WINRT && MPT_OS_WINDOWS_WINNT && (MPT_WIN_VERSION < (v)))
#define MPT_WINNT_AT_LEAST(v) (MPT_OS_WINDOWS_WINNT && (MPT_WIN_VERSION >= (v)))
#define MPT_WINNT_BEFORE(v)   (MPT_OS_WINDOWS_WINNT && (MPT_WIN_VERSION < (v)))
#define MPT_WIN9X_AT_LEAST(v) ((MPT_OS_WINDOWS_WINNT || MPT_OS_WINDOWS_WIN9X) && (MPT_WIN_VERSION >= (v)))
#define MPT_WIN9X_BEFORE(v)   ((MPT_OS_WINDOWS_WINNT || MPT_OS_WINDOWS_WIN9X) && (MPT_WIN_VERSION < (v)))
#define MPT_WIN32_AT_LEAST(v) ((MPT_OS_WINDOWS_WINNT || MPT_OS_WINDOWS_WIN9X || MPT_OS_WINDOWS_WIN32) && (MPT_WIN_VERSION >= (v)))
#define MPT_WIN32_BEFORE(v)   ((MPT_OS_WINDOWS_WINNT || MPT_OS_WINDOWS_WIN9X || MPT_OS_WINDOWS_WIN32) && (MPT_WIN_VERSION < (v)))
#if MPT_OS_WINDOWS_WINRT
#define MPT_WIN_AT_LEAST(v) MPT_WINRT_AT_LEAST(v)
#define MPT_WIN_BEFORE(v)   MPT_WINRT_BEFORE(v)
#elif MPT_OS_WINDOWS_WINNT
#define MPT_WIN_AT_LEAST(v) MPT_WINNT_AT_LEAST(v)
#define MPT_WIN_BEFORE(v)   MPT_WINNT_BEFORE(v)
#elif MPT_OS_WINDOWS_WIN9X
#define MPT_WIN_AT_LEAST(v) MPT_WIN9X_AT_LEAST(v)
#define MPT_WIN_BEFORE(v)   MPT_WIN9X_BEFORE(v)
#elif MPT_OS_WINDOWS_WIN32
#define MPT_WIN_AT_LEAST(v) MPT_WIN32_AT_LEAST(v)
#define MPT_WIN_BEFORE(v)   MPT_WIN32_BEFORE(v)
#else
#define MPT_WIN_AT_LEAST(v) 0
#define MPT_WIN_BEFORE(v)   1
#endif


#elif defined(__APPLE__)
#define MPT_OS_MACOSX_OR_IOS 1
#include <TargetConditionals.h>
#if defined(TARGET_OS_OSX)
#if (TARGET_OS_OSX != 0)
#include <AvailabilityMacros.h>
#endif
#endif
//#if TARGET_IPHONE_SIMULATOR
//#elif TARGET_OS_IPHONE
//#elif TARGET_OS_MAC
//#else
//#endif


#elif defined(__HAIKU__)
#define MPT_OS_HAIKU 1


#elif defined(__ANDROID__) || defined(ANDROID)
#define MPT_OS_ANDROID 1


#elif defined(__linux__)
#define MPT_OS_LINUX 1


#elif defined(__DragonFly__)
#define MPT_OS_DRAGONFLYBSD 1


#elif defined(__FreeBSD__)
#define MPT_OS_FREEBSD 1


#elif defined(__OpenBSD__)
#define MPT_OS_OPENBSD 1


#elif defined(__NetBSD__)
#define MPT_OS_NETBSD 1


#elif defined(__unix__)
#define MPT_OS_GENERIC_UNIX 1


#else
#define MPT_OS_UNKNOWN 1


#endif


#ifndef MPT_OS_DJGPP
#define MPT_OS_DJGPP 0
#endif
#ifndef MPT_OS_EMSCRIPTEN
#define MPT_OS_EMSCRIPTEN 0
#endif
#ifndef MPT_OS_WINDOWS
#define MPT_OS_WINDOWS 0
#endif
#ifndef MPT_OS_WINDOWS_WINRT
#define MPT_OS_WINDOWS_WINRT 0
#endif
#ifndef MPT_OS_WINDOWS_WINNT
#define MPT_OS_WINDOWS_WINNT 0
#endif
#ifndef MPT_OS_WINDOWS_WIN9X
#define MPT_OS_WINDOWS_WIN9X 0
#endif
#ifndef MPT_OS_WINDOWS_WIN32
#define MPT_OS_WINDOWS_WIN32 0
#endif
#ifndef MPT_WINRT_AT_LEAST
#define MPT_WINRT_AT_LEAST(v) 0
#endif
#ifndef MPT_WINRT_BEFORE
#define MPT_WINRT_BEFORE(v) 0
#endif
#ifndef MPT_WINNT_AT_LEAST
#define MPT_WINNT_AT_LEAST(v) 0
#endif
#ifndef MPT_WINNT_BEFORE
#define MPT_WINNT_BEFORE(v) 0
#endif
#ifndef MPT_WIN9X_AT_LEAST
#define MPT_WIN9X_AT_LEAST(v) 0
#endif
#ifndef MPT_WIN9X_BEFORE
#define MPT_WIN9X_BEFORE(v) 0
#endif
#ifndef MPT_WIN32_AT_LEAST
#define MPT_WIN32_AT_LEAST(v) 0
#endif
#ifndef MPT_WIN32_BEFORE
#define MPT_WIN32_BEFORE(v) 0
#endif
#ifndef MPT_WIN_AT_LEAST
#define MPT_WIN_AT_LEAST(v) 0
#endif
#ifndef MPT_WIN_BEFORE
#define MPT_WIN_BEFORE(v) 0
#endif
#ifndef MPT_WIN_API_DESKTOP
#define MPT_WIN_API_DESKTOP 0
#endif
#ifndef MPT_WIN_API_UNIVERSAL
#define MPT_WIN_API_UNIVERSAL 0
#endif
#ifndef MPT_WIN_API_STORE_PC
#define MPT_WIN_API_STORE_PC 0
#endif
#ifndef MPT_WIN_API_STORE_PHONE
#define MPT_WIN_API_STORE_PHONE 0
#endif
#ifndef MPT_OS_MACOSX_OR_IOS
#define MPT_OS_MACOSX_OR_IOS 0
#endif
#ifndef MPT_OS_HAIKU
#define MPT_OS_HAIKU 0
#endif
#ifndef MPT_OS_ANDROID
#define MPT_OS_ANDROID 0
#endif
#ifndef MPT_OS_LINUX
#define MPT_OS_LINUX 0
#endif
#ifndef MPT_OS_DRAGONFLYBSD
#define MPT_OS_DRAGONFLYBSD 0
#endif
#ifndef MPT_OS_FREEBSD
#define MPT_OS_FREEBSD 0
#endif
#ifndef MPT_OS_OPENBSD
#define MPT_OS_OPENBSD 0
#endif
#ifndef MPT_OS_NETBSD
#define MPT_OS_NETBSD 0
#endif
#ifndef MPT_OS_GENERIC_UNIX
#define MPT_OS_GENERIC_UNIX 0
#endif
#ifndef MPT_OS_UNKNOWN
#define MPT_OS_UNKNOWN 0
#endif



#define MPT_MODE_KERNEL 0



#endif // MPT_BASE_DETECT_OS_HPP
