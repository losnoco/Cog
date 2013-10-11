// original code is from here: http://www.codeproject.com/Tips/197097/Converting-ANSI-to-Unicode-and-back
// License: The Code Project Open License (CPOL) http://www.codeproject.com/info/cpol10.aspx

#ifndef UTFENCODEDECODE_H
#define UTFENCODEDECODE_H

#include <string>
#include <locale>
#include <sstream>
#include <stdexcept>

std::string EncodeToUTF8(const std::string &, const std::locale & = std::locale::classic());
std::string DecodeFromUTF8(const std::string &, const std::locale & = std::locale::classic());

template<size_t buf_size = 100> class cp_converter
{
	const std::locale loc;
public:
	cp_converter(const std::locale &L = std::locale::classic()) : loc(L) { }
	inline std::wstring widen(const std::string &in)
	{
		return this->convert<char, wchar_t>(in);
	}
	inline std::string narrow(const std::wstring &in)
	{
		return this->convert<wchar_t, char>(in);
	}
private:
	typedef std::codecvt<wchar_t, char, mbstate_t> codecvt_facet;

	// widen
	inline codecvt_facet::result cv(const codecvt_facet &facet, mbstate_t &s, const char *f1, const char *l1, const char *&n1, wchar_t *f2, wchar_t *l2, wchar_t *&n2) const
	{
		return facet.in(s, f1, l1, n1, f2, l2, n2);
	}

	// narrow
	inline codecvt_facet::result cv(const codecvt_facet &facet, mbstate_t &s, const wchar_t *f1, const wchar_t *l1, const wchar_t *&n1, char *f2, char *l2, char *&n2) const
	{
		return facet.out(s, f1, l1, n1, f2, l2, n2);
	}

	template<class ct_in, class ct_out> std::basic_string<ct_out> convert(const std::basic_string<ct_in> &in)
	{
		auto &facet = std::use_facet<codecvt_facet>(this->loc);
		std::basic_stringstream<ct_out> os;
		ct_out buf[buf_size];
		mbstate_t state = {0};
		codecvt_facet::result result;
		const ct_in *ipc = &in[0];
		do
		{
			ct_out *opc = nullptr;
			result = this->cv(facet, state, ipc, &in[0] + in.length(), ipc, buf, buf + buf_size, opc);
			os << std::basic_string<ct_out>(buf, opc - buf);
		} while (ipc < &in[0] + in.length() && result != codecvt_facet::error);
		if (result != codecvt_facet::ok)
			throw std::runtime_error("result is not ok!");
		return os.str();
	}
};

#endif
