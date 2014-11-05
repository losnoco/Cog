/*
 * Common conversion functions
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2014-09-24
 */

#pragma once

#include <stdexcept>
#include <string>
#include <sstream>
#include <typeinfo>
#include <locale>
#if (defined(__GNUC__) || defined(__clang__)) && !defined(_LIBCPP_VERSION)
# include "wstring_convert.h"
# include "codecvt.h"
#else
# include <codecvt>
#endif
#include <vector>
#include <cmath>

/*
 * The following exception class and the *stringify and convert* functions are
 * from the C++ FAQ, section 39, entry 3:
 * http://www.parashift.com/c++-faq/convert-string-to-any.html
 *
 * The convert and convertTo functions were made into templates of std::basic_string
 * to handle wide-character strings properly, as well as adding wstringify for the
 * same reason.
 */
class BadConversion : public std::runtime_error
{
public:
	BadConversion(const std::string &s) : std::runtime_error(s) { }
};

template<typename T> inline std::string stringify(const T &x)
{
	std::ostringstream o;
	if (!(o << x))
		throw BadConversion(std::string("stringify(") + typeid(x).name() + ")");
	return o.str();
}

template<typename T> inline std::wstring wstringify(const T &x)
{
	std::wostringstream o;
	if (!(o << x))
		throw BadConversion(std::string("wstringify(") + typeid(x).name() + ")");
	return o.str();
}

template<typename T, typename S> inline void convert(const std::basic_string<S> &s, T &x, bool failIfLeftoverChars = true)
{
	std::basic_istringstream<S> i(s);
	S c;
	if (!(i >> x) || (failIfLeftoverChars && i.get(c)))
		throw BadConversion(std::string("convert(") + typeid(S).name() + ")");
}

template<typename T, typename S> inline T convertTo(const std::basic_string<S> &s, bool failIfLeftoverChars = true)
{
	T x;
	convert(s, x, failIfLeftoverChars);
	return x;
}

// Miscellaneous conversion functions
class ConvertFuncs
{
private:
	static inline bool IsDigitsOnly(const std::string &input, const std::locale &loc = std::locale::classic())
	{
		auto inputChars = std::vector<char>(input.begin(), input.end());
		size_t length = inputChars.size();
		auto masks = std::vector<std::ctype<char>::mask>(length);
		std::use_facet<std::ctype<char>>(loc).is(&inputChars[0], &inputChars[length], &masks[0]);
		for (size_t x = 0; x < length; ++x)
			if (inputChars[x] != '.' && !(masks[x] & std::ctype<char>::digit))
				return false;
		return true;
	}

public:
	static unsigned long StringToMS(const std::string &time)
	{
		unsigned long hours = 0, minutes = 0;
		double seconds = 0.0;
		std::string hoursStr, minutesStr, secondsStr;
		size_t firstcolon = time.find(':');
		if (firstcolon != std::string::npos)
		{
			size_t secondcolon = time.substr(firstcolon + 1).find(':');
			if (secondcolon != std::string::npos)
			{
				secondcolon = firstcolon + secondcolon + 1;
				hoursStr = time.substr(0, firstcolon);
				minutesStr = time.substr(firstcolon + 1, secondcolon - firstcolon - 1);
				secondsStr = time.substr(secondcolon + 1);
			}
			else
			{
				minutesStr = time.substr(0, firstcolon);
				secondsStr = time.substr(firstcolon + 1);
			}
		}
		else
			secondsStr = time;
		if (!hoursStr.empty())
		{
			if (!ConvertFuncs::IsDigitsOnly(hoursStr))
				return 0;
			hours = convertTo<unsigned long>(hoursStr, false);
		}
		if (!minutesStr.empty())
		{
			if (!ConvertFuncs::IsDigitsOnly(minutesStr))
				return 0;
			minutes = convertTo<unsigned long>(minutesStr, false);
		}
		if (!secondsStr.empty())
		{
			if (!ConvertFuncs::IsDigitsOnly(secondsStr))
				return 0;
			size_t comma = secondsStr.find(',');
			if (comma != std::string::npos)
				secondsStr[comma] = '.';
			seconds = convertTo<double>(secondsStr, false);
		}
		seconds += minutes * 60 + hours * 1440;
		return static_cast<unsigned long>(std::floor(seconds * 1000 + 0.5));
	}

	static unsigned long StringToMS(const std::wstring &time)
	{
		return ConvertFuncs::StringToMS(ConvertFuncs::WStringToString(time));
	}

	static std::string MSToString(unsigned long time)
	{
		double seconds = time / 1000.0;
		if (seconds < 60)
			return stringify(seconds);
		unsigned long minutes = static_cast<unsigned long>(seconds) / 60;
		seconds -= minutes * 60;
		if (minutes < 60)
			return stringify(minutes) + ":" + (seconds < 10 ? "0" : "") + stringify(seconds);
		unsigned long hours = minutes / 60;
		minutes %= 60;
		return stringify(hours) + ":" + (minutes < 10 ? "0" : "") + stringify(minutes) + ":" + (seconds < 10 ? "0" : "") + stringify(seconds);
	}

	static std::wstring MSToWString(unsigned long time)
	{
		return ConvertFuncs::StringToWString(ConvertFuncs::MSToString(time));
	}

	static std::wstring StringToWString(const std::string &str)
	{
		static std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
		return conv.from_bytes(str);
	}

	static std::string WStringToString(const std::wstring &wstr)
	{
		static std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
		return conv.to_bytes(wstr);
	}
};
