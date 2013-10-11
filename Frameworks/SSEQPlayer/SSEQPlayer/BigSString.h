/*
 * String class in ANSI (or rather, current Windows code page), UTF-8, and UTF-16
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2013-03-25
 */

#ifndef BIGSSTRING_H
#define BIGSSTRING_H

#include <string>
#include <cstring>
#include <cwchar>
#include "UtfConverter.h"
#include "UTFEncodeDecode.h"

class String
{
public:
	String(const std::locale &L = std::locale::classic()) : ansi(""), utf8(""), utf16(L""), loc(L) { }
	String(const char *new_str, bool is_utf8 = false, const std::locale &L = std::locale::classic()) : ansi(is_utf8 ? DecodeFromUTF8(new_str, L) : new_str), utf8(is_utf8 ? new_str : EncodeToUTF8(new_str, L)),
		utf16(UtfConverter::FromUtf8(utf8)), loc(L) { }
	String(const std::string &new_str, bool is_utf8 = false, const std::locale &L = std::locale::classic()) : ansi(is_utf8 ? DecodeFromUTF8(new_str, L) : new_str), utf8(is_utf8 ? new_str : EncodeToUTF8(new_str, L)),
		utf16(UtfConverter::FromUtf8(utf8)), loc(L) { }
	String(const wchar_t *new_wstr, const std::locale &L = std::locale::classic()) : ansi(DecodeFromUTF8(UtfConverter::ToUtf8(new_wstr), L)), utf8(UtfConverter::ToUtf8(new_wstr)), utf16(new_wstr), loc(L) { }
	String(const std::wstring &new_wstr, const std::locale &L = std::locale::classic()) : ansi(DecodeFromUTF8(UtfConverter::ToUtf8(new_wstr), L)), utf8(UtfConverter::ToUtf8(new_wstr)), utf16(new_wstr), loc(L) { }
	bool empty() const { return this->utf8.empty(); }
	std::string GetAnsi() const { return this->ansi; }
	const char *GetAnsiC() const { return this->ansi.c_str(); }
	std::string GetStr() const { return this->utf8; }
	const char *GetStrC() const { return this->utf8.c_str(); }
	std::wstring GetWStr() const { return this->utf16; }
	const wchar_t *GetWStrC() const { return this->utf16.c_str(); }
	bool operator==(const String &str2) const
	{
		return this->utf8 == str2.utf8;
	}
	String &operator=(const std::string &new_str)
	{
		this->ansi = DecodeFromUTF8(new_str, this->loc);
		this->utf8 = new_str;
		this->utf16 = UtfConverter::FromUtf8(new_str);
		return *this;
	}
	String &operator=(const std::wstring &new_wstr)
	{
		this->utf8 = UtfConverter::ToUtf8(new_wstr);
		this->ansi = DecodeFromUTF8(this->utf8, this->loc);
		this->utf16 = new_wstr;
		return *this;
	}
	String &operator=(const String &new_string)
	{
		if (this != &new_string)
		{
			this->ansi = new_string.ansi;
			this->utf8 = new_string.utf8;
			this->utf16 = new_string.utf16;
		}
		return *this;
	}
	String &SetISO8859_1(const std::string &new_str)
	{
		this->ansi = new_str;
		this->utf8 = EncodeToUTF8(new_str, this->loc);
		this->utf16 = UtfConverter::FromUtf8(this->utf8);
		return *this;
	}
	String operator+(const String &str2) const
	{
		String new_string = String(*this);
		new_string.ansi.append(str2.ansi);
		new_string.utf8.append(str2.utf8);
		new_string.utf16.append(str2.utf16);
		return new_string;
	}
	String operator+(char chr) const
	{
		String new_string = String(*this);
		char str2[] = { chr, 0 };
		new_string.ansi.append(str2);
		std::string new_utf8 = EncodeToUTF8(str2, this->loc);
		new_string.utf8.append(new_utf8);
		new_string.utf16.append(UtfConverter::FromUtf8(new_utf8));
		return new_string;
	}
	String operator+(wchar_t wchr) const
	{
		String new_string = String(*this);
		wchar_t wstr2[] = { wchr, 0 };
		std::string new_utf8 = UtfConverter::ToUtf8(wstr2);
		new_string.ansi.append(DecodeFromUTF8(new_utf8, this->loc));
		new_string.utf8.append(new_utf8);
		new_string.utf16.append(wstr2);
		return new_string;
	}
	String operator+(const char *str2) const
	{
		String new_string = String(*this);
		new_string.ansi.append(DecodeFromUTF8(str2, this->loc));
		new_string.utf8.append(str2);
		new_string.utf16.append(UtfConverter::FromUtf8(str2));
		return new_string;
	}
	String operator+(const std::string &str2) const
	{
		String new_string = String(*this);
		new_string.ansi.append(DecodeFromUTF8(str2, this->loc));
		new_string.utf8.append(str2);
		new_string.utf16.append(UtfConverter::FromUtf8(str2));
		return new_string;
	}
	String operator+(const wchar_t *wstr2) const
	{
		String new_string = String(*this);
		std::string new_utf8 = UtfConverter::ToUtf8(wstr2);
		new_string.ansi.append(DecodeFromUTF8(new_utf8, this->loc));
		new_string.utf8.append(new_utf8);
		new_string.utf16.append(wstr2);
		return new_string;
	}
	String operator+(const std::wstring &wstr2) const
	{
		String new_string = String(*this);
		std::string new_utf8 = UtfConverter::ToUtf8(wstr2);
		new_string.ansi.append(DecodeFromUTF8(new_utf8, this->loc));
		new_string.utf8.append(new_utf8);
		new_string.utf16.append(wstr2);
		return new_string;
	}
	String &operator+=(const String &str2)
	{
		if (this != &str2)
		{
			this->ansi.append(str2.ansi);
			this->utf8.append(str2.utf8);
			this->utf16.append(str2.utf16);
		}
		return *this;
	}
	String &operator+=(char chr)
	{
		char str2[] = { chr, 0 };
		this->ansi.append(str2);
		std::string new_utf8 = EncodeToUTF8(str2, this->loc);
		this->utf8.append(new_utf8);
		this->utf16.append(UtfConverter::FromUtf8(new_utf8));
		return *this;
	}
	String &operator+=(wchar_t wchr)
	{
		wchar_t wstr2[] = { wchr, 0 };
		std::string new_utf8 = UtfConverter::ToUtf8(wstr2);
		this->ansi.append(DecodeFromUTF8(new_utf8, this->loc));
		this->utf8.append(new_utf8);
		this->utf16.append(wstr2);
		return *this;
	}
	String &operator+=(const char *str2)
	{
		this->ansi.append(DecodeFromUTF8(str2, this->loc));
		this->utf8.append(str2);
		this->utf16.append(UtfConverter::FromUtf8(str2));
		return *this;
	}
	String &operator+=(const std::string &str2)
	{
		this->ansi.append(DecodeFromUTF8(str2, this->loc));
		this->utf8.append(str2);
		this->utf16.append(UtfConverter::FromUtf8(str2));
		return *this;
	}
	String &operator+=(const wchar_t *wstr2)
	{
		std::string new_utf8 = UtfConverter::ToUtf8(wstr2);
		this->ansi.append(DecodeFromUTF8(new_utf8, this->loc));
		this->utf8.append(new_utf8);
		this->utf16.append(wstr2);
		return *this;
	}
	String &operator+=(const std::wstring &wstr2)
	{
		std::string new_utf8 = UtfConverter::ToUtf8(wstr2);
		this->ansi.append(DecodeFromUTF8(new_utf8, this->loc));
		this->utf8.append(new_utf8);
		this->utf16.append(wstr2);
		return *this;
	}
	String &AppendISO8859_1(const std::string &str2)
	{
		this->ansi.append(str2);
		std::string new_utf8 = EncodeToUTF8(str2, this->loc);
		this->utf8.append(new_utf8);
		this->utf16.append(UtfConverter::FromUtf8(new_utf8));
		return *this;
	}
	String Substring(size_t pos = 0, size_t n = std::string::npos) const
	{
		String new_string = String(*this);
		new_string.ansi.substr(pos, n);
		new_string.utf8.substr(pos, n);
		if (n == std::string::npos)
			n = std::wstring::npos;
		new_string.utf16.substr(pos, n);
		return new_string;
	}
	void CopyToString(wchar_t *str, bool = false) const
	{
		wcscpy(str, utf16.c_str());
	}
	void CopyToString(char *str, bool use_utf8 = false) const
	{
		strcpy(str, (use_utf8 ? utf8 : ansi).c_str());
	}
protected:
	std::string ansi, utf8;
	std::wstring utf16;
	std::locale loc;
};

#endif
