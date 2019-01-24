/*
 * mptSpan.h
 * ---------
 * Purpose: Various useful utility functions.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"


#include "mptBaseTypes.h"

#include <array>
#include <iterator>



OPENMPT_NAMESPACE_BEGIN




namespace mpt {



//  Simplified version of gsl::span.
//  Non-owning read-only or read-write view into a contiguous block of T
// objects, i.e. equivalent to a (beg,end) or (data,size) tuple.
//  Can eventually be replaced without further modifications with a full C++20
// std::span.
template <typename T>
class span
{

public:

	typedef std::size_t size_type;

	typedef T value_type;
	typedef T & reference;
	typedef T * pointer;
	typedef const T * const_pointer;
	typedef const T & const_reference;

	typedef pointer iterator;
	typedef const_pointer const_iterator;

	typedef typename std::iterator_traits<iterator>::difference_type difference_type;

private:

	T * m_beg;
	T * m_end;

public:

	span() : m_beg(nullptr), m_end(nullptr) { }

	span(pointer beg, pointer end) : m_beg(beg), m_end(end) { }

	span(pointer data, size_type size) : m_beg(data), m_end(data + size) { }

	template <typename U, std::size_t N> span(U (&arr)[N]) : m_beg(arr), m_end(arr + N) { }

	template <typename Cont> span(Cont &cont) : m_beg(cont.empty() ? nullptr : &(cont[0])), m_end(cont.empty() ? nullptr : &(cont[0]) + cont.size()) { }

	span(const span &other) : m_beg(other.begin()), m_end(other.end()) { }

	template <typename U> span(const span<U> &other) : m_beg(other.begin()), m_end(other.end()) { }

	span & operator = (span other) { m_beg = other.begin(); m_end = other.end(); return *this; }

	iterator begin() const { return iterator(m_beg); }
	iterator end() const { return iterator(m_end); }

	const_iterator cbegin() const { return const_iterator(begin()); }
	const_iterator cend() const { return const_iterator(end()); }

	operator bool () const noexcept { return m_beg != nullptr; }

	reference operator[](size_type index) { return at(index); }
	const_reference operator[](size_type index) const { return at(index); }

	bool operator==(span const & other) const noexcept { return size() == other.size() && (m_beg == other.m_beg || std::equal(begin(), end(), other.begin())); }
	bool operator!=(span const & other) const noexcept { return !(*this == other); }

	reference at(size_type index) { return m_beg[index]; }
	const_reference at(size_type index) const { return m_beg[index]; }

	pointer data() const noexcept { return m_beg; }

	bool empty() const noexcept { return size() == 0; }

	size_type size() const noexcept { return static_cast<size_type>(std::distance(m_beg, m_end)); }
	size_type length() const noexcept { return size(); }

}; // class span

template <typename T> inline span<T> as_span(T * beg, T * end) { return span<T>(beg, end); }

template <typename T> inline span<T> as_span(T * data, std::size_t size) { return span<T>(data, size); }

template <typename T, std::size_t N> inline span<T> as_span(T (&arr)[N]) { return span<T>(std::begin(arr), std::end(arr)); }

template <typename T, std::size_t N> inline span<T> as_span(std::array<T, N> & cont) { return span<T>(cont); }

template <typename T, std::size_t N> inline span<const T> as_span(const std::array<T, N> & cont) { return span<const T>(cont); }



} // namespace mpt



OPENMPT_NAMESPACE_END
