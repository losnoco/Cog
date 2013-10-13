// original code is from here: http://www.codeproject.com/Articles/17573/Convert-Between-std-string-and-std-wstring-UTF-8-a
// License: The Code Project Open License (CPOL) http://www.codeproject.com/info/cpol10.aspx

#include <string>
#include <vector>
#include <stdexcept>
#include "ConvertUTF.h"

namespace UtfConverter
{
	std::wstring FromUtf8(const std::string &utf8string)
	{
		auto widesize = utf8string.length();
		auto result = std::vector<wchar_t>(widesize + 1, L'\0');
		auto orig = std::vector<char>(widesize + 1, '\0');
		std::copy(utf8string.begin(), utf8string.end(), orig.begin());
		auto *sourcestart = reinterpret_cast<const UTF8 *>(&orig[0]), *sourceend = sourcestart + widesize;
		ConversionResult res;
		if (sizeof(wchar_t) == 2)
		{
			auto *targetstart = reinterpret_cast<UTF16 *>(&result[0]), *targetend = targetstart + widesize;
			res = ConvertUTF8toUTF16(&sourcestart, sourceend, &targetstart, targetend, strictConversion);
			*targetstart = 0;
			unsigned end = targetstart - reinterpret_cast<UTF16 *>(&result[0]);
			result.erase(result.begin() + end, result.end());
		}
		else if (sizeof(wchar_t) == 4)
		{
			auto *targetstart = reinterpret_cast<UTF32 *>(&result[0]), *targetend = targetstart + widesize;
			res = ConvertUTF8toUTF32(&sourcestart, sourceend, &targetstart, targetend, strictConversion);
			*targetstart = 0;
			unsigned long end = targetstart - reinterpret_cast<UTF32 *>(&result[0]);
			result.erase(result.begin() + end, result.end());
		}
		else
			throw std::runtime_error("UtfConverter::FromUtf8: sizeof(wchar_t) is not 2 or 4.");
		if (res != conversionOK)
			throw std::runtime_error("UtfConverter::FromUtf8: Conversion failed.");
		return std::wstring(result.begin(), result.end());
	}

	std::string ToUtf8(const std::wstring &widestring)
	{
		auto widesize = widestring.length(), utf8size = (sizeof(wchar_t) == 2 ? 3 : 4) * widesize + 1;
		auto result = std::vector<char>(utf8size, '\0');
		auto orig = std::vector<wchar_t>(widesize + 1, L'\0');
		std::copy(widestring.begin(), widestring.end(), orig.begin());
		auto *targetstart = reinterpret_cast<UTF8 *>(&result[0]), *targetend = targetstart + utf8size;
		ConversionResult res;
		if (sizeof(wchar_t) == 2)
		{
			auto *sourcestart = reinterpret_cast<const UTF16 *>(&orig[0]), *sourceend = sourcestart + widesize;
			res = ConvertUTF16toUTF8(&sourcestart, sourceend, &targetstart, targetend, strictConversion);
		}
		else if (sizeof(wchar_t) == 4)
		{
			auto *sourcestart = reinterpret_cast<const UTF32 *>(&orig[0]), *sourceend = sourcestart + widesize;
			res = ConvertUTF32toUTF8(&sourcestart, sourceend, &targetstart, targetend, strictConversion);
		}
		else
			throw std::runtime_error("UtfConverter::ToUtf8: sizeof(wchar_t) is not 2 or 4.");
		if (res != conversionOK)
			throw std::runtime_error("UtfConverter::ToUtf8: Conversion failed.");
		*targetstart = 0;
		auto end = targetstart - reinterpret_cast<UTF8 *>(&result[0]);
		result.erase(result.begin() + end, result.end());
		return std::string(result.begin(), result.end());
	}
}
