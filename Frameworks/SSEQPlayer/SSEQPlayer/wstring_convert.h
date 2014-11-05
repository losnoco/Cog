// This comes from llvm's libcxx project. I've copied the code from there (with very minor modifications) for use with GCC and Clang when libcxx isn't being used.

#if (defined(__GNUC__) || defined(__clang__)) && !defined(_LIBCPP_VERSION)
#pragma once

#include <memory>
#include <string>
#include <locale>

namespace std
{

template<class _Codecvt, class _Elem = wchar_t, class _Wide_alloc = allocator<_Elem>, class _Byte_alloc = allocator<char>> class wstring_convert
{
public:
	typedef basic_string<char, char_traits<char>, _Byte_alloc> byte_string;
	typedef basic_string<_Elem, char_traits<_Elem>, _Wide_alloc> wide_string;
	typedef typename _Codecvt::state_type state_type;
	typedef typename wide_string::traits_type::int_type int_type;

private:
	byte_string __byte_err_string_;
	wide_string __wide_err_string_;
	_Codecvt *__cvtptr_;
	state_type __cvtstate_;
	size_t __cvtcount_;

	wstring_convert(const wstring_convert &__wc);
	wstring_convert& operator=(const wstring_convert &__wc);
public:
	wstring_convert(_Codecvt *__pcvt = new _Codecvt) : __cvtptr_(__pcvt), __cvtstate_(), __cvtcount_(0) { }
	wstring_convert(_Codecvt *__pcvt, state_type __state) : __cvtptr_(__pcvt), __cvtstate_(__state), __cvtcount_(0) { }
	wstring_convert(const byte_string &__byte_err, const wide_string &__wide_err = wide_string()) : __byte_err_string_(__byte_err), __wide_err_string_(__wide_err), __cvtptr_(new _Codecvt), __cvtstate_(), __cvtcount_(0) { }
	wstring_convert(wstring_convert &&__wc) : __byte_err_string_(move(__wc.__byte_err_string_)), __wide_err_string_(move(__wc.__wide_err_string_)), __cvtptr_(__wc.__cvtptr_), __cvtstate_(__wc.__cvtstate_), __cvtcount_(__wc.__cvtstate_)
	{
		__wc.__cvtptr_ = nullptr;
	}
	~wstring_convert() { delete this->__cvtptr_; }

	wide_string from_bytes(char __byte) { return from_bytes(&__byte, &__byte + 1); }
	wide_string from_bytes(const char *__ptr) { return from_bytes(__ptr, __ptr + char_traits<char>::length(__ptr)); }
	wide_string from_bytes(const byte_string &__str) { return from_bytes(__str.data(), __str.data() + __str.size()); }
	wide_string from_bytes(const char *__frm, const char *__frm_end)
	{
		this->__cvtcount_ = 0;
		if (this->__cvtptr_)
		{
			wide_string __ws(2 * (__frm_end - __frm), _Elem());
			if (__frm != __frm_end)
				__ws.resize(__ws.capacity());
			auto __r = codecvt_base::ok;
			auto __st = this->__cvtstate_;
			if (__frm != __frm_end)
			{
				auto __to = &__ws[0];
				auto __to_end = __to + __ws.size();
				const char *__frm_nxt;
				do
				{
					_Elem *__to_nxt;
					__r = this->__cvtptr_->in(__st, __frm, __frm_end, __frm_nxt, __to, __to_end, __to_nxt);
					this->__cvtcount_ += __frm_nxt - __frm;
					if (__frm_nxt == __frm)
						__r = codecvt_base::error;
					else if (__r == codecvt_base::noconv)
					{
						__ws.resize(__to - &__ws[0]);
						// This only gets executed if _Elem is char
						__ws.append(reinterpret_cast<const _Elem *>(__frm), reinterpret_cast<const _Elem *>(__frm_end));
						__frm = __frm_nxt;
						__r = codecvt_base::ok;
					}
					else if (__r == codecvt_base::ok)
					{
						__ws.resize(__to_nxt - &__ws[0]);
						__frm = __frm_nxt;
					}
					else if (__r == codecvt_base::partial)
					{
						ptrdiff_t __s = __to_nxt - &__ws[0];
						__ws.resize(2 * __s);
						__to = &__ws[0] + __s;
						__to_end = &__ws[0] + __ws.size();
						__frm = __frm_nxt;
					}
				} while (__r == codecvt_base::partial && __frm_nxt < __frm_end);
			}
			if (__r == codecvt_base::ok)
				return __ws;
		}
		if (this->__wide_err_string_.empty())
			throw range_error("wstring_convert: from_bytes error");
		return this->__wide_err_string_;
	}

	byte_string to_bytes(_Elem __wchar) { return to_bytes(&__wchar, &__wchar + 1); }
	byte_string to_bytes(const _Elem *__wptr) { return to_bytes(__wptr, __wptr + char_traits<_Elem>::length(__wptr)); }
	byte_string to_bytes(const wide_string &__wstr) { return to_bytes(__wstr.data(), __wstr.data() + __wstr.size()); }
	byte_string to_bytes(const _Elem *__frm, const _Elem *__frm_end)
	{
		this->__cvtcount_ = 0;
		if (this->__cvtptr_)
		{
			byte_string __bs(2 * (__frm_end - __frm), char());
			if (__frm != __frm_end)
				__bs.resize(__bs.capacity());
			auto __r = codecvt_base::ok;
			auto __st = this->__cvtstate_;
			if (__frm != __frm_end)
			{
				auto __to = &__bs[0];
				auto __to_end = __to + __bs.size();
				const _Elem *__frm_nxt;
				do
				{
					char *__to_nxt;
					__r = this->__cvtptr_->out(__st, __frm, __frm_end, __frm_nxt, __to, __to_end, __to_nxt);
					this->__cvtcount_ += __frm_nxt - __frm;
					if (__frm_nxt == __frm)
						__r = codecvt_base::error;
					else if (__r == codecvt_base::noconv)
					{
						__bs.resize(__to - &__bs[0]);
						// This only gets executed if _Elem is char
						__bs.append(reinterpret_cast<const char *>(__frm), reinterpret_cast<const char *>(__frm_end));
						__frm = __frm_nxt;
						__r = codecvt_base::ok;
					}
					else if (__r == codecvt_base::ok)
					{
						__bs.resize(__to_nxt - &__bs[0]);
						__frm = __frm_nxt;
					}
					else if (__r == codecvt_base::partial)
					{
						ptrdiff_t __s = __to_nxt - &__bs[0];
						__bs.resize(2 * __s);
						__to = &__bs[0] + __s;
						__to_end = &__bs[0] + __bs.size();
						__frm = __frm_nxt;
					}
				} while (__r == codecvt_base::partial && __frm_nxt < __frm_end);
			}
			if (__r == codecvt_base::ok)
			{
				auto __s = __bs.size();
				__bs.resize(__bs.capacity());
				auto __to = &__bs[0] + __s;
				auto __to_end = __to + __bs.size();
				do
				{
					char *__to_nxt;
					__r = this->__cvtptr_->unshift(__st, __to, __to_end, __to_nxt);
					if (__r == codecvt_base::noconv)
					{
						__bs.resize(__to - &__bs[0]);
						__r = codecvt_base::ok;
					}
					else if (__r == codecvt_base::ok)
						__bs.resize(__to_nxt - &__bs[0]);
					else if (__r == codecvt_base::partial)
					{
						ptrdiff_t __sp = __to_nxt - &__bs[0];
						__bs.resize(2 * __sp);
						__to = &__bs[0] + __sp;
						__to_end = &__bs[0] + __bs.size();
					}
				} while (__r == codecvt_base::partial);
				if (__r == codecvt_base::ok)
					return __bs;
			}
		}
		if (this->__byte_err_string_.empty())
			throw range_error("wstring_convert: to_bytes error");
		return this->__byte_err_string_;
	}

	size_t converted() const noexcept { return this->__cvtcount_; }
	state_type state() const { return this->__cvtstate_; }
};

}
#endif
