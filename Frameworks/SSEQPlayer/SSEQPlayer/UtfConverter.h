// original code is from here: http://www.codeproject.com/Articles/17573/Convert-Between-std-string-and-std-wstring-UTF-8-a
// License: The Code Project Open License (CPOL) http://www.codeproject.com/info/cpol10.aspx

#ifndef UTFCONVERTER_H
#define UTFCONVERTER_H

#include <string>

namespace UtfConverter
{
	std::wstring FromUtf8(const std::string &);
	std::string ToUtf8(const std::wstring &);
}

#endif
