/*
 * mptString.cpp
 * -------------
 * Purpose: Small string-related utilities, number and message formatting.
 * Notes  : Currently none.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"
#include "mptString.h"

#include "mpt/string/types.hpp"
#include "mpt/string/utility.hpp"
#include "mpt/string_transcode/transcode.hpp"

#include <locale>
#include <string>
#include <vector>

#include <cstdlib>

#if defined(MODPLUG_TRACKER)
#include <cwctype>
#endif // MODPLUG_TRACKER

#if defined(MODPLUG_TRACKER)
#include <wctype.h>
#endif // MODPLUG_TRACKER

#if MPT_OS_WINDOWS
#include <windows.h>
#endif // MPT_OS_WINDOWS


OPENMPT_NAMESPACE_BEGIN



/*



Quick guide to the OpenMPT string type jungle
=============================================



This quick guide is only meant as a hint. There may be valid reasons to not
honor the recommendations found here. Staying consistent with surrounding and/or
related code sections may also be important.



List of string types
--------------------

 *  std::string (OpenMPT, libopenmpt)
    C++ string of unspecifed 8bit encoding. Try to always document the
    encoding if not clear from context. Do not use unless there is an obvious
    reason to do so.

 *  std::wstring (OpenMPT)
    UTF16 (on windows) or UTF32 (otherwise). Do not use unless there is an
    obvious reason to do so.

 *  mpt::lstring (OpenMPT)
    OpenMPT locale string type. The encoding is always CP_ACP. Do not use unless
    there is an obvious reason to do so.

 *  char* (OpenMPT, libopenmpt)
    C string of unspecified encoding. Use only for static literals or in
    performance critical inner loops where full control and avoidance of memory
    allocations is required.

 *  wchar_t* (OpenMPT)
    C wide string. Use only if Unicode is required for static literals or in
    performance critical inner loops where full control and avoidance of memory
    allocation is required.

 *  mpt::winstring (OpenMPT)
    OpenMPT type-safe string to interface with native WinAPI, either encoded in
    locale/CP_ACP (if !UNICODE) or UTF16 (if UNICODE).

 *  CString (OpenMPT)
    MFC string type, either encoded in locale/CP_ACP (if !UNICODE) or UTF16 (if
    UNICODE). Specify literals with _T(""). Use in MFC GUI code.

 *  CStringA (OpenMPT)
    MFC ANSI string type. The encoding is always CP_ACP. Do not use.

 *  CStringW (OpenMPT)
    MFC Unicode string type. Do not use.

 *  mpt::PathString (OpenMPT, libopenmpt)
    String type representing paths and filenames. Always use for these in order
    to avoid potentially lossy conversions. Use P_("") macro for
    literals.

 *  mpt::ustring (OpenMPT, libopenmpt)
    The default unicode string type. Can be encoded in UTF8 or UTF16 or UTF32,
    depending on MPT_USTRING_MODE_* and sizeof(wchar_t). Literals can written as
    U_(""). Use as your default string type if no other string type is
    a measurably better fit.

 *  MPT_UTF8 (OpenMPT, libopenmpt)
    Macro that generates a mpt::ustring from string literals containing
    non-ascii characters. In order to keep the source code in ascii encoding,
    always express non-ascii characters using explicit \x23 escaping. Note that
    depending on the underlying type of mpt::ustring, MPT_UTF8 *requires* a
    runtime conversion. Only use for string literals containing non-ascii
    characters (use MPT_USTRING otherwise).

 *  MPT_ULITERAL / MPT_UCHAR / mpt::uchar (OpenMPT, libopenmpt)
    Macros which generate string literals, char literals and the char literal
    type respectively. These are especially useful in constexpr contexts or
    global data where MPT_USTRING is either unusable or requires a global
    contructor to run. Do NOT use as a performance optimization in place of
    MPT_USTRING however, because MPT_USTRING can be converted to C++11/14 user
    defined literals eventually, while MPT_ULITERAL cannot because of constexpr
    requirements.

 *  mpt::RawPathString (OpenMPT, libopenmpt)
    Internal representation of mpt::PathString. Only use for parsing path
    fragments.

 *  mpt::u8string (OpenMPT, libopenmpt)
    Internal representation of mpt::ustring. Do not use directly. Ever.

 *  std::basic_string<char> (OpenMPT)
    Same as std::string. Do not use std::basic_string in the templated form.

 *  std::basic_string<wchar_t> (OpenMPT)
    Same as std::wstring. Do not use std::basic_string in the templated form.

The following string types are available in order to avoid the need to overload
functions on a huge variety of string types. Use only ever as function argument
types.
Note that the locale charset is not available on all libopenmpt builds (in which
case the option is ignored or a sensible fallback is used; these types are
always available).
All these types publicly inherit from mpt::ustring and do not contain any
additional state. This means that they work the same way as mpt::ustring does
and do support type-slicing for both, read and write accesses.
These types only add conversion constructors for all string types that have a
defined encoding and for all 8bit string types using the specified encoding
heuristic.

 *  AnyUnicodeString (OpenMPT, libopenmpt)
    Is constructible from any Unicode string.

 *  AnyString (OpenMPT, libopenmpt)
    Tries to do the smartest auto-magic we can do.

 *  AnyStringLocale (OpenMPT, libopenmpt)
    char-based strings are assumed to be in locale encoding.

 *  AnyStringUTF8orLocale (OpenMPT, libopenmpt)
    char-based strings are tried in UTF8 first, if this fails, locale is used.

 *  AnyStringUTF8 (OpenMPT, libopenmpt)
    char-based strings are assumed to be in UTF8.



Encoding of 8bit strings
------------------------

8bit strings have an unspecified encoding. When the string is contained within a
CSoundFile object, the encoding is most likely CSoundFile::GetCharsetInternal(),
otherwise, try to gather the most probable encoding from surrounding or related
code sections.



Decision tree to help deciding which string type to use
-------------------------------------------------------

if in libopenmpt
 if in libopenmpt c++ interface
  T = std::string, the encoding is utf8
 elif in libopenmpt c interface
  T = char*, the encoding is utf8
 elif performance critical inner loop
  T = char*, document the encoding if not clear from context 
 elif string literal containing non-ascii characters
  T = MPT_UTF8
 elif path or file
  if parsing path fragments
   T = mpt::RawPathString
       template your function on the concrete underlying string type
       (std::string and std::wstring) or use preprocessor MPT_OS_WINDOWS
  else
   T = mpt::PathString
  fi
 else
  T = mpt::ustring
 fi
else
 if performance critical inner loop
  if needs unicode support
   T = mpt::uchar* / MPT_ULITERAL
  else
   T = char*, document the encoding if not clear from context 
  fi
 elif string literal containing non-ascii characters
  T = MPT_UTF8
 elif path or file
  if parsing path fragments
   T = mpt::RawPathString
       template your function on the concrete underlying string type
       (std::string and std::wstring) or use preprocessor MPT_OS_WINDOWS
  else
   T = mpt::PathString
  fi
 elif winapi interfacing code
  T = mpt::winstring
 elif mfc/gui code
  T = CString
 else
  if constexpr context or global data
   T = mpt::uchar* / MPT_ULITERAL
  else
   T = mpt::ustring
  fi
 fi
fi

This boils down to: Prefer mpt::PathString and mpt::ustring, and only use any
other string type if there is an obvious reason to do so.



Character set conversions
-------------------------

Character set conversions in OpenMPT are always fuzzy.

Behaviour in case of an invalid source encoding and behaviour in case of an
unrepresentable destination encoding can be any of the following:
 *  The character is replaced by some replacement character ('?' or L'\ufffd' in
    most cases).
 *  The character is replaced by a similar character (either semantically
    similiar or visually similar).
 *  The character is transcribed with some ASCII text.
 *  The character is discarded.
 *  Conversion stops at this very character.

Additionally. conversion may stop or continue on \0 characters in the middle of
the string.

Behaviour can vary from one conversion tuple to any other.

If you need to ensure lossless conversion, do a roundtrip conversion and check
for equality.



Unicode handling
----------------

OpenMPT is generally not aware of and does not handle different Unicode
normalization forms.
You should be aware of the following possibilities:
 *  Conversion between UTF8, UTF16, UTF32 may or may not change between NFC and
    NFD.
 *  Conversion from any non-Unicode 8bit encoding can result in both, NFC or NFD
    forms.
 *  Conversion to any non-Unicode 8bit encoding may or may not involve
    conversion to NFC, NFD, NFKC or NFKD during the conversion. This in
    particular means that conversion of decomposed german umlauts to ISO8859-1
    may fail.
 *  Changing the normalization form of path strings may render the file
    inaccessible.

Unicode BOM may or may not be preserved and/or discarded during conversion.

Invalid Unicode code points may be treated as invalid or as valid characters
when converting between different Unicode encodings.



Interfacing with WinAPI
-----------------------

When in MFC code, use CString.
When in non MFC code, either use std::wstring when directly interfacing with
APIs only available in WCHAR variants, or use mpt::winstring and
mpt::WinStringBuf helpers otherwise.
Specify TCHAR string literals with _T("foo") in mptrack/, and with TEXT("foo")
in common/ or sounddev/. _T() requires <tchar.h> which is specific to the MSVC
runtime and not portable across compilers. TEXT() is from <windows.h>. We use
_T() in mptrack/ only because it is shorter.



*/



namespace mpt { namespace String {



#define C(x) (mpt::char_value((x)))

// AMS1 actually only supports ASCII plus the modified control characters and no high chars at all.
// Just default to CP437 for those to keep things simple.
static constexpr char32_t CharsetTableCP437AMS[256] = {
	C(' '),0x0001,0x0002,0x0003,0x00e4,0x0005,0x00e5,0x0007,0x0008,0x0009,0x000a,0x000b,0x000c,0x000d,0x00c4,0x00c5, // differs from CP437
	0x0010,0x0011,0x0012,0x0013,0x00f6,0x0015,0x0016,0x0017,0x0018,0x00d6,0x001a,0x001b,0x001c,0x001d,0x001e,0x001f, // differs from CP437
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

// AMS2: Looking at Velvet Studio's bitmap font (TPIC32.PCX), these appear to be the only supported non-ASCII chars.
static constexpr char32_t CharsetTableCP437AMS2[256] = {
	C(' '),0x00a9,0x221a,0x00b7,C('0'),C('1'),C('2'),C('3'),C('4'),C('5'),C('6'),C('7'),C('8'),C('9'),C('A'),C('B'), // differs from CP437
	C('C'),C('D'),C('E'),C('F'),C(' '),0x00a7,C(' '),C(' '),C(' '),C(' '),C(' '),C(' '),C(' '),C(' '),C(' '),C(' '), // differs from CP437
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

#undef C


// templated on 8bit strings because of type-safe variants
template<typename Tdststring>
static Tdststring EncodeImpl(Charset charset, const mpt::widestring &src)
{
	static_assert(sizeof(typename Tdststring::value_type) == sizeof(char));
	static_assert(mpt::is_character<typename Tdststring::value_type>::value);
	switch(charset)
	{
#if defined(MPT_ENABLE_CHARSET_LOCALE)
		case Charset::Locale:           return mpt::encode<Tdststring>(mpt::logical_encoding::locale, src); break;
#endif
		case Charset::UTF8:             return mpt::encode<Tdststring>(mpt::common_encoding::utf8, src); break;
		case Charset::ASCII:            return mpt::encode<Tdststring>(mpt::common_encoding::ascii, src); break;
		case Charset::ISO8859_1:        return mpt::encode<Tdststring>(mpt::common_encoding::iso8859_1, src); break;
		case Charset::ISO8859_15:       return mpt::encode<Tdststring>(mpt::common_encoding::iso8859_15, src); break;
		case Charset::CP850:            return mpt::encode<Tdststring>(mpt::common_encoding::cp850, src); break;
		case Charset::CP437:            return mpt::encode<Tdststring>(mpt::common_encoding::cp437, src); break;
		case Charset::CP437AMS:         return mpt::encode<Tdststring>(CharsetTableCP437AMS, src); break;
		case Charset::CP437AMS2:        return mpt::encode<Tdststring>(CharsetTableCP437AMS2, src); break;
		case Charset::Windows1252:      return mpt::encode<Tdststring>(mpt::common_encoding::windows1252, src); break;
		case Charset::Amiga:            return mpt::encode<Tdststring>(mpt::common_encoding::amiga, src); break;
		case Charset::RISC_OS:          return mpt::encode<Tdststring>(mpt::common_encoding::riscos, src); break;
		case Charset::ISO8859_1_no_C1:  return mpt::encode<Tdststring>(mpt::common_encoding::iso8859_1_no_c1, src); break;
		case Charset::ISO8859_15_no_C1: return mpt::encode<Tdststring>(mpt::common_encoding::iso8859_15_no_c1, src); break;
		case Charset::Amiga_no_C1:      return mpt::encode<Tdststring>(mpt::common_encoding::amiga_no_c1, src); break;
	}
	return Tdststring();
}


// templated on 8bit strings because of type-safe variants
template<typename Tsrcstring>
static mpt::widestring DecodeImpl(Charset charset, const Tsrcstring &src)
{
	static_assert(sizeof(typename Tsrcstring::value_type) == sizeof(char));
	static_assert(mpt::is_character<typename Tsrcstring::value_type>::value);
	switch(charset)
	{
#if defined(MPT_ENABLE_CHARSET_LOCALE)
		case Charset::Locale:           return mpt::decode<Tsrcstring>(mpt::logical_encoding::locale, src); break;
#endif
		case Charset::UTF8:             return mpt::decode<Tsrcstring>(mpt::common_encoding::utf8, src); break;
		case Charset::ASCII:            return mpt::decode<Tsrcstring>(mpt::common_encoding::ascii, src); break;
		case Charset::ISO8859_1:        return mpt::decode<Tsrcstring>(mpt::common_encoding::iso8859_1, src); break;
		case Charset::ISO8859_15:       return mpt::decode<Tsrcstring>(mpt::common_encoding::iso8859_15, src); break;
		case Charset::CP850:            return mpt::decode<Tsrcstring>(mpt::common_encoding::cp850, src); break;
		case Charset::CP437:            return mpt::decode<Tsrcstring>(mpt::common_encoding::cp437, src); break;
		case Charset::CP437AMS:         return mpt::decode<Tsrcstring>(CharsetTableCP437AMS, src); break;
		case Charset::CP437AMS2:        return mpt::decode<Tsrcstring>(CharsetTableCP437AMS2, src); break;
		case Charset::Windows1252:      return mpt::decode<Tsrcstring>(mpt::common_encoding::windows1252, src); break;
		case Charset::Amiga:            return mpt::decode<Tsrcstring>(mpt::common_encoding::amiga, src); break;
		case Charset::RISC_OS:          return mpt::decode<Tsrcstring>(mpt::common_encoding::riscos, src); break;
		case Charset::ISO8859_1_no_C1:  return mpt::decode<Tsrcstring>(mpt::common_encoding::iso8859_1_no_c1, src); break;
		case Charset::ISO8859_15_no_C1: return mpt::decode<Tsrcstring>(mpt::common_encoding::iso8859_15_no_c1, src); break;
		case Charset::Amiga_no_C1:      return mpt::decode<Tsrcstring>(mpt::common_encoding::amiga_no_c1, src); break;
	}
	return mpt::widestring();
}


// templated on 8bit strings because of type-safe variants
template<typename Tdststring, typename Tsrcstring>
static Tdststring ConvertImpl(Charset to, Charset from, const Tsrcstring &src)
{
	static_assert(sizeof(typename Tdststring::value_type) == sizeof(char));
	static_assert(sizeof(typename Tsrcstring::value_type) == sizeof(char));
	if(to == from)
	{
		const typename Tsrcstring::value_type * src_beg = src.data();
		const typename Tsrcstring::value_type * src_end = src_beg + src.size();
		return Tdststring(reinterpret_cast<const typename Tdststring::value_type *>(src_beg), reinterpret_cast<const typename Tdststring::value_type *>(src_end));
	}
	return EncodeImpl<Tdststring>(to, DecodeImpl(from, src));
}



} // namespace String


bool IsUTF8(const std::string &str)
{
	return mpt::is_utf8(str);
}


#if MPT_WSTRING_CONVERT
std::wstring ToWide(Charset from, const std::string &str)
{
	return String::DecodeImpl(from, str);
}
#if defined(MPT_ENABLE_CHARSET_LOCALE)
std::wstring ToWide(const mpt::lstring &str)
{
	return String::DecodeImpl(Charset::Locale, str);
}
#endif // MPT_ENABLE_CHARSET_LOCALE
#endif

#if MPT_WSTRING_CONVERT
std::string ToCharset(Charset to, const std::wstring &str)
{
	return String::EncodeImpl<std::string>(to, str);
}
#endif
std::string ToCharset(Charset to, Charset from, const std::string &str)
{
	return String::ConvertImpl<std::string>(to, from, str);
}
#if defined(MPT_ENABLE_CHARSET_LOCALE)
std::string ToCharset(Charset to, const mpt::lstring &str)
{
	return String::ConvertImpl<std::string>(to, Charset::Locale, str);
}
#endif // MPT_ENABLE_CHARSET_LOCALE

#if defined(MPT_ENABLE_CHARSET_LOCALE)
#if MPT_WSTRING_CONVERT
mpt::lstring ToLocale(const std::wstring &str)
{
	return String::EncodeImpl<mpt::lstring>(Charset::Locale, str);
}
#endif
mpt::lstring ToLocale(Charset from, const std::string &str)
{
	return String::ConvertImpl<mpt::lstring>(Charset::Locale, from, str);
}
#endif // MPT_ENABLE_CHARSET_LOCALE

#if MPT_OS_WINDOWS
#if MPT_WSTRING_CONVERT
mpt::winstring ToWin(const std::wstring &str)
{
	#ifdef UNICODE
		return str;
	#else
		return ToLocale(str);
	#endif
}
#endif
mpt::winstring ToWin(Charset from, const std::string &str)
{
	#ifdef UNICODE
		return ToWide(from, str);
	#else
		return ToLocale(from, str);
	#endif
}
#if defined(MPT_ENABLE_CHARSET_LOCALE)
mpt::winstring ToWin(const mpt::lstring &str)
{
	#ifdef UNICODE
		return ToWide(str);
	#else
		return str;
	#endif
}
#endif // MPT_ENABLE_CHARSET_LOCALE
#endif // MPT_OS_WINDOWS


#if defined(MPT_WITH_MFC)

CString ToCString(const std::wstring &str)
{
	#ifdef UNICODE
		return str.c_str();
	#else
		return ToCharset(Charset::Locale, str).c_str();
	#endif
}
CString ToCString(Charset from, const std::string &str)
{
	#ifdef UNICODE
		return ToWide(from, str).c_str();
	#else
		return ToCharset(Charset::Locale, from, str).c_str();
	#endif
}
std::wstring ToWide(const CString &str)
{
	#ifdef UNICODE
		return str.GetString();
	#else
		return ToWide(Charset::Locale, str.GetString());
	#endif
}
std::string ToCharset(Charset to, const CString &str)
{
	#ifdef UNICODE
		return ToCharset(to, str.GetString());
	#else
		return ToCharset(to, Charset::Locale, str.GetString());
	#endif
}
#if defined(MPT_ENABLE_CHARSET_LOCALE)
CString ToCString(const mpt::lstring &str)
{
	#ifdef UNICODE
		return ToWide(str).c_str();
	#else
		return str.c_str();
	#endif
}
mpt::lstring ToLocale(const CString &str)
{
	#ifdef UNICODE
		return String::EncodeImpl<mpt::lstring>(Charset::Locale, str.GetString());
	#else
		return str.GetString();
	#endif
}
#endif // MPT_ENABLE_CHARSET_LOCALE
#if MPT_OS_WINDOWS
mpt::winstring ToWin(const CString &str)
{
	return str.GetString();
}
#endif // MPT_OS_WINDOWS

#endif // MPT_WITH_MFC


#if MPT_USTRING_MODE_WIDE
// inline
#else // !MPT_USTRING_MODE_WIDE
#if MPT_WSTRING_CONVERT
mpt::ustring ToUnicode(const std::wstring &str)
{
	return String::EncodeImpl<mpt::ustring>(mpt::Charset::UTF8, str);
}
#endif
mpt::ustring ToUnicode(Charset from, const std::string &str)
{
	return String::ConvertImpl<mpt::ustring>(mpt::Charset::UTF8, from, str);
}
#if defined(MPT_ENABLE_CHARSET_LOCALE)
mpt::ustring ToUnicode(const mpt::lstring &str)
{
	return String::ConvertImpl<mpt::ustring>(mpt::Charset::UTF8, mpt::Charset::Locale, str);
}
#endif // MPT_ENABLE_CHARSET_LOCALE
#if defined(MPT_WITH_MFC)
mpt::ustring ToUnicode(const CString &str)
{
	#ifdef UNICODE
		return String::EncodeImpl<mpt::ustring>(mpt::Charset::UTF8, str.GetString());
	#else // !UNICODE
		return String::ConvertImpl<mpt::ustring, std::string>(mpt::Charset::UTF8, mpt::Charset::Locale, str.GetString());
	#endif // UNICODE
}
#endif // MPT_WITH_MFC
#endif // MPT_USTRING_MODE_WIDE

#if MPT_USTRING_MODE_WIDE
// nothing, std::wstring overloads will catch all stuff
#else // !MPT_USTRING_MODE_WIDE
#if MPT_WSTRING_CONVERT
std::wstring ToWide(const mpt::ustring &str)
{
	return String::DecodeImpl<mpt::ustring>(mpt::Charset::UTF8, str);
}
#endif
std::string ToCharset(Charset to, const mpt::ustring &str)
{
	return String::ConvertImpl<std::string, mpt::ustring>(to, mpt::Charset::UTF8, str);
}
#if defined(MPT_ENABLE_CHARSET_LOCALE)
mpt::lstring ToLocale(const mpt::ustring &str)
{
	return String::ConvertImpl<mpt::lstring, mpt::ustring>(mpt::Charset::Locale, mpt::Charset::UTF8, str);
}
#endif // MPT_ENABLE_CHARSET_LOCALE
#if MPT_OS_WINDOWS
mpt::winstring ToWin(const mpt::ustring &str)
{
	#ifdef UNICODE
		return String::DecodeImpl<mpt::ustring>(mpt::Charset::UTF8, str);
	#else
		return String::ConvertImpl<mpt::lstring, mpt::ustring>(mpt::Charset::Locale, mpt::Charset::UTF8, str);
	#endif
}
#endif // MPT_OS_WINDOWS
#if defined(MPT_WITH_MFC)
CString ToCString(const mpt::ustring &str)
{
	#ifdef UNICODE
		return String::DecodeImpl<mpt::ustring>(mpt::Charset::UTF8, str).c_str();
	#else // !UNICODE
		return String::ConvertImpl<std::string, mpt::ustring>(mpt::Charset::Locale, mpt::Charset::UTF8, str).c_str();
	#endif // UNICODE
}
#endif // MPT_WITH_MFC
#endif // MPT_USTRING_MODE_WIDE





static mpt::Charset CharsetFromCodePage(uint16 codepage, mpt::Charset fallback, bool * isFallback = nullptr)
{
	mpt::Charset result = fallback;
	switch(codepage)
	{
	case 65001:
		result = mpt::Charset::UTF8;
		if(isFallback) *isFallback = false;
		break;
	case 20127:
		result = mpt::Charset::ASCII;
		if(isFallback) *isFallback = false;
		break;
	case 28591:
		result = mpt::Charset::ISO8859_1;
		if(isFallback) *isFallback = false;
		break;
	case 28605:
		result = mpt::Charset::ISO8859_15;
		if(isFallback) *isFallback = false;
		break;
	case 437:
		result = mpt::Charset::CP437;
		if(isFallback) *isFallback = false;
		break;
	case 1252:
		result = mpt::Charset::Windows1252;
		if(isFallback) *isFallback = false;
		break;
	default:
		result = fallback;
		if(isFallback) *isFallback = true;
		break;
	}
	return result;
}

mpt::ustring ToUnicode(uint16 codepage, mpt::Charset fallback, const std::string &str)
{
	#if MPT_OS_WINDOWS
		mpt::ustring result;
		bool noCharsetMatch = true;
		mpt::Charset charset = mpt::CharsetFromCodePage(codepage, fallback, &noCharsetMatch);
		if(noCharsetMatch && mpt::has_codepage(codepage))
		{
			result = mpt::ToUnicode(mpt::decode<std::string>(codepage, str));
		} else
		{
			result = mpt::ToUnicode(charset, str);
		}
		return result;
	#else // !MPT_OS_WINDOWS
		return mpt::ToUnicode(mpt::CharsetFromCodePage(codepage, fallback), str);
	#endif // MPT_OS_WINDOWS
}





char ToLowerCaseAscii(char c)
{
	return mpt::to_lower_ascii(c);
}

char ToUpperCaseAscii(char c)
{
	return mpt::to_upper_ascii(c);
}

std::string ToLowerCaseAscii(std::string s)
{
	std::transform(s.begin(), s.end(), s.begin(), static_cast<char(*)(char)>(&mpt::ToLowerCaseAscii));
	return s;
}

std::string ToUpperCaseAscii(std::string s)
{
	std::transform(s.begin(), s.end(), s.begin(), static_cast<char(*)(char)>(&mpt::ToUpperCaseAscii));
	return s;
}

int CompareNoCaseAscii(const char *a, const char *b, std::size_t n)
{
	while(n--)
	{
		unsigned char ac = mpt::char_value(mpt::ToLowerCaseAscii(*a));
		unsigned char bc = mpt::char_value(mpt::ToLowerCaseAscii(*b));
		if(ac != bc)
		{
			return ac < bc ? -1 : 1;
		} else if(!ac && !bc)
		{
			return 0;
		}
		++a;
		++b;
	}
	return 0;
}

int CompareNoCaseAscii(std::string_view a, std::string_view b)
{
	for(std::size_t i = 0; i < std::min(a.length(), b.length()); ++i)
	{
		unsigned char ac = mpt::char_value(mpt::ToLowerCaseAscii(a[i]));
		unsigned char bc = mpt::char_value(mpt::ToLowerCaseAscii(b[i]));
		if(ac != bc)
		{
			return ac < bc ? -1 : 1;
		} else if(!ac && !bc)
		{
			return 0;
		}
	}
	if(a.length() == b.length())
	{
		return 0;
	}
	return a.length() < b.length() ? -1 : 1;
}

int CompareNoCaseAscii(const std::string &a, const std::string &b)
{
	return CompareNoCaseAscii(std::string_view(a), std::string_view(b));
}


#if defined(MODPLUG_TRACKER)

mpt::ustring ToLowerCase(const mpt::ustring &s)
{
	#if defined(MPT_WITH_MFC)
		#if defined(UNICODE)
			CString tmp = mpt::ToCString(s);
			tmp.MakeLower();
			return mpt::ToUnicode(tmp);
		#else // !UNICODE
			CStringW tmp = mpt::ToWide(s).c_str();
			tmp.MakeLower();
			return mpt::ToUnicode(tmp.GetString());
		#endif // UNICODE
	#else // !MPT_WITH_MFC
		std::wstring ws = mpt::ToWide(s);
		std::transform(ws.begin(), ws.end(), ws.begin(), &std::towlower);
		return mpt::ToUnicode(ws);
	#endif // MPT_WITH_MFC
}

mpt::ustring ToUpperCase(const mpt::ustring &s)
{
	#if defined(MPT_WITH_MFC)
		#if defined(UNICODE)
			CString tmp = mpt::ToCString(s);
			tmp.MakeUpper();
			return mpt::ToUnicode(tmp);
		#else // !UNICODE
			CStringW tmp = mpt::ToWide(s).c_str();
			tmp.MakeUpper();
			return mpt::ToUnicode(tmp.GetString());
		#endif // UNICODE
	#else // !MPT_WITH_MFC
		std::wstring ws = mpt::ToWide(s);
		std::transform(ws.begin(), ws.end(), ws.begin(), &std::towlower);
		return mpt::ToUnicode(ws);
	#endif // MPT_WITH_MFC
}

#endif // MODPLUG_TRACKER



} // namespace mpt



OPENMPT_NAMESPACE_END
