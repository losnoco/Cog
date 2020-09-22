/*
 * openmpt123.hpp
 * --------------
 * Purpose: libopenmpt command line player
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef OPENMPT123_HPP
#define OPENMPT123_HPP

#include "openmpt123_config.hpp"

namespace openmpt123 {

struct exception : public openmpt::exception {
	exception( const std::string & text ) : openmpt::exception(text) { }
};

struct show_help_exception {
	std::string message;
	bool longhelp;
	show_help_exception( const std::string & msg = "", bool longhelp_ = true ) : message(msg), longhelp(longhelp_) { }
};

struct args_error_exception {
	args_error_exception() { }
};

struct show_help_keyboard_exception { };

#if defined(WIN32)
bool IsConsole( DWORD stdHandle );
#endif
bool IsTerminal( int fd );



#if defined(WIN32)



static inline std::string wstring_to_utf8( const std::wstring & unicode_string ) {
	std::string utf8_string;
	int source_length = ( unicode_string.size() < static_cast<unsigned int>( std::numeric_limits<int>::max() ) ) ? static_cast<int>( unicode_string.size() ) : std::numeric_limits<int>::max();
	int required_size = WideCharToMultiByte( CP_UTF8, 0, unicode_string.data(), source_length, NULL, 0, NULL, NULL );
	if ( required_size > 0 ) {
		utf8_string.resize( required_size );
		WideCharToMultiByte( CP_UTF8, 0, unicode_string.data(), source_length, utf8_string.data(), required_size, NULL, NULL );
	}
	return utf8_string;
}

static inline std::wstring utf8_to_wstring( const std::string & utf8_string ) {
	std::wstring unicode_string;
	int source_length = ( utf8_string.size() < static_cast<unsigned int>( std::numeric_limits<int>::max() ) ) ? static_cast<int>( utf8_string.size() ) : std::numeric_limits<int>::max();
	int required_size = MultiByteToWideChar( CP_UTF8, 0, utf8_string.data(), source_length, NULL, 0 );
	if ( required_size > 0 ) {
		unicode_string.resize( required_size );
		MultiByteToWideChar( CP_UTF8, 0, utf8_string.data(), source_length, unicode_string.data(), required_size );
	}
	return unicode_string;
}

static inline std::string wstring_to_locale( const std::wstring & unicode_string ) {
	std::string locale_string;
	int source_length = ( unicode_string.size() < static_cast<unsigned int>( std::numeric_limits<int>::max() ) ) ? static_cast<int>( unicode_string.size() ) : std::numeric_limits<int>::max();
	int required_size = WideCharToMultiByte( CP_ACP, 0, unicode_string.data(), source_length, NULL, 0, NULL, NULL );
	if ( required_size > 0 ) {
		locale_string.resize( required_size );
		WideCharToMultiByte( CP_ACP, 0, unicode_string.data(), source_length, locale_string.data(), required_size, NULL, NULL );
	}
	return locale_string;
}

static inline std::wstring locale_to_wstring( const std::string & locale_string ) {
	std::wstring unicode_string;
	int source_length = ( locale_string.size() < static_cast<unsigned int>( std::numeric_limits<int>::max() ) ) ? static_cast<int>( locale_string.size() ) : std::numeric_limits<int>::max();
	int required_size = MultiByteToWideChar( CP_ACP, 0, locale_string.data(), source_length, NULL, 0 );
	if ( required_size > 0 ) {
		unicode_string.resize( required_size );
		MultiByteToWideChar( CP_ACP, 0, locale_string.data(), source_length, unicode_string.data(), required_size );
	}
	return unicode_string;
}



#endif



/*
default 1:1 mapping
static constexpr char32_t CharsetTableISO8859_1[256] = {
	0x0000,0x0001,0x0002,0x0003,0x0004,0x0005,0x0006,0x0007,0x0008,0x0009,0x000a,0x000b,0x000c,0x000d,0x000e,0x000f,
	0x0010,0x0011,0x0012,0x0013,0x0014,0x0015,0x0016,0x0017,0x0018,0x0019,0x001a,0x001b,0x001c,0x001d,0x001e,0x001f,
	0x0020,0x0021,0x0022,0x0023,0x0024,0x0025,0x0026,0x0027,0x0028,0x0029,0x002a,0x002b,0x002c,0x002d,0x002e,0x002f,
	0x0030,0x0031,0x0032,0x0033,0x0034,0x0035,0x0036,0x0037,0x0038,0x0039,0x003a,0x003b,0x003c,0x003d,0x003e,0x003f,
	0x0040,0x0041,0x0042,0x0043,0x0044,0x0045,0x0046,0x0047,0x0048,0x0049,0x004a,0x004b,0x004c,0x004d,0x004e,0x004f,
	0x0050,0x0051,0x0052,0x0053,0x0054,0x0055,0x0056,0x0057,0x0058,0x0059,0x005a,0x005b,0x005c,0x005d,0x005e,0x005f,
	0x0060,0x0061,0x0062,0x0063,0x0064,0x0065,0x0066,0x0067,0x0068,0x0069,0x006a,0x006b,0x006c,0x006d,0x006e,0x006f,
	0x0070,0x0071,0x0072,0x0073,0x0074,0x0075,0x0076,0x0077,0x0078,0x0079,0x007a,0x007b,0x007c,0x007d,0x007e,0x007f,
	0x0080,0x0081,0x0082,0x0083,0x0084,0x0085,0x0086,0x0087,0x0088,0x0089,0x008a,0x008b,0x008c,0x008d,0x008e,0x008f,
	0x0090,0x0091,0x0092,0x0093,0x0094,0x0095,0x0096,0x0097,0x0098,0x0099,0x009a,0x009b,0x009c,0x009d,0x009e,0x009f,
	0x00a0,0x00a1,0x00a2,0x00a3,0x00a4,0x00a5,0x00a6,0x00a7,0x00a8,0x00a9,0x00aa,0x00ab,0x00ac,0x00ad,0x00ae,0x00af,
	0x00b0,0x00b1,0x00b2,0x00b3,0x00b4,0x00b5,0x00b6,0x00b7,0x00b8,0x00b9,0x00ba,0x00bb,0x00bc,0x00bd,0x00be,0x00bf,
	0x00c0,0x00c1,0x00c2,0x00c3,0x00c4,0x00c5,0x00c6,0x00c7,0x00c8,0x00c9,0x00ca,0x00cb,0x00cc,0x00cd,0x00ce,0x00cf,
	0x00d0,0x00d1,0x00d2,0x00d3,0x00d4,0x00d5,0x00d6,0x00d7,0x00d8,0x00d9,0x00da,0x00db,0x00dc,0x00dd,0x00de,0x00df,
	0x00e0,0x00e1,0x00e2,0x00e3,0x00e4,0x00e5,0x00e6,0x00e7,0x00e8,0x00e9,0x00ea,0x00eb,0x00ec,0x00ed,0x00ee,0x00ef,
	0x00f0,0x00f1,0x00f2,0x00f3,0x00f4,0x00f5,0x00f6,0x00f7,0x00f8,0x00f9,0x00fa,0x00fb,0x00fc,0x00fd,0x00fe,0x00ff
};
*/

static constexpr char32_t CharsetTableISO8859_15[256] = {
	0x0000,0x0001,0x0002,0x0003,0x0004,0x0005,0x0006,0x0007,0x0008,0x0009,0x000a,0x000b,0x000c,0x000d,0x000e,0x000f,
	0x0010,0x0011,0x0012,0x0013,0x0014,0x0015,0x0016,0x0017,0x0018,0x0019,0x001a,0x001b,0x001c,0x001d,0x001e,0x001f,
	0x0020,0x0021,0x0022,0x0023,0x0024,0x0025,0x0026,0x0027,0x0028,0x0029,0x002a,0x002b,0x002c,0x002d,0x002e,0x002f,
	0x0030,0x0031,0x0032,0x0033,0x0034,0x0035,0x0036,0x0037,0x0038,0x0039,0x003a,0x003b,0x003c,0x003d,0x003e,0x003f,
	0x0040,0x0041,0x0042,0x0043,0x0044,0x0045,0x0046,0x0047,0x0048,0x0049,0x004a,0x004b,0x004c,0x004d,0x004e,0x004f,
	0x0050,0x0051,0x0052,0x0053,0x0054,0x0055,0x0056,0x0057,0x0058,0x0059,0x005a,0x005b,0x005c,0x005d,0x005e,0x005f,
	0x0060,0x0061,0x0062,0x0063,0x0064,0x0065,0x0066,0x0067,0x0068,0x0069,0x006a,0x006b,0x006c,0x006d,0x006e,0x006f,
	0x0070,0x0071,0x0072,0x0073,0x0074,0x0075,0x0076,0x0077,0x0078,0x0079,0x007a,0x007b,0x007c,0x007d,0x007e,0x007f,
	0x0080,0x0081,0x0082,0x0083,0x0084,0x0085,0x0086,0x0087,0x0088,0x0089,0x008a,0x008b,0x008c,0x008d,0x008e,0x008f,
	0x0090,0x0091,0x0092,0x0093,0x0094,0x0095,0x0096,0x0097,0x0098,0x0099,0x009a,0x009b,0x009c,0x009d,0x009e,0x009f,
	0x00a0,0x00a1,0x00a2,0x00a3,0x20ac,0x00a5,0x0160,0x00a7,0x0161,0x00a9,0x00aa,0x00ab,0x00ac,0x00ad,0x00ae,0x00af,
	0x00b0,0x00b1,0x00b2,0x00b3,0x017d,0x00b5,0x00b6,0x00b7,0x017e,0x00b9,0x00ba,0x00bb,0x0152,0x0153,0x0178,0x00bf,
	0x00c0,0x00c1,0x00c2,0x00c3,0x00c4,0x00c5,0x00c6,0x00c7,0x00c8,0x00c9,0x00ca,0x00cb,0x00cc,0x00cd,0x00ce,0x00cf,
	0x00d0,0x00d1,0x00d2,0x00d3,0x00d4,0x00d5,0x00d6,0x00d7,0x00d8,0x00d9,0x00da,0x00db,0x00dc,0x00dd,0x00de,0x00df,
	0x00e0,0x00e1,0x00e2,0x00e3,0x00e4,0x00e5,0x00e6,0x00e7,0x00e8,0x00e9,0x00ea,0x00eb,0x00ec,0x00ed,0x00ee,0x00ef,
	0x00f0,0x00f1,0x00f2,0x00f3,0x00f4,0x00f5,0x00f6,0x00f7,0x00f8,0x00f9,0x00fa,0x00fb,0x00fc,0x00fd,0x00fe,0x00ff
};

static constexpr char32_t CharsetTableWindows1252[256] = {
	0x0000,0x0001,0x0002,0x0003,0x0004,0x0005,0x0006,0x0007,0x0008,0x0009,0x000a,0x000b,0x000c,0x000d,0x000e,0x000f,
	0x0010,0x0011,0x0012,0x0013,0x0014,0x0015,0x0016,0x0017,0x0018,0x0019,0x001a,0x001b,0x001c,0x001d,0x001e,0x001f,
	0x0020,0x0021,0x0022,0x0023,0x0024,0x0025,0x0026,0x0027,0x0028,0x0029,0x002a,0x002b,0x002c,0x002d,0x002e,0x002f,
	0x0030,0x0031,0x0032,0x0033,0x0034,0x0035,0x0036,0x0037,0x0038,0x0039,0x003a,0x003b,0x003c,0x003d,0x003e,0x003f,
	0x0040,0x0041,0x0042,0x0043,0x0044,0x0045,0x0046,0x0047,0x0048,0x0049,0x004a,0x004b,0x004c,0x004d,0x004e,0x004f,
	0x0050,0x0051,0x0052,0x0053,0x0054,0x0055,0x0056,0x0057,0x0058,0x0059,0x005a,0x005b,0x005c,0x005d,0x005e,0x005f,
	0x0060,0x0061,0x0062,0x0063,0x0064,0x0065,0x0066,0x0067,0x0068,0x0069,0x006a,0x006b,0x006c,0x006d,0x006e,0x006f,
	0x0070,0x0071,0x0072,0x0073,0x0074,0x0075,0x0076,0x0077,0x0078,0x0079,0x007a,0x007b,0x007c,0x007d,0x007e,0x007f,
	0x20ac,0x0081,0x201a,0x0192,0x201e,0x2026,0x2020,0x2021,0x02c6,0x2030,0x0160,0x2039,0x0152,0x008d,0x017d,0x008f,
	0x0090,0x2018,0x2019,0x201c,0x201d,0x2022,0x2013,0x2014,0x02dc,0x2122,0x0161,0x203a,0x0153,0x009d,0x017e,0x0178,
	0x00a0,0x00a1,0x00a2,0x00a3,0x00a4,0x00a5,0x00a6,0x00a7,0x00a8,0x00a9,0x00aa,0x00ab,0x00ac,0x00ad,0x00ae,0x00af,
	0x00b0,0x00b1,0x00b2,0x00b3,0x00b4,0x00b5,0x00b6,0x00b7,0x00b8,0x00b9,0x00ba,0x00bb,0x00bc,0x00bd,0x00be,0x00bf,
	0x00c0,0x00c1,0x00c2,0x00c3,0x00c4,0x00c5,0x00c6,0x00c7,0x00c8,0x00c9,0x00ca,0x00cb,0x00cc,0x00cd,0x00ce,0x00cf,
	0x00d0,0x00d1,0x00d2,0x00d3,0x00d4,0x00d5,0x00d6,0x00d7,0x00d8,0x00d9,0x00da,0x00db,0x00dc,0x00dd,0x00de,0x00df,
	0x00e0,0x00e1,0x00e2,0x00e3,0x00e4,0x00e5,0x00e6,0x00e7,0x00e8,0x00e9,0x00ea,0x00eb,0x00ec,0x00ed,0x00ee,0x00ef,
	0x00f0,0x00f1,0x00f2,0x00f3,0x00f4,0x00f5,0x00f6,0x00f7,0x00f8,0x00f9,0x00fa,0x00fb,0x00fc,0x00fd,0x00fe,0x00ff
};

static constexpr char32_t CharsetTableCP437[256] = {
	0x0000,0x0001,0x0002,0x0003,0x0004,0x0005,0x0006,0x0007,0x0008,0x0009,0x000a,0x000b,0x000c,0x000d,0x000e,0x000f,
	0x0010,0x0011,0x0012,0x0013,0x0014,0x0015,0x0016,0x0017,0x0018,0x0019,0x001a,0x001b,0x001c,0x001d,0x001e,0x001f,
	0x0020,0x0021,0x0022,0x0023,0x0024,0x0025,0x0026,0x0027,0x0028,0x0029,0x002a,0x002b,0x002c,0x002d,0x002e,0x002f,
	0x0030,0x0031,0x0032,0x0033,0x0034,0x0035,0x0036,0x0037,0x0038,0x0039,0x003a,0x003b,0x003c,0x003d,0x003e,0x003f,
	0x0040,0x0041,0x0042,0x0043,0x0044,0x0045,0x0046,0x0047,0x0048,0x0049,0x004a,0x004b,0x004c,0x004d,0x004e,0x004f,
	0x0050,0x0051,0x0052,0x0053,0x0054,0x0055,0x0056,0x0057,0x0058,0x0059,0x005a,0x005b,0x005c,0x005d,0x005e,0x005f,
	0x0060,0x0061,0x0062,0x0063,0x0064,0x0065,0x0066,0x0067,0x0068,0x0069,0x006a,0x006b,0x006c,0x006d,0x006e,0x006f,
	0x0070,0x0071,0x0072,0x0073,0x0074,0x0075,0x0076,0x0077,0x0078,0x0079,0x007a,0x007b,0x007c,0x007d,0x007e,0x2302,
	0x00c7,0x00fc,0x00e9,0x00e2,0x00e4,0x00e0,0x00e5,0x00e7,0x00ea,0x00eb,0x00e8,0x00ef,0x00ee,0x00ec,0x00c4,0x00c5,
	0x00c9,0x00e6,0x00c6,0x00f4,0x00f6,0x00f2,0x00fb,0x00f9,0x00ff,0x00d6,0x00dc,0x00a2,0x00a3,0x00a5,0x20a7,0x0192,
	0x00e1,0x00ed,0x00f3,0x00fa,0x00f1,0x00d1,0x00aa,0x00ba,0x00bf,0x2310,0x00ac,0x00bd,0x00bc,0x00a1,0x00ab,0x00bb,
	0x2591,0x2592,0x2593,0x2502,0x2524,0x2561,0x2562,0x2556,0x2555,0x2563,0x2551,0x2557,0x255d,0x255c,0x255b,0x2510,
	0x2514,0x2534,0x252c,0x251c,0x2500,0x253c,0x255e,0x255f,0x255a,0x2554,0x2569,0x2566,0x2560,0x2550,0x256c,0x2567,
	0x2568,0x2564,0x2565,0x2559,0x2558,0x2552,0x2553,0x256b,0x256a,0x2518,0x250c,0x2588,0x2584,0x258c,0x2590,0x2580,
	0x03b1,0x00df,0x0393,0x03c0,0x03a3,0x03c3,0x00b5,0x03c4,0x03a6,0x0398,0x03a9,0x03b4,0x221e,0x03c6,0x03b5,0x2229,
	0x2261,0x00b1,0x2265,0x2264,0x2320,0x2321,0x00f7,0x2248,0x00b0,0x2219,0x00b7,0x221a,0x207f,0x00b2,0x25a0,0x00a0
};

static constexpr char32_t CharsetTableCP850[256] = {
	0x0000,0x0001,0x0002,0x0003,0x0004,0x0005,0x0006,0x0007,0x0008,0x0009,0x000a,0x000b,0x000c,0x000d,0x000e,0x000f,
	0x0010,0x0011,0x0012,0x0013,0x0014,0x0015,0x0016,0x0017,0x0018,0x0019,0x001a,0x001b,0x001c,0x001d,0x001e,0x001f,
	0x0020,0x0021,0x0022,0x0023,0x0024,0x0025,0x0026,0x0027,0x0028,0x0029,0x002a,0x002b,0x002c,0x002d,0x002e,0x002f,
	0x0030,0x0031,0x0032,0x0033,0x0034,0x0035,0x0036,0x0037,0x0038,0x0039,0x003a,0x003b,0x003c,0x003d,0x003e,0x003f,
	0x0040,0x0041,0x0042,0x0043,0x0044,0x0045,0x0046,0x0047,0x0048,0x0049,0x004a,0x004b,0x004c,0x004d,0x004e,0x004f,
	0x0050,0x0051,0x0052,0x0053,0x0054,0x0055,0x0056,0x0057,0x0058,0x0059,0x005a,0x005b,0x005c,0x005d,0x005e,0x005f,
	0x0060,0x0061,0x0062,0x0063,0x0064,0x0065,0x0066,0x0067,0x0068,0x0069,0x006a,0x006b,0x006c,0x006d,0x006e,0x006f,
	0x0070,0x0071,0x0072,0x0073,0x0074,0x0075,0x0076,0x0077,0x0078,0x0079,0x007a,0x007b,0x007c,0x007d,0x007e,0x2302,
	0x00c7,0x00fc,0x00e9,0x00e2,0x00e4,0x00e0,0x00e5,0x00e7,0x00ea,0x00eb,0x00e8,0x00ef,0x00ee,0x00ec,0x00c4,0x00c5,
	0x00c9,0x00e6,0x00c6,0x00f4,0x00f6,0x00f2,0x00fb,0x00f9,0x00ff,0x00d6,0x00dc,0x00F8,0x00a3,0x00D8,0x00D7,0x0192,
	0x00e1,0x00ed,0x00f3,0x00fa,0x00f1,0x00d1,0x00aa,0x00ba,0x00bf,0x00AE,0x00ac,0x00bd,0x00bc,0x00a1,0x00ab,0x00bb,
	0x2591,0x2592,0x2593,0x2502,0x2524,0x00C1,0x00C2,0x00C0,0x00A9,0x2563,0x2551,0x2557,0x255d,0x00A2,0x00A5,0x2510,
	0x2514,0x2534,0x252c,0x251c,0x2500,0x253c,0x00E3,0x00C3,0x255a,0x2554,0x2569,0x2566,0x2560,0x2550,0x256c,0x00A4,
	0x00F0,0x00D0,0x00CA,0x00CB,0x00C8,0x0131,0x00CD,0x00CE,0x00CF,0x2518,0x250c,0x2588,0x2584,0x00A6,0x00CC,0x2580,
	0x00D3,0x00df,0x00D4,0x00D2,0x00F5,0x00D5,0x00b5,0x00FE,0x00DE,0x00DA,0x00DB,0x00D9,0x00FD,0x00DD,0x00AF,0x00B4,
	0x00AD,0x00b1,0x2017,0x00BE,0x00B6,0x00A7,0x00f7,0x00B8,0x00b0,0x00A8,0x00b7,0x00B9,0x00B3,0x00b2,0x25a0,0x00a0
};


#if defined(__DJGPP__)
using widechar = char32_t;
using widestring = std::u32string;
static constexpr widechar wide_default_replacement = 0xFFFD;
#else // !__DJGPP__
using widechar = wchar_t;
using widestring = std::wstring;
static constexpr widechar wide_default_replacement = L'\uFFFD';
#endif // !__DJGPP__


static inline widestring From8bit(const std::string &str, const char32_t (&table)[256], widechar replacement = wide_default_replacement)
{
	widestring res;
	res.reserve(str.length());
	for(std::size_t i = 0; i < str.length(); ++i)
	{
		std::size_t c = static_cast<std::size_t>(static_cast<std::uint8_t>(str[i]));
		if(c < std::size(table))
		{
			res.push_back(static_cast<widechar>(table[c]));
		} else
		{
			res.push_back(replacement);
		}
	}
	return res;
}

static inline std::string To8bit(const widestring &str, const char32_t (&table)[256], char replacement = '?')
{
	std::string res;
	res.reserve(str.length());
	for(std::size_t i = 0; i < str.length(); ++i)
	{
		char32_t c = static_cast<char32_t>(str[i]);
		bool found = false;
		// Try non-control characters first.
		// In cases where there are actual characters mirrored in this range (like in AMS/AMS2 character sets),
		// characters in the common range are preferred this way.
		for(std::size_t x = 0x20; x < std::size(table); ++x)
		{
			if(c == table[x])
			{
				res.push_back(static_cast<char>(static_cast<std::uint8_t>(x)));
				found = true;
				break;
			}
		}
		if(!found)
		{
			// try control characters
			for(std::size_t x = 0x00; x < std::size(table) && x < 0x20; ++x)
			{
				if(c == table[x])
				{
					res.push_back(static_cast<char>(static_cast<std::uint8_t>(x)));
					found = true;
					break;
				}
			}
		}
		if(!found)
		{
			res.push_back(replacement);
		}
	}
	return res;
}

static inline widestring FromAscii(const std::string &str, widechar replacement = wide_default_replacement)
{
	widestring res;
	res.reserve(str.length());
	for(std::size_t i = 0; i < str.length(); ++i)
	{
		std::uint8_t c = str[i];
		if(c <= 0x7f)
		{
			res.push_back(static_cast<widechar>(static_cast<std::uint32_t>(c)));
		} else
		{
			res.push_back(replacement);
		}
	}
	return res;
}

static inline std::string ToAscii(const widestring &str, char replacement = '?')
{
	std::string res;
	res.reserve(str.length());
	for(std::size_t i = 0; i < str.length(); ++i)
	{
		char32_t c = static_cast<char32_t>(str[i]);
		if(c <= 0x7f)
		{
			res.push_back(static_cast<char>(static_cast<std::uint8_t>(c)));
		} else
		{
			res.push_back(replacement);
		}
	}
	return res;
}

static inline widestring FromISO_8859_1(const std::string &str, widechar replacement = wide_default_replacement)
{
	static_cast<void>( replacement );
	widestring res;
	res.reserve(str.length());
	for(std::size_t i = 0; i < str.length(); ++i)
	{
		std::uint8_t c = str[i];
		res.push_back(static_cast<widechar>(static_cast<std::uint32_t>(c)));
	}
	return res;
}

static inline std::string ToISO_8859_1(const widestring &str, char replacement = '?')
{
	std::string res;
	res.reserve(str.length());
	for(std::size_t i = 0; i < str.length(); ++i)
	{
		char32_t c = static_cast<char32_t>(str[i]);
		if(c <= 0xff)
		{
			res.push_back(static_cast<char>(static_cast<std::uint8_t>(c)));
		} else
		{
			res.push_back(replacement);
		}
	}
	return res;
}


#if !defined(__DJGPP__) && !defined(__EMSCRIPTEN__)

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

static inline std::wstring LocaleDecode(const std::string &str, const std::locale & locale, wchar_t replacement = L'\uFFFD', int retry = 0, bool * progress = nullptr)
{
	if(str.empty())
	{
		return std::wstring();
	}
	std::vector<wchar_t> out;
	using codecvt_type = std::codecvt<wchar_t, char, std::mbstate_t>;
	std::mbstate_t state = std::mbstate_t();
	const codecvt_type & facet = std::use_facet<codecvt_type>(locale);
	codecvt_type::result result = codecvt_type::partial;
	const char * in_begin = str.data();
	const char * in_end = in_begin + str.size();
	out.resize((in_end - in_begin) * (facet.max_length() + 1));
	wchar_t * out_begin = &(out[0]);
	wchar_t * out_end = &(out[0]) + out.size();
	const char * in_next = nullptr;
	wchar_t * out_next = nullptr;
	do
	{
		if(retry == 2)
		{
			for(;;)
			{
				in_next = nullptr;
				out_next = nullptr;
				result = facet.in(state, in_begin, in_begin + 1, in_next, out_begin, out_end, out_next);
				if(result == codecvt_type::partial && in_next == in_begin + 1)
				{
					in_begin = in_next;
					out_begin = out_next;
					continue;
				} else
				{
					break;
				}
			}
		} else
		{
			in_next = nullptr;
			out_next = nullptr;
			result = facet.in(state, in_begin, in_end, in_next, out_begin, out_end, out_next);
		}
		if(result == codecvt_type::partial || (result == codecvt_type::error && out_next == out_end))
		{
			out.resize(out.size() * 2);
			in_begin = in_next;
			out_begin = &(out[0]) + (out_next - out_begin);
			out_end = &(out[0]) + out.size();
			continue;
		}
		if(retry == 0)
		{
			if(result == codecvt_type::error && in_next == in_begin && out_next == out_begin)
			{
				bool made_progress = true;
				LocaleDecode(std::string(" ") + str, locale, replacement, 1, &made_progress);
				if(!made_progress)
				{
					return LocaleDecode(str, locale, replacement, 2);
				}
			}
		} else if(retry == 1)
		{
			if(result == codecvt_type::error && in_next == in_begin && out_next == out_begin)
			{
				*progress = false;
			} else
			{
				*progress = true;
			}
			return std::wstring();
		}
		if(result == codecvt_type::error)
		{
			++in_next;
			*out_next = replacement;
			++out_next;
		}
		in_begin = in_next;
		out_begin = out_next;
	} while((result == codecvt_type::error && in_next < in_end && out_next < out_end) || (retry == 2 && in_next < in_end));
	return std::wstring(&(out[0]), out_next);
}

static inline std::string LocaleEncode(const std::wstring &str, const std::locale & locale, char replacement = '?', int retry = 0, bool * progress = nullptr)
{
	if(str.empty())
	{
		return std::string();
	}
	std::vector<char> out;
	using codecvt_type = std::codecvt<wchar_t, char, std::mbstate_t>;
	std::mbstate_t state = std::mbstate_t();
	const codecvt_type & facet = std::use_facet<codecvt_type>(locale);
	codecvt_type::result result = codecvt_type::partial;
	const wchar_t * in_begin = str.data();
	const wchar_t * in_end = in_begin + str.size();
	out.resize((in_end - in_begin) * (facet.max_length() + 1));
	char * out_begin = &(out[0]);
	char * out_end = &(out[0]) + out.size();
	const wchar_t * in_next = nullptr;
	char * out_next = nullptr;
	do
	{
		if(retry == 2)
		{
			for(;;)
			{
				in_next = nullptr;
				out_next = nullptr;
				result = facet.out(state, in_begin, in_begin + 1, in_next, out_begin, out_end, out_next);
				if(result == codecvt_type::partial && in_next == in_begin + 1)
				{
					in_begin = in_next;
					out_begin = out_next;
					continue;
				} else
				{
					break;
				}
			}
		} else
		{
			in_next = nullptr;
			out_next = nullptr;
			result = facet.out(state, in_begin, in_end, in_next, out_begin, out_end, out_next);
		}
		if(result == codecvt_type::partial || (result == codecvt_type::error && out_next == out_end))
		{
			out.resize(out.size() * 2);
			in_begin = in_next;
			out_begin = &(out[0]) + (out_next - out_begin);
			out_end = &(out[0]) + out.size();
			continue;
		}
		if(retry == 0)
		{
			if(result == codecvt_type::error && in_next == in_begin && out_next == out_begin)
			{
				bool made_progress = true;
				LocaleEncode(std::wstring(L" ") + str, locale, replacement, 1, &made_progress);
				if(!made_progress)
				{
					return LocaleEncode(str, locale, replacement, 2);
				}
			}
		} else if(retry == 1)
		{
			if(result == codecvt_type::error && in_next == in_begin && out_next == out_begin)
			{
				*progress = false;
			} else
			{
				*progress = true;
			}
			return std::string();
		}
		if(result == codecvt_type::error)
		{
			++in_next;
			*out_next = replacement;
			++out_next;
		}
		in_begin = in_next;
		out_begin = out_next;
	} while((result == codecvt_type::error && in_next < in_end && out_next < out_end) || (retry == 2 && in_next < in_end));
	return std::string(&(out[0]), out_next);
}

static inline std::wstring FromLocaleCpp(const std::string &str, wchar_t replacement)
{
	try
	{
		std::locale locale(""); // user locale
		return LocaleDecode(str, locale, replacement);
	} catch ( const std::bad_alloc & )
	{
		throw;
	} catch(...)
	{
		// nothing
	}
	try
	{
		std::locale locale; // current c++ locale
		return LocaleDecode(str, locale, replacement);
	} catch ( const std::bad_alloc & )
	{
		throw;
	} catch(...)
	{
		// nothing
	}
	try
	{
		std::locale locale = std::locale::classic(); // "C" locale
		return LocaleDecode(str, locale, replacement);
	} catch ( const std::bad_alloc & )
	{
		throw;
	} catch(...)
	{
		// nothing
	}
	assert(0);
	return FromAscii(str, replacement); // fallback
}

static inline std::string ToLocaleCpp(const std::wstring &str, char replacement)
{
	try
	{
		std::locale locale(""); // user locale
		return LocaleEncode(str, locale, replacement);
	} catch ( const std::bad_alloc & )
	{
		throw;
	} catch(...)
	{
		// nothing
	}
	try
	{
		std::locale locale; // current c++ locale
		return LocaleEncode(str, locale, replacement);
	} catch ( const std::bad_alloc & )
	{
		throw;
	} catch(...)
	{
		// nothing
	}
	try
	{
		std::locale locale = std::locale::classic(); // "C" locale
		return LocaleEncode(str, locale, replacement);
	} catch ( const std::bad_alloc & )
	{
		throw;
	} catch(...)
	{
		// nothing
	}
	assert(0);
	return ToAscii(str, replacement); // fallback
}


static inline std::wstring FromLocale(const std::string &str, wchar_t replacement = L'\uFFFD')
{
	return FromLocaleCpp(str, replacement);
}

static inline std::string ToLocale(const std::wstring &str, char replacement = '?')
{
	return ToLocaleCpp(str, replacement);
}



#endif // !__DJGPP__ && !__EMSCRIPTEN



static inline widestring FromUTF8(const std::string &str, widechar replacement = wide_default_replacement)
{
	const std::string &in = str;

	widestring out;

	// state:
	std::size_t charsleft = 0;
	char32_t ucs4 = 0;

	for ( std::uint8_t c : in ) {

		if ( charsleft == 0 ) {

			if ( ( c & 0x80 ) == 0x00 ) {
				out.push_back( (wchar_t)c );
			} else if ( ( c & 0xE0 ) == 0xC0 ) {
				ucs4 = c & 0x1F;
				charsleft = 1;
			} else if ( ( c & 0xF0 ) == 0xE0 ) {
				ucs4 = c & 0x0F;
				charsleft = 2;
			} else if ( ( c & 0xF8 ) == 0xF0 ) {
				ucs4 = c & 0x07;
				charsleft = 3;
			} else {
				out.push_back( replacement );
				ucs4 = 0;
				charsleft = 0;
			}

		} else {

			if ( ( c & 0xC0 ) != 0x80 ) {
				out.push_back( replacement );
				ucs4 = 0;
				charsleft = 0;
			}
			ucs4 <<= 6;
			ucs4 |= c & 0x3F;
			charsleft--;

			if ( charsleft == 0 ) {
				if constexpr ( sizeof( widechar ) == 2 ) {
					if ( ucs4 > 0x1fffff ) {
						out.push_back( replacement );
						ucs4 = 0;
						charsleft = 0;
					}
					if ( ucs4 <= 0xffff ) {
						out.push_back( static_cast<widechar>(ucs4) );
					} else {
						std::uint32_t surrogate = static_cast<std::uint32_t>(ucs4) - 0x10000;
						std::uint16_t hi_sur = static_cast<std::uint16_t>( ( 0x36 << 10 ) | ( (surrogate>>10) & ((1<<10)-1) ) );
						std::uint16_t lo_sur = static_cast<std::uint16_t>( ( 0x37 << 10 ) | ( (surrogate>> 0) & ((1<<10)-1) ) );
						out.push_back( hi_sur );
						out.push_back( lo_sur );
					}
				} else {
					out.push_back( static_cast<widechar>( ucs4 ) );
				}
				ucs4 = 0;
			}

		}

	}

	if ( charsleft != 0 ) {
		out.push_back( replacement );
		ucs4 = 0;
		charsleft = 0;
	}

	return out;

}

static inline std::string ToUTF8(const widestring &str, char replacement = '?')
{
	const widestring &in = str;

	std::string out;

	for ( std::size_t i=0; i<in.length(); i++ ) {

		wchar_t wc = in[i];

		char32_t ucs4 = 0;
		if constexpr ( sizeof( widechar ) == 2 ) {
			std::uint16_t c = static_cast<std::uint16_t>( wc );
			if ( i + 1 < in.length() ) {
				// check for surrogate pair
				std::uint16_t hi_sur = in[i+0];
				std::uint16_t lo_sur = in[i+1];
				if ( hi_sur >> 10 == 0x36 && lo_sur >> 10 == 0x37 ) {
					// surrogate pair
					++i;
					hi_sur &= (1<<10)-1;
					lo_sur &= (1<<10)-1;
					ucs4 = ( static_cast<std::uint32_t>(hi_sur) << 10 ) | ( static_cast<std::uint32_t>(lo_sur) << 0 );
				} else {
					// no surrogate pair
					ucs4 = static_cast<char32_t>( c );
				}
			} else {
				// no surrogate possible
				ucs4 = static_cast<char32_t>( c );
			}
		} else {
			ucs4 = static_cast<char32_t>( wc );
		}
		
		if ( ucs4 > 0x1fffff ) {
			out.push_back( replacement );
			continue;
		}

		std::uint8_t utf8[6];
		std::size_t numchars = 0;
		for ( numchars = 0; numchars < 6; numchars++ ) {
			utf8[numchars] = ucs4 & 0x3F;
			ucs4 >>= 6;
			if ( ucs4 == 0 ) {
				break;
			}
		}
		numchars++;

		if ( numchars == 1 ) {
			out.push_back( utf8[0] );
			continue;
		}

		if ( numchars == 2 && utf8[numchars-1] == 0x01 ) {
			// generate shortest form
			out.push_back( utf8[0] | 0x40 );
			continue;
		}

		std::size_t charsleft = numchars;
		while ( charsleft > 0 ) {
			if ( charsleft == numchars ) {
				out.push_back( utf8[ charsleft - 1 ] | ( ((1<<numchars)-1) << (8-numchars) ) );
			} else {
				// cppcheck false-positive
				// cppcheck-suppress arrayIndexOutOfBounds
				out.push_back( utf8[ charsleft - 1 ] | 0x80 );
			}
			charsleft--;
		}

	}

	return out;

}



struct field {
	std::string key;
	std::string val;
	field( const std::string & key )
		: key(key)
	{
		return;
	}
};

class textout : public std::ostringstream {
public:
	textout() {
		return;
	}
	virtual ~textout() {
		return;
	}
protected:
	std::string pop() {
		std::string text = str();
		str(std::string());
		return text;
	}
public:
	virtual void writeout() = 0;
	virtual void cursor_up( std::size_t lines ) {
		static_cast<void>( lines );
	}
};

class textout_dummy : public textout {
public:
	textout_dummy() {
		return;
	}
	virtual ~textout_dummy() {
		return;
	}
public:
	void writeout() override {
		static_cast<void>( pop() );
	}
};

class textout_ostream : public textout {
private:
	std::ostream & s;
#if defined(__DJGPP__)
	std::uint16_t active_codepage;
	std::uint16_t system_codepage;
#endif
public:
	textout_ostream( std::ostream & s_ )
		: s(s_)
#if defined(__DJGPP__)
		, active_codepage(437)
		, system_codepage(437)
#endif
	{
		#if defined(__DJGPP__)
			__dpmi_regs regs;
			std::memset( &regs, 0, sizeof( __dpmi_regs ) );
			regs.x.ax = 0x6601;
			if ( __dpmi_int( 0x21, &regs ) == 0 ) {
				int cf = ( regs.x.flags >> 0 ) & 1;
				if ( cf == 0 ) {
					active_codepage = regs.x.bx;
					system_codepage = regs.x.dx;
				}
			}
		#endif
		return;
	}
	virtual ~textout_ostream() {
		writeout_impl();
	}
private:
	void writeout_impl() {
		std::string text = pop();
		if ( text.length() > 0 ) {
			#if defined(__DJGPP__)
				if ( active_codepage == 0 ) {
					s << To8bit( FromUTF8( text ), CharsetTableCP437 );
				} else if ( active_codepage == 437 ) {
					s << To8bit( FromUTF8( text ), CharsetTableCP437 );
				} else if ( active_codepage == 850 ) {
					s << To8bit( FromUTF8( text ), CharsetTableCP850 );
				} else if ( system_codepage == 437 ) {
					s << To8bit( FromUTF8( text ), CharsetTableCP437 );
				} else if ( system_codepage == 850 ) {
					s << To8bit( FromUTF8( text ), CharsetTableCP850 );
				} else {
					s << To8bit( FromUTF8( text ), CharsetTableCP437 );
				}
			#elif defined(__EMSCRIPTEN__)
				s << text;
			#elif defined(WIN32)
				s << ToLocale( FromUTF8( text ) );
			#else
				s << ToLocale( FromUTF8( text ) );
			#endif
			s.flush();
		}	
	}
public:
	void writeout() override {
		writeout_impl();
	}
	void cursor_up( std::size_t lines ) override {
		s.flush();
		for ( std::size_t line = 0; line < lines; ++line ) {
			*this << "\x1b[1A";
		}
	}
};

#if defined(WIN32)

class textout_ostream_console : public textout {
private:
	std::wostream & s;
	HANDLE handle;
	bool console;
public:
	textout_ostream_console( std::wostream & s_, DWORD stdHandle_ )
		: s(s_)
		, handle(GetStdHandle( stdHandle_ ))
		, console(IsConsole( stdHandle_ ))
	{
		return;
	}
	virtual ~textout_ostream_console() {
		writeout_impl();
	}
private:
	void writeout_impl() {
		std::string text = pop();
		if ( text.length() > 0 ) {
			if ( console ) {
				#if defined(UNICODE)
					std::wstring wtext = utf8_to_wstring( text );
					WriteConsole( handle, wtext.data(), static_cast<DWORD>( wtext.size() ), NULL, NULL );
				#else
					std::string ltext = wstring_to_locale( utf8_to_wstring( text ) );
					WriteConsole( handle, ltext.data(), static_cast<DWORD>( ltext.size() ), NULL, NULL );
				#endif
			} else {
				#if defined(UNICODE)
					s << utf8_to_wstring( text );
				#else
					s << wstring_to_locale( utf8_to_wstring( text ) );
				#endif
				s.flush();
			}
		}
	}
public:
	void writeout() override {
		writeout_impl();
	}
	void cursor_up( std::size_t lines ) override {
		if ( console ) {
			s.flush();
			CONSOLE_SCREEN_BUFFER_INFO csbi;
			ZeroMemory( &csbi, sizeof( CONSOLE_SCREEN_BUFFER_INFO ) );
			COORD coord_cursor = COORD();
			if ( GetConsoleScreenBufferInfo( handle, &csbi ) != FALSE ) {
				coord_cursor = csbi.dwCursorPosition;
				coord_cursor.X = 1;
				coord_cursor.Y -= static_cast<SHORT>( lines );
				SetConsoleCursorPosition( handle, coord_cursor );
			}
		}
	}
};

#endif // WIN32

static inline float mpt_round( float val ) {
	if ( val >= 0.0f ) {
		return std::floor( val + 0.5f );
	} else {
		return std::ceil( val - 0.5f );
	}
}

static inline long mpt_lround( float val ) {
	return static_cast< long >( mpt_round( val ) );
}

static inline std::string append_software_tag( std::string software ) {
	std::string openmpt123 = std::string() + "openmpt123 " + OPENMPT123_VERSION_STRING + " (libopenmpt " + openmpt::string::get( "library_version" ) + ", OpenMPT " + openmpt::string::get( "core_version" ) + ")";
	if ( software.empty() ) {
		software = openmpt123;
	} else {
		software += " (via " + openmpt123 + ")";
	}
	return software;
}

static inline std::string get_encoder_tag() {
	return std::string() + "openmpt123 " + OPENMPT123_VERSION_STRING + " (libopenmpt " + openmpt::string::get( "library_version" ) + ", OpenMPT " + openmpt::string::get( "core_version" ) + ")";
}

static inline std::string get_extension( std::string filename ) {
	if ( filename.find_last_of( "." ) != std::string::npos ) {
		return filename.substr( filename.find_last_of( "." ) + 1 );
	}
	return "";
}

enum class Mode {
	None,
	Probe,
	Info,
	UI,
	Batch,
	Render
};

static inline std::string mode_to_string( Mode mode ) {
	switch ( mode ) {
		case Mode::None:   return "none"; break;
		case Mode::Probe:  return "probe"; break;
		case Mode::Info:   return "info"; break;
		case Mode::UI:     return "ui"; break;
		case Mode::Batch:  return "batch"; break;
		case Mode::Render: return "render"; break;
	}
	return "";
}

static const std::int32_t default_low = -2;
static const std::int32_t default_high = -1;

struct commandlineflags {
	Mode mode;
	bool canUI;
	std::int32_t ui_redraw_interval;
	bool canProgress;
	std::string driver;
	std::string device;
	std::int32_t buffer;
	std::int32_t period;
	std::int32_t samplerate;
	std::int32_t channels;
	std::int32_t gain;
	std::int32_t separation;
	std::int32_t filtertaps;
	std::int32_t ramping; // ramping strength : -1:default 0:off 1 2 3 4 5 // roughly milliseconds
	std::int32_t tempo;
	std::int32_t pitch;
	std::int32_t dither;
	std::int32_t repeatcount;
	std::int32_t subsong;
	std::map<std::string, std::string> ctls;
	double seek_target;
	double end_time;
	bool quiet;
	bool verbose;
	int terminal_width;
	int terminal_height;
	bool show_details;
	bool show_message;
	bool show_ui;
	bool show_progress;
	bool show_meters;
	bool show_channel_meters;
	bool show_pattern;
	bool use_float;
	bool use_stdout;
	bool randomize;
	bool shuffle;
	bool restart;
	std::size_t playlist_index;
	std::vector<std::string> filenames;
	std::string output_filename;
	std::string output_extension;
	bool force_overwrite;
	bool paused;
	std::string warnings;
	void apply_default_buffer_sizes() {
		if ( ui_redraw_interval == default_high ) {
			ui_redraw_interval = 50;
		} else if ( ui_redraw_interval == default_low ) {
			ui_redraw_interval = 10;
		}
		if ( buffer == default_high ) {
			buffer = 250;
		} else if ( buffer == default_low ) {
			buffer = 50;
		}
		if ( period == default_high ) {
			period = 50;
		} else if ( period == default_low ) {
			period = 10;
		}
	}
	commandlineflags() {
		mode = Mode::UI;
		ui_redraw_interval = default_high;
		driver = "";
		device = "";
		buffer = default_high;
		period = default_high;
#if defined(__DJGPP__)
		samplerate = 44100;
		channels = 2;
		use_float = false;
#else
		samplerate = 48000;
		channels = 2;
		use_float = true;
#endif
		gain = 0;
		separation = 100;
		filtertaps = 8;
		ramping = -1;
		tempo = 0;
		pitch = 0;
		dither = 1;
		repeatcount = 0;
		subsong = -1;
		seek_target = 0.0;
		end_time = 0.0;
		quiet = false;
		verbose = false;
#if defined(__DJGPP__)
		terminal_width = 80;
		terminal_height = 25;
#else
		terminal_width = 72;
		terminal_height = 23;
#endif
#if defined(WIN32)
		terminal_width = 72;
		terminal_height = 23;
		HANDLE hStdOutput = GetStdHandle( STD_OUTPUT_HANDLE );
		if ( ( hStdOutput != NULL ) && ( hStdOutput != INVALID_HANDLE_VALUE ) ) {
			CONSOLE_SCREEN_BUFFER_INFO csbi;
			ZeroMemory( &csbi, sizeof( CONSOLE_SCREEN_BUFFER_INFO ) );
			if ( GetConsoleScreenBufferInfo( hStdOutput, &csbi ) != FALSE ) {
				terminal_width = std::min( static_cast<int>( 1 + csbi.srWindow.Right - csbi.srWindow.Left ), static_cast<int>( csbi.dwSize.X ) );
				terminal_height = std::min( static_cast<int>( 1 + csbi.srWindow.Bottom - csbi.srWindow.Top ), static_cast<int>( csbi.dwSize.Y ) );
			}
		}
#else // WIN32
		if ( isatty( STDERR_FILENO ) ) {
			if ( std::getenv( "COLUMNS" ) ) {
				std::istringstream istr( std::getenv( "COLUMNS" ) );
				int tmp = 0;
				istr >> tmp;
				if ( tmp > 0 ) {
					terminal_width = tmp;
				}
			}
			if ( std::getenv( "ROWS" ) ) {
				std::istringstream istr( std::getenv( "ROWS" ) );
				int tmp = 0;
				istr >> tmp;
				if ( tmp > 0 ) {
					terminal_height = tmp;
				}
			}
			#if defined(TIOCGWINSZ)
				struct winsize ts;
				if ( ioctl( STDERR_FILENO, TIOCGWINSZ, &ts ) >= 0 ) {
					terminal_width = ts.ws_col;
					terminal_height = ts.ws_row;
				}
			#elif defined(TIOCGSIZE)
				struct ttysize ts;
				if ( ioctl( STDERR_FILENO, TIOCGSIZE, &ts ) >= 0 ) {
					terminal_width = ts.ts_cols;
					terminal_height = ts.ts_rows;
				}
			#endif
		}
#endif
		show_details = true;
		show_message = false;
#if defined(WIN32)
		canUI = IsTerminal( 0 ) ? true : false;
		canProgress = IsTerminal( 2 ) ? true : false;
#else // !WIN32
		canUI = isatty( STDIN_FILENO ) ? true : false;
		canProgress = isatty( STDERR_FILENO ) ? true : false;
#endif // WIN32
		show_ui = canUI;
		show_progress = canProgress;
		show_meters = canUI && canProgress;
		show_channel_meters = false;
		show_pattern = false;
		use_stdout = false;
		randomize = false;
		shuffle = false;
		restart = false;
		playlist_index = 0;
		output_extension = "auto";
		force_overwrite = false;
		paused = false;
	}
	void check_and_sanitize() {
		if ( filenames.size() == 0 ) {
			throw args_error_exception();
		}
		if ( use_stdout && ( device != commandlineflags().device || !output_filename.empty() ) ) {
			throw args_error_exception();
		}
		if ( !output_filename.empty() && ( device != commandlineflags().device || use_stdout ) ) {
			throw args_error_exception();
		}
		for ( const auto & filename : filenames ) {
			if ( filename == "-" ) {
				canUI = false;
			}
		}
		show_ui = canUI;
		if ( mode == Mode::None ) {
			if ( canUI ) {
				mode = Mode::UI;
			} else {
				mode = Mode::Batch;
			}
		}
		if ( mode == Mode::UI && !canUI ) {
			throw args_error_exception();
		}
		if ( show_progress && !canProgress ) {
			throw args_error_exception();
		}
		switch ( mode ) {
			case Mode::None:
				throw args_error_exception();
			break;
			case Mode::Probe:
				show_ui = false;
				show_progress = false;
				show_meters = false;
				show_channel_meters = false;
				show_pattern = false;
			break;
			case Mode::Info:
				show_ui = false;
				show_progress = false;
				show_meters = false;
				show_channel_meters = false;
				show_pattern = false;
			break;
			case Mode::UI:
			break;
			case Mode::Batch:
				show_meters = false;
				show_channel_meters = false;
				show_pattern = false;
			break;
			case Mode::Render:
				show_meters = false;
				show_channel_meters = false;
				show_pattern = false;
				show_ui = false;
			break;
		}
		if ( quiet ) {
			verbose = false;
			show_ui = false;
			show_details = false;
			show_progress = false;
			show_channel_meters = false;
		}
		if ( verbose ) {
			show_details = true;
		}
		if ( channels != 1 && channels != 2 && channels != 4 ) {
			channels = commandlineflags().channels;
		}
		if ( samplerate < 0 ) {
			samplerate = commandlineflags().samplerate;
		}
		if ( output_extension == "auto" ) {
			output_extension = "";
		}
		if ( mode != Mode::Render && !output_extension.empty() ) {
			throw args_error_exception();
		}
		if ( mode == Mode::Render && !output_filename.empty() ) {
			throw args_error_exception();
		}
		if ( mode != Mode::Render && !output_filename.empty() ) {
			output_extension = get_extension( output_filename );
		}
		if ( output_extension.empty() ) {
			output_extension = "wav";
		}
	}
};

template < typename Tsample > Tsample convert_sample_to( float val );
template < > float convert_sample_to( float val ) {
	return val;
}
template < > std::int16_t convert_sample_to( float val ) {
	std::int32_t tmp = static_cast<std::int32_t>( val * 32768.0f );
	tmp = std::min( tmp, std::int32_t( 32767 ) );
	tmp = std::max( tmp, std::int32_t( -32768 ) );
	return static_cast<std::int16_t>( tmp );
}

class write_buffers_interface {
protected:
	virtual ~write_buffers_interface() {
		return;
	}
public:
	virtual void write_metadata( std::map<std::string,std::string> metadata ) {
		(void)metadata;
		return;
	}
	virtual void write_updated_metadata( std::map<std::string,std::string> metadata ) {
		(void)metadata;
		return;
	}
	virtual void write( const std::vector<float*> buffers, std::size_t frames ) = 0;
	virtual void write( const std::vector<std::int16_t*> buffers, std::size_t frames ) = 0;
	virtual bool pause() {
		return false;
	}
	virtual bool unpause() {
		return false;
	}
	virtual bool sleep( int /*ms*/ ) {
		return false;
	}
	virtual bool is_dummy() const {
		return false;
	}
};

class write_buffers_polling_wrapper : public write_buffers_interface {
protected:
	std::size_t channels;
	std::size_t sampleQueueMaxFrames;
	std::deque<float> sampleQueue;
protected:
	virtual ~write_buffers_polling_wrapper() {
		return;
	}
protected:
	write_buffers_polling_wrapper( const commandlineflags & flags )
		: channels(flags.channels)
		, sampleQueueMaxFrames(0)
	{
		return;
	}
	void set_queue_size_frames( std::size_t frames ) {
		sampleQueueMaxFrames = frames;
	}
	template < typename Tsample >
	Tsample pop_queue() {
		float val = 0.0f;
		if ( !sampleQueue.empty() ) {
			val = sampleQueue.front();
			sampleQueue.pop_front();
		}
		return convert_sample_to<Tsample>( val );
	}
public:
	void write( const std::vector<float*> buffers, std::size_t frames ) override {
		for ( std::size_t frame = 0; frame < frames; ++frame ) {
			for ( std::size_t channel = 0; channel < channels; ++channel ) {
				sampleQueue.push_back( buffers[channel][frame] );
			}
			while ( sampleQueue.size() >= sampleQueueMaxFrames * channels ) {
				while ( !forward_queue() ) {
					sleep( 1 );
				}
			}
		}
	}
	void write( const std::vector<std::int16_t*> buffers, std::size_t frames ) override {
		for ( std::size_t frame = 0; frame < frames; ++frame ) {
			for ( std::size_t channel = 0; channel < channels; ++channel ) {
				sampleQueue.push_back( buffers[channel][frame] * (1.0f/32768.0f) );
			}
			while ( sampleQueue.size() >= sampleQueueMaxFrames * channels ) {
				while ( !forward_queue() ) {
					sleep( 1 );
				}
			}
		}
	}
	virtual bool forward_queue() = 0;
	bool sleep( int ms ) override = 0;
};

class write_buffers_polling_wrapper_int : public write_buffers_interface {
protected:
	std::size_t channels;
	std::size_t sampleQueueMaxFrames;
	std::deque<std::int16_t> sampleQueue;
protected:
	virtual ~write_buffers_polling_wrapper_int() {
		return;
	}
protected:
	write_buffers_polling_wrapper_int( const commandlineflags & flags )
		: channels(flags.channels)
		, sampleQueueMaxFrames(0)
	{
		return;
	}
	void set_queue_size_frames( std::size_t frames ) {
		sampleQueueMaxFrames = frames;
	}
	std::int16_t pop_queue() {
		std::int16_t val = 0;
		if ( !sampleQueue.empty() ) {
			val = sampleQueue.front();
			sampleQueue.pop_front();
		}
		return val;
	}
public:
	void write( const std::vector<float*> buffers, std::size_t frames ) override {
		for ( std::size_t frame = 0; frame < frames; ++frame ) {
			for ( std::size_t channel = 0; channel < channels; ++channel ) {
				sampleQueue.push_back( convert_sample_to<std::int16_t>( buffers[channel][frame] ) );
			}
			while ( sampleQueue.size() >= sampleQueueMaxFrames * channels ) {
				while ( !forward_queue() ) {
					sleep( 1 );
				}
			}
		}
	}
	void write( const std::vector<std::int16_t*> buffers, std::size_t frames ) override {
		for ( std::size_t frame = 0; frame < frames; ++frame ) {
			for ( std::size_t channel = 0; channel < channels; ++channel ) {
				sampleQueue.push_back( buffers[channel][frame] );
			}
			while ( sampleQueue.size() >= sampleQueueMaxFrames * channels ) {
				while ( !forward_queue() ) {
					sleep( 1 );
				}
			}
		}
	}
	virtual bool forward_queue() = 0;
	bool sleep( int ms ) override = 0;
};

class void_audio_stream : public write_buffers_interface {
public:
	virtual ~void_audio_stream() {
		return;
	}
public:
	void write( const std::vector<float*> buffers, std::size_t frames ) override {
		(void)buffers;
		(void)frames;
	}
	void write( const std::vector<std::int16_t*> buffers, std::size_t frames ) override {
		(void)buffers;
		(void)frames;
	}
	bool is_dummy() const override {
		return true;
	}
};

class file_audio_stream_base : public write_buffers_interface {
protected:
	file_audio_stream_base() {
		return;
	}
public:
	void write_metadata( std::map<std::string,std::string> metadata ) override {
		(void)metadata;
		return;
	}
	void write_updated_metadata( std::map<std::string,std::string> metadata ) override {
		(void)metadata;
		return;
	}
	void write( const std::vector<float*> buffers, std::size_t frames ) override = 0;
	void write( const std::vector<std::int16_t*> buffers, std::size_t frames ) override = 0;
	virtual ~file_audio_stream_base() {
		return;
	}
};

} // namespace openmpt123

#endif // OPENMPT123_HPP
