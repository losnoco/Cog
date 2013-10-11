// original code is from here: http://www.codeproject.com/Tips/197097/Converting-ANSI-to-Unicode-and-back
// License: The Code Project Open License (CPOL) http://www.codeproject.com/info/cpol10.aspx

#include "UTFEncodeDecode.h"
#include "UtfConverter.h"

std::string EncodeToUTF8(const std::string &source, const std::locale &L)
{
	try
	{
		return UtfConverter::ToUtf8(cp_converter<>(L).widen(source));
	}
	catch (const std::runtime_error &)
	{
		return "";
	}
}

std::string DecodeFromUTF8(const std::string &source, const std::locale &L)
{
	try
	{
		return cp_converter<>(L).narrow(UtfConverter::FromUtf8(source));
	}
	catch (const std::runtime_error &)
	{
		return "";
	}
}
