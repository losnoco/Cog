/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_STRING_TRANSCODE_TRANSCODE_HPP
#define MPT_STRING_TRANSCODE_TRANSCODE_HPP



#include "mpt/base/detect.hpp"
#include "mpt/base/integer.hpp"
#include "mpt/base/macros.hpp"
#include "mpt/base/namespace.hpp"
#include "mpt/base/saturate_cast.hpp"
#include "mpt/detect/mfc.hpp"
#include "mpt/string/types.hpp"

#include <array>
#if !defined(MPT_COMPILER_QUIRK_NO_WCHAR)
#include <locale>
#endif // !MPT_COMPILER_QUIRK_NO_WCHAR
#include <stdexcept>
#include <string>
#if !defined(MPT_COMPILER_QUIRK_NO_WCHAR)
#include <type_traits>
#endif // !MPT_COMPILER_QUIRK_NO_WCHAR
#include <vector>

#if MPT_OS_DJGPP
#include <cstring>
#endif // MPT_OS_DJGPP

#if MPT_OS_WINDOWS
#include <windows.h>
#endif // MPT_OS_WINDOWS

#if MPT_OS_DJGPP
#include <dpmi.h>
#endif // MPT_OS_DJGPP



namespace mpt {
inline namespace MPT_INLINE_NS {




// default 1:1 mapping
inline constexpr char32_t CharsetTableISO8859_1[256] = {
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008, 0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f,
	0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 0x0018, 0x0019, 0x001a, 0x001b, 0x001c, 0x001d, 0x001e, 0x001f,
	0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
	0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
	0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,
	0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f,
	0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,
	0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x007f,
	0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087, 0x0088, 0x0089, 0x008a, 0x008b, 0x008c, 0x008d, 0x008e, 0x008f,
	0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097, 0x0098, 0x0099, 0x009a, 0x009b, 0x009c, 0x009d, 0x009e, 0x009f,
	0x00a0, 0x00a1, 0x00a2, 0x00a3, 0x00a4, 0x00a5, 0x00a6, 0x00a7, 0x00a8, 0x00a9, 0x00aa, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x00af,
	0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x00b4, 0x00b5, 0x00b6, 0x00b7, 0x00b8, 0x00b9, 0x00ba, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x00bf,
	0x00c0, 0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5, 0x00c6, 0x00c7, 0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd, 0x00ce, 0x00cf,
	0x00d0, 0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x00d7, 0x00d8, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x00dd, 0x00de, 0x00df,
	0x00e0, 0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x00e7, 0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef,
	0x00f0, 0x00f1, 0x00f2, 0x00f3, 0x00f4, 0x00f5, 0x00f6, 0x00f7, 0x00f8, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x00fd, 0x00fe, 0x00ff};

inline constexpr char32_t CharsetTableISO8859_15[256] = {
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008, 0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f,
	0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 0x0018, 0x0019, 0x001a, 0x001b, 0x001c, 0x001d, 0x001e, 0x001f,
	0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
	0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
	0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,
	0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f,
	0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,
	0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x007f,
	0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087, 0x0088, 0x0089, 0x008a, 0x008b, 0x008c, 0x008d, 0x008e, 0x008f,
	0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097, 0x0098, 0x0099, 0x009a, 0x009b, 0x009c, 0x009d, 0x009e, 0x009f,
	0x00a0, 0x00a1, 0x00a2, 0x00a3, 0x20ac, 0x00a5, 0x0160, 0x00a7, 0x0161, 0x00a9, 0x00aa, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x00af,
	0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x017d, 0x00b5, 0x00b6, 0x00b7, 0x017e, 0x00b9, 0x00ba, 0x00bb, 0x0152, 0x0153, 0x0178, 0x00bf,
	0x00c0, 0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5, 0x00c6, 0x00c7, 0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd, 0x00ce, 0x00cf,
	0x00d0, 0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x00d7, 0x00d8, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x00dd, 0x00de, 0x00df,
	0x00e0, 0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x00e7, 0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef,
	0x00f0, 0x00f1, 0x00f2, 0x00f3, 0x00f4, 0x00f5, 0x00f6, 0x00f7, 0x00f8, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x00fd, 0x00fe, 0x00ff};

inline constexpr char32_t CharsetTableWindows1252[256] = {
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008, 0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f,
	0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 0x0018, 0x0019, 0x001a, 0x001b, 0x001c, 0x001d, 0x001e, 0x001f,
	0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
	0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
	0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,
	0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f,
	0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,
	0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x007f,
	0x20ac, 0x0081, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021, 0x02c6, 0x2030, 0x0160, 0x2039, 0x0152, 0x008d, 0x017d, 0x008f,
	0x0090, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014, 0x02dc, 0x2122, 0x0161, 0x203a, 0x0153, 0x009d, 0x017e, 0x0178,
	0x00a0, 0x00a1, 0x00a2, 0x00a3, 0x00a4, 0x00a5, 0x00a6, 0x00a7, 0x00a8, 0x00a9, 0x00aa, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x00af,
	0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x00b4, 0x00b5, 0x00b6, 0x00b7, 0x00b8, 0x00b9, 0x00ba, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x00bf,
	0x00c0, 0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5, 0x00c6, 0x00c7, 0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd, 0x00ce, 0x00cf,
	0x00d0, 0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x00d7, 0x00d8, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x00dd, 0x00de, 0x00df,
	0x00e0, 0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x00e7, 0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef,
	0x00f0, 0x00f1, 0x00f2, 0x00f3, 0x00f4, 0x00f5, 0x00f6, 0x00f7, 0x00f8, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x00fd, 0x00fe, 0x00ff};

inline constexpr char32_t CharsetTableCP850[256] = {
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008, 0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f,
	0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 0x0018, 0x0019, 0x001a, 0x001b, 0x001c, 0x001d, 0x001e, 0x001f,
	0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
	0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
	0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,
	0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f,
	0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,
	0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x2302,
	0x00c7, 0x00fc, 0x00e9, 0x00e2, 0x00e4, 0x00e0, 0x00e5, 0x00e7, 0x00ea, 0x00eb, 0x00e8, 0x00ef, 0x00ee, 0x00ec, 0x00c4, 0x00c5,
	0x00c9, 0x00e6, 0x00c6, 0x00f4, 0x00f6, 0x00f2, 0x00fb, 0x00f9, 0x00ff, 0x00d6, 0x00dc, 0x00F8, 0x00a3, 0x00D8, 0x00D7, 0x0192,
	0x00e1, 0x00ed, 0x00f3, 0x00fa, 0x00f1, 0x00d1, 0x00aa, 0x00ba, 0x00bf, 0x00AE, 0x00ac, 0x00bd, 0x00bc, 0x00a1, 0x00ab, 0x00bb,
	0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x00C1, 0x00C2, 0x00C0, 0x00A9, 0x2563, 0x2551, 0x2557, 0x255d, 0x00A2, 0x00A5, 0x2510,
	0x2514, 0x2534, 0x252c, 0x251c, 0x2500, 0x253c, 0x00E3, 0x00C3, 0x255a, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256c, 0x00A4,
	0x00F0, 0x00D0, 0x00CA, 0x00CB, 0x00C8, 0x0131, 0x00CD, 0x00CE, 0x00CF, 0x2518, 0x250c, 0x2588, 0x2584, 0x00A6, 0x00CC, 0x2580,
	0x00D3, 0x00df, 0x00D4, 0x00D2, 0x00F5, 0x00D5, 0x00b5, 0x00FE, 0x00DE, 0x00DA, 0x00DB, 0x00D9, 0x00FD, 0x00DD, 0x00AF, 0x00B4,
	0x00AD, 0x00b1, 0x2017, 0x00BE, 0x00B6, 0x00A7, 0x00f7, 0x00B8, 0x00b0, 0x00A8, 0x00b7, 0x00B9, 0x00B3, 0x00b2, 0x25a0, 0x00a0};

inline constexpr char32_t CharsetTableCP437[256] = {
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008, 0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f,
	0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 0x0018, 0x0019, 0x001a, 0x001b, 0x001c, 0x001d, 0x001e, 0x001f,
	0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
	0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
	0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,
	0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f,
	0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,
	0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x2302,
	0x00c7, 0x00fc, 0x00e9, 0x00e2, 0x00e4, 0x00e0, 0x00e5, 0x00e7, 0x00ea, 0x00eb, 0x00e8, 0x00ef, 0x00ee, 0x00ec, 0x00c4, 0x00c5,
	0x00c9, 0x00e6, 0x00c6, 0x00f4, 0x00f6, 0x00f2, 0x00fb, 0x00f9, 0x00ff, 0x00d6, 0x00dc, 0x00a2, 0x00a3, 0x00a5, 0x20a7, 0x0192,
	0x00e1, 0x00ed, 0x00f3, 0x00fa, 0x00f1, 0x00d1, 0x00aa, 0x00ba, 0x00bf, 0x2310, 0x00ac, 0x00bd, 0x00bc, 0x00a1, 0x00ab, 0x00bb,
	0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556, 0x2555, 0x2563, 0x2551, 0x2557, 0x255d, 0x255c, 0x255b, 0x2510,
	0x2514, 0x2534, 0x252c, 0x251c, 0x2500, 0x253c, 0x255e, 0x255f, 0x255a, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256c, 0x2567,
	0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256b, 0x256a, 0x2518, 0x250c, 0x2588, 0x2584, 0x258c, 0x2590, 0x2580,
	0x03b1, 0x00df, 0x0393, 0x03c0, 0x03a3, 0x03c3, 0x00b5, 0x03c4, 0x03a6, 0x0398, 0x03a9, 0x03b4, 0x221e, 0x03c6, 0x03b5, 0x2229,
	0x2261, 0x00b1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00f7, 0x2248, 0x00b0, 0x2219, 0x00b7, 0x221a, 0x207f, 0x00b2, 0x25a0, 0x00a0};

// <https://de.wikipedia.org/wiki/Commodore_Amiga_(Zeichensatz)>
inline constexpr char32_t CharsetTableAmiga[256] = {
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008, 0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f,
	0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 0x0018, 0x0019, 0x001a, 0x001b, 0x001c, 0x001d, 0x001e, 0x001f,
	0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
	0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
	0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,
	0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f,
	0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,
	0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x2592,
	0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087, 0x0088, 0x0089, 0x008a, 0x008b, 0x008c, 0x008d, 0x008e, 0x008f,
	0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097, 0x0098, 0x0099, 0x009a, 0x009b, 0x009c, 0x009d, 0x009e, 0x009f,
	0x00a0, 0x00a1, 0x00a2, 0x00a3, 0x00a4, 0x00a5, 0x00a6, 0x00a7, 0x00a8, 0x00a9, 0x00aa, 0x00ab, 0x00ac, 0x2013, 0x00ae, 0x00af,
	0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x00b4, 0x00b5, 0x00b6, 0x00b7, 0x00b8, 0x00b9, 0x00ba, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x00bf,
	0x00c0, 0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5, 0x00c6, 0x00c7, 0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd, 0x00ce, 0x00cf,
	0x00d0, 0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x00d7, 0x00d8, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x00dd, 0x00de, 0x00df,
	0x00e0, 0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x00e7, 0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef,
	0x00f0, 0x00f1, 0x00f2, 0x00f3, 0x00f4, 0x00f5, 0x00f6, 0x00f7, 0x00f8, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x00fd, 0x00fe, 0x00ff};

// Based on RISCOSI.TXT from <https://www.unicode.org/L2/L2019/19025-terminals-prop.pdf>,
// with gaps filled in from standard set at <https://en.wikipedia.org/wiki/RISC_OS_character_set>.
inline constexpr char32_t CharsetTableRISC_OS[256] = {
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008, 0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f,
	0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 0x0018, 0x0019, 0x001a, 0x001b, 0x001c, 0x001d, 0x001e, 0x001f,
	0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
	0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
	0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,
	0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f,
	0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,
	0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x007f,
	0x20AC, 0x0174, 0x0175, 0x25f0, 0x1fbc0, 0x0176, 0x0177, 0xfffd, 0x21e6, 0x21e8, 0x21e9, 0x21e7, 0x2026, 0x2122, 0x2030, 0x2022,
	0x2018, 0x2019, 0x2039, 0x203A, 0x201C, 0x201D, 0x201E, 0x2013, 0x2014, 0x2212, 0x0152, 0x0153, 0x2020, 0x2021, 0xFB01, 0xFB02,
	0x00a0, 0x00a1, 0x00a2, 0x00a3, 0x00a4, 0x00a5, 0x00a6, 0x00a7, 0x00a8, 0x00a9, 0x00aa, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x00af,
	0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x00b4, 0x00b5, 0x00b6, 0x00b7, 0x00b8, 0x00b9, 0x00ba, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x00bf,
	0x00c0, 0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5, 0x00c6, 0x00c7, 0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd, 0x00ce, 0x00cf,
	0x00d0, 0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x00d7, 0x00d8, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x00dd, 0x00de, 0x00df,
	0x00e0, 0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x00e7, 0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef,
	0x00f0, 0x00f1, 0x00f2, 0x00f3, 0x00f4, 0x00f5, 0x00f6, 0x00f7, 0x00f8, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x00fd, 0x00fe, 0x00ff};

template <typename Tsrcstring>
inline mpt::widestring decode_8bit(const Tsrcstring & str, const char32_t (&table)[256], mpt::widechar replacement = MPT_WIDECHAR('\uFFFD')) {
	mpt::widestring res;
	res.reserve(str.length());
	for (std::size_t i = 0; i < str.length(); ++i) {
		std::size_t c = static_cast<std::size_t>(mpt::char_value(str[i]));
		if (c < std::size(table)) {
			res.push_back(static_cast<mpt::widechar>(table[c]));
		} else {
			res.push_back(replacement);
		}
	}
	return res;
}

template <typename Tdststring>
inline Tdststring encode_8bit(const mpt::widestring & str, const char32_t (&table)[256], char replacement = '?') {
	Tdststring res;
	res.reserve(str.length());
	for (std::size_t i = 0; i < str.length(); ++i) {
		char32_t c = static_cast<char32_t>(str[i]);
		bool found = false;
		// Try non-control characters first.
		// In cases where there are actual characters mirrored in this range (like in AMS/AMS2 character sets),
		// characters in the common range are preferred this way.
		for (std::size_t x = 0x20; x < std::size(table); ++x) {
			if (c == table[x]) {
				res.push_back(static_cast<typename Tdststring::value_type>(static_cast<uint8>(x)));
				found = true;
				break;
			}
		}
		if (!found) {
			// try control characters
			for (std::size_t x = 0x00; x < std::size(table) && x < 0x20; ++x) {
				if (c == table[x]) {
					res.push_back(static_cast<typename Tdststring::value_type>(static_cast<uint8>(x)));
					found = true;
					break;
				}
			}
		}
		if (!found) {
			res.push_back(replacement);
		}
	}
	return res;
}

template <typename Tsrcstring>
inline mpt::widestring decode_8bit_no_c1(const Tsrcstring & str, const char32_t (&table)[256], mpt::widechar replacement = MPT_WIDECHAR('\uFFFD')) {
	mpt::widestring res;
	res.reserve(str.length());
	for (std::size_t i = 0; i < str.length(); ++i) {
		std::size_t c = static_cast<std::size_t>(mpt::char_value(str[i]));
		if ((0x80 <= c) && (c <= 0x9f)) {
			res.push_back(replacement);
		} else if (c < std::size(table)) {
			res.push_back(static_cast<mpt::widechar>(table[c]));
		} else {
			res.push_back(replacement);
		}
	}
	return res;
}

template <typename Tdststring>
inline Tdststring encode_8bit_no_c1(const mpt::widestring & str, const char32_t (&table)[256], char replacement = '?') {
	Tdststring res;
	res.reserve(str.length());
	for (std::size_t i = 0; i < str.length(); ++i) {
		char32_t c = static_cast<char32_t>(str[i]);
		bool found = false;
		// Try non-control characters first.
		// In cases where there are actual characters mirrored in this range (like in AMS/AMS2 character sets),
		// characters in the common range are preferred this way.
		for (std::size_t x = 0x20; x < std::size(table); ++x) {
			if ((0x80 <= c) && (c <= 0x9f)) {
				continue;
			}
			if (c == table[x]) {
				res.push_back(static_cast<typename Tdststring::value_type>(static_cast<uint8>(x)));
				found = true;
				break;
			}
		}
		if (!found) {
			// try control characters
			for (std::size_t x = 0x00; x < std::size(table) && x < 0x20; ++x) {
				if (c == table[x]) {
					res.push_back(static_cast<typename Tdststring::value_type>(static_cast<uint8>(x)));
					found = true;
					break;
				}
			}
		}
		if (!found) {
			res.push_back(replacement);
		}
	}
	return res;
}

template <typename Tsrcstring>
inline mpt::widestring decode_ascii(const Tsrcstring & str, mpt::widechar replacement = MPT_WIDECHAR('\uFFFD')) {
	mpt::widestring res;
	res.reserve(str.length());
	for (std::size_t i = 0; i < str.length(); ++i) {
		uint8 c = mpt::char_value(str[i]);
		if (c <= 0x7f) {
			res.push_back(static_cast<mpt::widechar>(static_cast<uint32>(c)));
		} else {
			res.push_back(replacement);
		}
	}
	return res;
}

template <typename Tdststring>
inline Tdststring encode_ascii(const mpt::widestring & str, char replacement = '?') {
	Tdststring res;
	res.reserve(str.length());
	for (std::size_t i = 0; i < str.length(); ++i) {
		char32_t c = static_cast<char32_t>(str[i]);
		if (c <= 0x7f) {
			res.push_back(static_cast<typename Tdststring::value_type>(static_cast<uint8>(c)));
		} else {
			res.push_back(replacement);
		}
	}
	return res;
}

template <typename Tsrcstring>
inline mpt::widestring decode_iso8859_1(const Tsrcstring & str, mpt::widechar replacement = MPT_WIDECHAR('\uFFFD')) {
	MPT_UNUSED(replacement);
	mpt::widestring res;
	res.reserve(str.length());
	for (std::size_t i = 0; i < str.length(); ++i) {
		uint8 c = mpt::char_value(str[i]);
		res.push_back(static_cast<mpt::widechar>(static_cast<uint32>(c)));
	}
	return res;
}

template <typename Tdststring>
inline Tdststring encode_iso8859_1(const mpt::widestring & str, char replacement = '?') {
	Tdststring res;
	res.reserve(str.length());
	for (std::size_t i = 0; i < str.length(); ++i) {
		char32_t c = static_cast<char32_t>(str[i]);
		if (c <= 0xff) {
			res.push_back(static_cast<typename Tdststring::value_type>(static_cast<uint8>(c)));
		} else {
			res.push_back(replacement);
		}
	}
	return res;
}



template <typename Tsrcstring>
inline mpt::widestring decode_utf8(const Tsrcstring & str, mpt::widechar replacement = MPT_WIDECHAR('\uFFFD')) {
	const Tsrcstring & in = str;
	mpt::widestring out;
	// state:
	std::size_t charsleft = 0;
	char32_t ucs4 = 0;
	for (uint8 c : in) {
		if (charsleft == 0) {
			if ((c & 0x80) == 0x00) {
				out.push_back(static_cast<mpt::widechar>(c));
			} else if ((c & 0xE0) == 0xC0) {
				ucs4 = c & 0x1F;
				charsleft = 1;
			} else if ((c & 0xF0) == 0xE0) {
				ucs4 = c & 0x0F;
				charsleft = 2;
			} else if ((c & 0xF8) == 0xF0) {
				ucs4 = c & 0x07;
				charsleft = 3;
			} else {
				out.push_back(replacement);
				ucs4 = 0;
				charsleft = 0;
			}
		} else {
			if ((c & 0xC0) != 0x80) {
				out.push_back(replacement);
				ucs4 = 0;
				charsleft = 0;
			}
			ucs4 <<= 6;
			ucs4 |= c & 0x3F;
			charsleft--;
			if (charsleft == 0) {
				if constexpr (sizeof(mpt::widechar) == 2) {
					if (ucs4 > 0x1fffff) {
						out.push_back(replacement);
						ucs4 = 0;
						charsleft = 0;
					}
					if (ucs4 <= 0xffff) {
						out.push_back(static_cast<mpt::widechar>(ucs4));
					} else {
						uint32 surrogate = static_cast<uint32>(ucs4) - 0x10000;
						uint16 hi_sur = static_cast<uint16>((0x36 << 10) | ((surrogate >> 10) & ((1 << 10) - 1)));
						uint16 lo_sur = static_cast<uint16>((0x37 << 10) | ((surrogate >> 0) & ((1 << 10) - 1)));
						out.push_back(hi_sur);
						out.push_back(lo_sur);
					}
				} else {
					out.push_back(static_cast<mpt::widechar>(ucs4));
				}
				ucs4 = 0;
			}
		}
	}
	if (charsleft != 0) {
		out.push_back(replacement);
		ucs4 = 0;
		charsleft = 0;
	}
	return out;
}

template <typename Tdststring>
inline Tdststring encode_utf8(const mpt::widestring & str, typename Tdststring::value_type replacement = static_cast<typename Tdststring::value_type>(mpt::char_value('?'))) {
	const mpt::widestring & in = str;
	Tdststring out;
	for (std::size_t i = 0; i < in.length(); i++) {
		mpt::widechar wc = in[i];
		char32_t ucs4 = 0;
		if constexpr (sizeof(mpt::widechar) == 2) {
			uint16 c = static_cast<uint16>(wc);
			if (i + 1 < in.length()) {
				// check for surrogate pair
				uint16 hi_sur = in[i + 0];
				uint16 lo_sur = in[i + 1];
				if (hi_sur >> 10 == 0x36 && lo_sur >> 10 == 0x37) {
					// surrogate pair
					++i;
					hi_sur &= (1 << 10) - 1;
					lo_sur &= (1 << 10) - 1;
					ucs4 = (static_cast<uint32>(hi_sur) << 10) | (static_cast<uint32>(lo_sur) << 0);
				} else {
					// no surrogate pair
					ucs4 = static_cast<char32_t>(c);
				}
			} else {
				// no surrogate possible
				ucs4 = static_cast<char32_t>(c);
			}
		} else {
			ucs4 = static_cast<char32_t>(wc);
		}
		if (ucs4 > 0x1fffff) {
			out.push_back(replacement);
			continue;
		}
		uint8 utf8[6];
		std::size_t numchars = 0;
		for (numchars = 0; numchars < 6; numchars++) {
			utf8[numchars] = ucs4 & 0x3F;
			ucs4 >>= 6;
			if (ucs4 == 0) {
				break;
			}
		}
		numchars++;
		if (numchars == 1) {
			out.push_back(utf8[0]);
			continue;
		}
		if (numchars == 2 && utf8[numchars - 1] == 0x01) {
			// generate shortest form
			out.push_back(utf8[0] | 0x40);
			continue;
		}
		std::size_t charsleft = numchars;
		while (charsleft > 0) {
			if (charsleft == numchars) {
				out.push_back(utf8[charsleft - 1] | (((1 << numchars) - 1) << (8 - numchars)));
			} else {
				// cppcheck false-positive
				// cppcheck-suppress arrayIndexOutOfBounds
				out.push_back(utf8[charsleft - 1] | 0x80);
			}
			charsleft--;
		}
	}
	return out;
}



template <typename Tdststring, typename Tsrcstring>
inline Tdststring utf32_from_utf16(const Tsrcstring & in, mpt::widechar replacement = MPT_WIDECHAR('\uFFFD')) {
	static_assert(sizeof(typename Tsrcstring::value_type) == 2);
	static_assert(sizeof(typename Tdststring::value_type) == 4);
	MPT_UNUSED(replacement);
	Tdststring out;
	out.reserve(in.length());
	for (std::size_t i = 0; i < in.length(); i++) {
		char16_t wc = static_cast<char16_t>(static_cast<uint16>(in[i]));
		char32_t ucs4 = 0;
		uint16 c = static_cast<uint16>(wc);
		if (i + 1 < in.length()) {
			// check for surrogate pair
			uint16 hi_sur = in[i + 0];
			uint16 lo_sur = in[i + 1];
			if (hi_sur >> 10 == 0x36 && lo_sur >> 10 == 0x37) {
				// surrogate pair
				++i;
				hi_sur &= (1 << 10) - 1;
				lo_sur &= (1 << 10) - 1;
				ucs4 = (static_cast<uint32>(hi_sur) << 10) | (static_cast<uint32>(lo_sur) << 0);
			} else {
				// no surrogate pair
				ucs4 = static_cast<char32_t>(c);
			}
		} else {
			// no surrogate possible
			ucs4 = static_cast<char32_t>(c);
		}
		out.push_back(static_cast<typename Tdststring::value_type>(static_cast<uint32>(ucs4)));
	}
	return out;
}

template <typename Tdststring, typename Tsrcstring>
inline Tdststring utf16_from_utf32(const Tsrcstring & in, mpt::widechar replacement = MPT_WIDECHAR('\uFFFD')) {
	static_assert(sizeof(typename Tsrcstring::value_type) == 4);
	static_assert(sizeof(typename Tdststring::value_type) == 2);
	Tdststring out;
	out.reserve(in.length());
	for (std::size_t i = 0; i < in.length(); i++) {
		char32_t ucs4 = static_cast<char32_t>(static_cast<uint32>(in[i]));
		if (ucs4 > 0x1fffff) {
			out.push_back(static_cast<typename Tdststring::value_type>(static_cast<uint16>(replacement)));
			ucs4 = 0;
		}
		if (ucs4 <= 0xffff) {
			out.push_back(static_cast<typename Tdststring::value_type>(static_cast<uint16>(ucs4)));
		} else {
			uint32 surrogate = static_cast<uint32>(ucs4) - 0x10000;
			uint16 hi_sur = static_cast<uint16>((0x36 << 10) | ((surrogate >> 10) & ((1 << 10) - 1)));
			uint16 lo_sur = static_cast<uint16>((0x37 << 10) | ((surrogate >> 0) & ((1 << 10) - 1)));
			out.push_back(static_cast<typename Tdststring::value_type>(hi_sur));
			out.push_back(static_cast<typename Tdststring::value_type>(lo_sur));
		}
	}
	return out;
}



#if MPT_OS_WINDOWS

inline bool has_codepage(UINT cp) {
	return IsValidCodePage(cp) ? true : false;
}

inline bool windows_has_encoding(common_encoding encoding) {
	bool result = false;
	switch (encoding) {
		case common_encoding::utf8:
			result = has_codepage(CP_UTF8);
			break;
		case common_encoding::ascii:
			result = has_codepage(20127);
			break;
		case common_encoding::iso8859_1:
			result = has_codepage(28591);
			break;
		case common_encoding::iso8859_15:
			result = has_codepage(28605);
			break;
		case common_encoding::cp850:
			result = has_codepage(850);
			break;
		case common_encoding::cp437:
			result = has_codepage(437);
			break;
		case common_encoding::windows1252:
			result = has_codepage(1252);
			break;
		case common_encoding::amiga:
			result = false;
			break;
		case common_encoding::riscos:
			result = false;
			break;
		case common_encoding::iso8859_1_no_c1:
			result = false;
			break;
		case common_encoding::iso8859_15_no_c1:
			result = false;
			break;
		case common_encoding::amiga_no_c1:
			result = false;
			break;
	}
	return result;
}

inline bool windows_has_encoding(logical_encoding encoding) {
	bool result = false;
	switch (encoding) {
		case logical_encoding::locale:
			result = true;
			break;
		case logical_encoding::active_locale:
			result = false;
			break;
	}
	return result;
}

inline UINT codepage_from_encoding(logical_encoding encoding) {
	UINT result = 0;
	switch (encoding) {
		case logical_encoding::locale:
			result = CP_ACP;
			break;
		case logical_encoding::active_locale:
			throw std::domain_error("unsupported encoding");
			break;
	}
	return result;
}

inline UINT codepage_from_encoding(common_encoding encoding) {
	UINT result = 0;
	switch (encoding) {
		case common_encoding::utf8:
			result = CP_UTF8;
			break;
		case common_encoding::ascii:
			result = 20127;
			break;
		case common_encoding::iso8859_1:
			result = 28591;
			break;
		case common_encoding::iso8859_15:
			result = 28605;
			break;
		case common_encoding::cp850:
			result = 850;
			break;
		case common_encoding::cp437:
			result = 437;
			break;
		case common_encoding::windows1252:
			result = 1252;
			break;
		case common_encoding::amiga:
			throw std::domain_error("unsupported encoding");
			break;
		case common_encoding::riscos:
			throw std::domain_error("unsupported encoding");
			break;
		case common_encoding::iso8859_1_no_c1:
			throw std::domain_error("unsupported encoding");
			break;
		case common_encoding::iso8859_15_no_c1:
			throw std::domain_error("unsupported encoding");
			break;
		case common_encoding::amiga_no_c1:
			throw std::domain_error("unsupported encoding");
			break;
	}
	return result;
}

template <typename Tdststring>
inline Tdststring encode_codepage(UINT codepage, const mpt::widestring & src) {
	static_assert(sizeof(typename Tdststring::value_type) == sizeof(char));
	static_assert(mpt::is_character<typename Tdststring::value_type>::value);
	Tdststring encoded_string;
	int required_size = WideCharToMultiByte(codepage, 0, src.data(), mpt::saturate_cast<int>(src.size()), nullptr, 0, nullptr, nullptr);
	if (required_size > 0) {
		encoded_string.resize(required_size);
		WideCharToMultiByte(codepage, 0, src.data(), mpt::saturate_cast<int>(src.size()), reinterpret_cast<CHAR *>(encoded_string.data()), required_size, nullptr, nullptr);
	}
	return encoded_string;
}

template <typename Tsrcstring>
inline mpt::widestring decode_codepage(UINT codepage, const Tsrcstring & src) {
	static_assert(sizeof(typename Tsrcstring::value_type) == sizeof(char));
	static_assert(mpt::is_character<typename Tsrcstring::value_type>::value);
	mpt::widestring decoded_string;
	int required_size = MultiByteToWideChar(codepage, 0, reinterpret_cast<const CHAR *>(src.data()), mpt::saturate_cast<int>(src.size()), nullptr, 0);
	if (required_size > 0) {
		decoded_string.resize(required_size);
		MultiByteToWideChar(codepage, 0, reinterpret_cast<const CHAR *>(src.data()), mpt::saturate_cast<int>(src.size()), decoded_string.data(), required_size);
	}
	return decoded_string;
}

#endif // MPT_OS_WINDOWS



#if MPT_OS_DJGPP

inline common_encoding djgpp_get_locale_encoding() {
	uint16 active_codepage = 437;
	uint16 system_codepage = 437;
	__dpmi_regs regs;
	std::memset(&regs, 0, sizeof(__dpmi_regs));
	regs.x.ax = 0x6601;
	if (__dpmi_int(0x21, &regs) == 0) {
		int cf = (regs.x.flags >> 0) & 1;
		if (cf == 0) {
			active_codepage = regs.x.bx;
			system_codepage = regs.x.dx;
		}
	}
	common_encoding result = common_encoding::cp437;
	if (active_codepage == 0) {
		result = common_encoding::cp437;
	} else if (active_codepage == 437) {
		result = common_encoding::cp437;
	} else if (active_codepage == 850) {
		result = common_encoding::cp850;
	} else if (system_codepage == 437) {
		result = common_encoding::cp437;
	} else if (system_codepage == 850) {
		result = common_encoding::cp850;
	} else {
		result = common_encoding::cp437;
	}
	return result;
}

#endif // MPT_OS_DJGPP



#if !defined(MPT_COMPILER_QUIRK_NO_WCHAR)

// Note:
//
//  std::codecvt::out in LLVM libc++ does not advance in and out pointers when
// running into a non-convertible character. This can happen when no locale is
// set on FreeBSD or MacOSX. This behaviour violates the C++ standard.
//
//  We apply the following (albeit costly, even on other platforms) work-around:
//  If the conversion errors out and does not advance the pointers at all, we
// retry the conversion with a space character prepended to the string. If it
// still does error out, we retry the whole conversion character by character.
//  This is costly even on other platforms in one single case: The first
// character is an invalid Unicode code point or otherwise not convertible. Any
// following non-convertible characters are not a problem.

inline std::wstring decode_locale_impl(const std::string & str, const std::locale & locale, wchar_t replacement = L'\uFFFD', int retry = 0, bool * progress = nullptr) {
	if (str.empty()) {
		return std::wstring();
	}
	std::vector<wchar_t> out;
	using codecvt_type = std::codecvt<wchar_t, char, std::mbstate_t>;
	std::mbstate_t state = std::mbstate_t();
	const codecvt_type & facet = std::use_facet<codecvt_type>(locale);
	codecvt_type::result result = codecvt_type::partial;
	const char * in_begin = str.data();
	const char * in_end = in_begin + str.size();
	out.resize((in_end - in_begin) * (mpt::saturate_cast<std::size_t>(facet.max_length()) + 1));
	wchar_t * out_begin = out.data();
	wchar_t * out_end = out.data() + out.size();
	const char * in_next = nullptr;
	wchar_t * out_next = nullptr;
	do {
		if (retry == 2) {
			for (;;) {
				in_next = nullptr;
				out_next = nullptr;
				result = facet.in(state, in_begin, in_begin + 1, in_next, out_begin, out_end, out_next);
				if (result == codecvt_type::partial && in_next == in_begin + 1) {
					in_begin = in_next;
					out_begin = out_next;
					continue;
				} else {
					break;
				}
			}
		} else {
			in_next = nullptr;
			out_next = nullptr;
			result = facet.in(state, in_begin, in_end, in_next, out_begin, out_end, out_next);
		}
		if (result == codecvt_type::partial || (result == codecvt_type::error && out_next == out_end)) {
			out.resize(out.size() * 2);
			in_begin = in_next;
			out_begin = out.data() + (out_next - out_begin);
			out_end = out.data() + out.size();
			continue;
		}
		if (retry == 0) {
			if (result == codecvt_type::error && in_next == in_begin && out_next == out_begin) {
				bool made_progress = true;
				decode_locale_impl(std::string(" ") + str, locale, replacement, 1, &made_progress);
				if (!made_progress) {
					return decode_locale_impl(str, locale, replacement, 2);
				}
			}
		} else if (retry == 1) {
			if (result == codecvt_type::error && in_next == in_begin && out_next == out_begin) {
				*progress = false;
			} else {
				*progress = true;
			}
			return std::wstring();
		}
		if (result == codecvt_type::error) {
			++in_next;
			*out_next = replacement;
			++out_next;
		}
		in_begin = in_next;
		out_begin = out_next;
	} while ((result == codecvt_type::error && in_next < in_end && out_next < out_end) || (retry == 2 && in_next < in_end));
	return std::wstring(out.data(), out_next);
}

template <typename Tsrcstring>
inline mpt::widestring decode_locale(const std::locale & locale, const Tsrcstring & src) {
	if constexpr (std::is_same<Tsrcstring, std::string>::value) {
		return decode_locale_impl(src, locale);
	} else {
		return decode_locale_impl(std::string(src.begin(), src.end()), locale);
	}
}

inline std::string encode_locale_impl(const std::wstring & str, const std::locale & locale, char replacement = '?', int retry = 0, bool * progress = nullptr) {
	if (str.empty()) {
		return std::string();
	}
	std::vector<char> out;
	using codecvt_type = std::codecvt<wchar_t, char, std::mbstate_t>;
	std::mbstate_t state = std::mbstate_t();
	const codecvt_type & facet = std::use_facet<codecvt_type>(locale);
	codecvt_type::result result = codecvt_type::partial;
	const wchar_t * in_begin = str.data();
	const wchar_t * in_end = in_begin + str.size();
	out.resize((in_end - in_begin) * (mpt::saturate_cast<std::size_t>(facet.max_length()) + 1));
	char * out_begin = out.data();
	char * out_end = out.data() + out.size();
	const wchar_t * in_next = nullptr;
	char * out_next = nullptr;
	do {
		if (retry == 2) {
			for (;;) {
				in_next = nullptr;
				out_next = nullptr;
				result = facet.out(state, in_begin, in_begin + 1, in_next, out_begin, out_end, out_next);
				if (result == codecvt_type::partial && in_next == in_begin + 1) {
					in_begin = in_next;
					out_begin = out_next;
					continue;
				} else {
					break;
				}
			}
		} else {
			in_next = nullptr;
			out_next = nullptr;
			result = facet.out(state, in_begin, in_end, in_next, out_begin, out_end, out_next);
		}
		if (result == codecvt_type::partial || (result == codecvt_type::error && out_next == out_end)) {
			out.resize(out.size() * 2);
			in_begin = in_next;
			out_begin = out.data() + (out_next - out_begin);
			out_end = out.data() + out.size();
			continue;
		}
		if (retry == 0) {
			if (result == codecvt_type::error && in_next == in_begin && out_next == out_begin) {
				bool made_progress = true;
				encode_locale_impl(std::wstring(L" ") + str, locale, replacement, 1, &made_progress);
				if (!made_progress) {
					return encode_locale_impl(str, locale, replacement, 2);
				}
			}
		} else if (retry == 1) {
			if (result == codecvt_type::error && in_next == in_begin && out_next == out_begin) {
				*progress = false;
			} else {
				*progress = true;
			}
			return std::string();
		}
		if (result == codecvt_type::error) {
			++in_next;
			*out_next = replacement;
			++out_next;
		}
		in_begin = in_next;
		out_begin = out_next;
	} while ((result == codecvt_type::error && in_next < in_end && out_next < out_end) || (retry == 2 && in_next < in_end));
	return std::string(out.data(), out_next);
}

template <typename Tdststring>
inline Tdststring encode_locale(const std::locale & locale, const mpt::widestring & src) {
	if constexpr (std::is_same<Tdststring, std::string>::value) {
		return encode_locale_impl(src, locale);
	} else {
		const std::string tmp = encode_locale_impl(src, locale);
		return Tdststring(tmp.begin(), tmp.end());
	}
}

#endif // !MPT_COMPILER_QUIRK_NO_WCHAR



#if MPT_OS_WINDOWS
template <typename Tdststring>
inline Tdststring encode(UINT codepage, const mpt::widestring & src) {
	static_assert(sizeof(typename Tdststring::value_type) == sizeof(char));
	static_assert(mpt::is_character<typename Tdststring::value_type>::value);
	return encode_codepage<Tdststring>(codepage, src);
}
#endif // MPT_OS_WINDOWS

#if !defined(MPT_COMPILER_QUIRK_NO_WCHAR)
template <typename Tdststring>
inline Tdststring encode(const std::locale & locale, const mpt::widestring & src) {
	static_assert(sizeof(typename Tdststring::value_type) == sizeof(char));
	static_assert(mpt::is_character<typename Tdststring::value_type>::value);
	return encode_locale<Tdststring>(src, locale);
}
#endif // !MPT_COMPILER_QUIRK_NO_WCHAR

template <typename Tdststring>
inline Tdststring encode(const char32_t (&table)[256], const mpt::widestring & src) {
	static_assert(sizeof(typename Tdststring::value_type) == sizeof(char));
	static_assert(mpt::is_character<typename Tdststring::value_type>::value);
	return encode_8bit<Tdststring>(src, table);
}

template <typename Tdststring>
inline Tdststring encode(common_encoding encoding, const mpt::widestring & src) {
	static_assert(sizeof(typename Tdststring::value_type) == sizeof(char));
	static_assert(mpt::is_character<typename Tdststring::value_type>::value);
#if MPT_OS_WINDOWS
	if (windows_has_encoding(encoding)) {
		return encode_codepage<Tdststring>(codepage_from_encoding(encoding), src);
	}
#endif
	switch (encoding) {
		case common_encoding::utf8:
			return encode_utf8<Tdststring>(src);
			break;
		case common_encoding::ascii:
			return encode_ascii<Tdststring>(src);
			break;
		case common_encoding::iso8859_1:
			return encode_iso8859_1<Tdststring>(src);
			break;
		case common_encoding::iso8859_15:
			return encode_8bit<Tdststring>(src, CharsetTableISO8859_15);
			break;
		case common_encoding::cp437:
			return encode_8bit<Tdststring>(src, CharsetTableCP437);
			break;
		case common_encoding::cp850:
			return encode_8bit<Tdststring>(src, CharsetTableCP850);
			break;
		case common_encoding::windows1252:
			return encode_8bit<Tdststring>(src, CharsetTableWindows1252);
			break;
		case common_encoding::amiga:
			return encode_8bit<Tdststring>(src, CharsetTableAmiga);
			break;
		case common_encoding::riscos:
			return encode_8bit<Tdststring>(src, CharsetTableRISC_OS);
			break;
		case common_encoding::iso8859_1_no_c1:
			return encode_8bit_no_c1<Tdststring>(src, CharsetTableISO8859_1);
			break;
		case common_encoding::iso8859_15_no_c1:
			return encode_8bit_no_c1<Tdststring>(src, CharsetTableISO8859_15);
			break;
		case common_encoding::amiga_no_c1:
			return encode_8bit_no_c1<Tdststring>(src, CharsetTableAmiga);
			break;
	}
	throw std::domain_error("unsupported encoding");
}

template <typename Tdststring>
inline Tdststring encode(logical_encoding encoding, const mpt::widestring & src) {
	static_assert(sizeof(typename Tdststring::value_type) == sizeof(char));
	static_assert(mpt::is_character<typename Tdststring::value_type>::value);
#if MPT_OS_WINDOWS
	if (windows_has_encoding(encoding)) {
		return encode_codepage<Tdststring>(codepage_from_encoding(encoding), src);
	}
#endif
#if MPT_OS_DJGPP
	switch (encoding) {
		case logical_encoding::locale:
			return encode<Tdststring>(djgpp_get_locale_encoding(), src);
			break;
		case logical_encoding::active_locale:
			return encode<Tdststring>(djgpp_get_locale_encoding(), src);
			break;
	}
	throw std::domain_error("unsupported encoding");
#elif !defined(MPT_COMPILER_QUIRK_NO_WCHAR)
	switch (encoding) {
		case logical_encoding::locale:
			return encode_locale<Tdststring>(std::locale(""), src);
			break;
		case logical_encoding::active_locale:
			return encode_locale<Tdststring>(std::locale(), src);
			break;
	}
	throw std::domain_error("unsupported encoding");
#else
	throw std::domain_error("unsupported encoding");
#endif
}

#if MPT_OS_WINDOWS
template <typename Tsrcstring>
inline mpt::widestring decode(UINT codepage, const Tsrcstring & src) {
	static_assert(sizeof(typename Tsrcstring::value_type) == sizeof(char));
	static_assert(mpt::is_character<typename Tsrcstring::value_type>::value);
	return decode_codepage(codepage, src);
}
#endif // MPT_OS_WINDOWS

#if !defined(MPT_COMPILER_QUIRK_NO_WCHAR)
template <typename Tsrcstring>
inline mpt::widestring decode(const std::locale & locale, const Tsrcstring & src) {
	static_assert(sizeof(typename Tsrcstring::value_type) == sizeof(char));
	static_assert(mpt::is_character<typename Tsrcstring::value_type>::value);
	return decode_locale(src, locale);
}
#endif // !MPT_COMPILER_QUIRK_NO_WCHAR

template <typename Tsrcstring>
inline mpt::widestring decode(const char32_t (&table)[256], const Tsrcstring & src) {
	static_assert(sizeof(typename Tsrcstring::value_type) == sizeof(char));
	static_assert(mpt::is_character<typename Tsrcstring::value_type>::value);
	return decode_8bit(src, table);
}

template <typename Tsrcstring>
inline mpt::widestring decode(common_encoding encoding, const Tsrcstring & src) {
	static_assert(sizeof(typename Tsrcstring::value_type) == sizeof(char));
	static_assert(mpt::is_character<typename Tsrcstring::value_type>::value);
#if MPT_OS_WINDOWS
	if (windows_has_encoding(encoding)) {
		return decode_codepage(codepage_from_encoding(encoding), src);
	}
#endif
	switch (encoding) {
		case common_encoding::utf8:
			return decode_utf8(src);
			break;
		case common_encoding::ascii:
			return decode_ascii(src);
			break;
		case common_encoding::iso8859_1:
			return decode_iso8859_1(src);
			break;
		case common_encoding::iso8859_15:
			return decode_8bit(src, CharsetTableISO8859_15);
			break;
		case common_encoding::cp437:
			return decode_8bit(src, CharsetTableCP437);
			break;
		case common_encoding::cp850:
			return decode_8bit(src, CharsetTableCP850);
			break;
		case common_encoding::windows1252:
			return decode_8bit(src, CharsetTableWindows1252);
			break;
		case common_encoding::amiga:
			return decode_8bit(src, CharsetTableAmiga);
			break;
		case common_encoding::riscos:
			return decode_8bit(src, CharsetTableRISC_OS);
			break;
		case common_encoding::iso8859_1_no_c1:
			return decode_8bit_no_c1(src, CharsetTableISO8859_1);
			break;
		case common_encoding::iso8859_15_no_c1:
			return decode_8bit_no_c1(src, CharsetTableISO8859_15);
			break;
		case common_encoding::amiga_no_c1:
			return decode_8bit_no_c1(src, CharsetTableAmiga);
			break;
	}
	throw std::domain_error("unsupported encoding");
}

template <typename Tsrcstring>
inline mpt::widestring decode(logical_encoding encoding, const Tsrcstring & src) {
	static_assert(sizeof(typename Tsrcstring::value_type) == sizeof(char));
	static_assert(mpt::is_character<typename Tsrcstring::value_type>::value);
#if MPT_OS_WINDOWS
	if (windows_has_encoding(encoding)) {
		return decode_codepage(codepage_from_encoding(encoding), src);
	}
#endif
#if MPT_OS_DJGPP
	switch (encoding) {
		case logical_encoding::locale:
			return decode(djgpp_get_locale_encoding(), src);
			break;
		case logical_encoding::active_locale:
			return decode(djgpp_get_locale_encoding(), src);
			break;
	}
	throw std::domain_error("unsupported encoding");
#elif !defined(MPT_COMPILER_QUIRK_NO_WCHAR)
	switch (encoding) {
		case logical_encoding::locale:
			return decode_locale(std::locale(""), src);
			break;
		case logical_encoding::active_locale:
			return decode_locale(std::locale(), src);
			break;
	}
	throw std::domain_error("unsupported encoding");
#else
	throw std::domain_error("unsupported encoding");
#endif
}



inline bool is_utf8(const std::string & str) {
	return (str == encode<std::string>(common_encoding::utf8, decode<std::string>(common_encoding::utf8, str)));
}



template <typename Tstring>
struct string_transcoder {
};

template <logical_encoding encoding>
struct string_transcoder<std::basic_string<char, logical_encoding_char_traits<encoding>>> {
	using string_type = std::basic_string<char, logical_encoding_char_traits<encoding>>;
	static inline mpt::widestring decode(const string_type & src) {
		return mpt::decode<string_type>(encoding, src);
	}
	static inline string_type encode(const mpt::widestring & src) {
		return mpt::encode<string_type>(encoding, src);
	}
};

template <common_encoding encoding>
struct string_transcoder<std::basic_string<char, common_encoding_char_traits<encoding>>> {
	using string_type = std::basic_string<char, common_encoding_char_traits<encoding>>;
	static inline mpt::widestring decode(const string_type & src) {
		return mpt::decode<string_type>(encoding, src);
	}
	static inline string_type encode(const mpt::widestring & src) {
		return mpt::encode<string_type>(encoding, src);
	}
};

#if !defined(MPT_COMPILER_QUIRK_NO_WCHAR)
template <>
struct string_transcoder<std::wstring> {
	using string_type = std::wstring;
	static inline mpt::widestring decode(const string_type & src) {
		return src;
	}
	static inline string_type encode(const mpt::widestring & src) {
		return src;
	}
};
#endif // !MPT_COMPILER_QUIRK_NO_WCHAR

#if MPT_CXX_AT_LEAST(20)
template <>
struct string_transcoder<std::u8string> {
	using string_type = std::u8string;
	static inline mpt::widestring decode(const string_type & src) {
		return mpt::decode_utf8<string_type>(src);
	}
	static inline string_type encode(const mpt::widestring & src) {
		return mpt::encode_utf8<string_type>(src);
	}
};
#endif // C++10

template <>
struct string_transcoder<std::u16string> {
	using string_type = std::u16string;
	static inline mpt::widestring decode(const string_type & src) {
		if constexpr (sizeof(mpt::widechar) == sizeof(char16_t)) {
			return mpt::widestring(src.begin(), src.end());
		} else {
			return utf32_from_utf16<mpt::widestring, std::u16string>(src);
		}
	}
	static inline string_type encode(const mpt::widestring & src) {
		if constexpr (sizeof(mpt::widechar) == sizeof(char16_t)) {
			return string_type(src.begin(), src.end());
		} else {
			return utf16_from_utf32<std::u16string, mpt::widestring>(src);
		}
	}
};

#if defined(MPT_COMPILER_QUIRK_NO_WCHAR)
template <>
struct string_transcoder<std::u32string> {
	using string_type = std::u32string;
	static inline mpt::widestring decode(const string_type & src) {
		return src;
	}
	static inline string_type encode(const mpt::widestring & src) {
		return src;
	}
};
#else  // !MPT_COMPILER_QUIRK_NO_WCHAR
template <>
struct string_transcoder<std::u32string> {
	using string_type = std::u32string;
	static inline mpt::widestring decode(const string_type & src) {
		if constexpr (sizeof(mpt::widechar) == sizeof(char32_t)) {
			return mpt::widestring(src.begin(), src.end());
		} else {
			return utf16_from_utf32<mpt::widestring, std::u32string>(src);
		}
	}
	static inline string_type encode(const mpt::widestring & src) {
		if constexpr (sizeof(mpt::widechar) == sizeof(char32_t)) {
			return string_type(src.begin(), src.end());
		} else {
			return utf32_from_utf16<std::u32string, mpt::widestring>(src);
		}
	}
};
#endif // MPT_COMPILER_QUIRK_NO_WCHAR

#if MPT_DETECTED_MFC

template <>
struct string_transcoder<CStringW> {
	using string_type = CStringW;
	static inline mpt::widestring decode(const string_type & src) {
		return mpt::widestring(src.GetString());
	}
	static inline string_type encode(const mpt::widestring & src) {
		return src.c_str();
	}
};

template <>
struct string_transcoder<CStringA> {
	using string_type = CStringA;
	static inline mpt::widestring decode(const string_type & src) {
		return mpt::decode<std::string>(mpt::logical_encoding::locale, std::string(src.GetString()));
	}
	static inline string_type encode(const mpt::widestring & src) {
		return mpt::encode<std::string>(mpt::logical_encoding::locale, src).c_str();
	}
};

#endif // MPT_DETECTED_MFC

template <typename Tdststring, typename Tsrcstring, std::enable_if_t<mpt::is_string_type<typename mpt::make_string_type<Tsrcstring>::type>::value, bool> = true>
inline Tdststring transcode(const Tsrcstring & src) {
	if constexpr (std::is_same<Tdststring, typename mpt::make_string_type<Tsrcstring>::type>::value) {
		return mpt::as_string(src);
	} else {
		return string_transcoder<Tdststring>::encode(string_transcoder<decltype(mpt::as_string(src))>::decode(mpt::as_string(src)));
	}
}

template <typename Tdststring, typename Tsrcstring, typename Tencoding, std::enable_if_t<std::is_same<Tdststring, std::string>::value, bool> = true, std::enable_if_t<mpt::is_string_type<typename mpt::make_string_type<Tsrcstring>::type>::value, bool> = true>
inline Tdststring transcode(Tencoding to, const Tsrcstring & src) {
	return mpt::encode<Tdststring>(to, string_transcoder<decltype(mpt::as_string(src))>::decode(mpt::as_string(src)));
}

template <typename Tdststring, typename Tsrcstring, typename Tencoding, std::enable_if_t<std::is_same<typename mpt::make_string_type<Tsrcstring>::type, std::string>::value, bool> = true, std::enable_if_t<mpt::is_string_type<typename mpt::make_string_type<Tsrcstring>::type>::value, bool> = true>
inline Tdststring transcode(Tencoding from, const Tsrcstring & src) {
	return string_transcoder<Tdststring>::encode(mpt::decode<decltype(mpt::as_string(src))>(from, mpt::as_string(src)));
}

template <typename Tdststring, typename Tsrcstring, typename Tto, typename Tfrom, std::enable_if_t<mpt::is_string_type<typename mpt::make_string_type<Tsrcstring>::type>::value, bool> = true>
inline Tdststring transcode(Tto to, Tfrom from, const Tsrcstring & src) {
	return mpt::encode<Tdststring>(to, mpt::decode<decltype(mpt::as_string(src))>(from, mpt::as_string(src)));
}



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_STRING_TRANSCODE_TRANSCODE_HPP
