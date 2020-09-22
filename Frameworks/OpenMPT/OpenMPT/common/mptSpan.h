/*
 * mptSpan.h
 * ---------
 * Purpose: C++20 span.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"


#include "mptBaseTypes.h"

#if MPT_CXX_AT_LEAST(20)
#include <array>
#include <span>
#else // !C++20
#include <array>
#include <iterator>
#include <type_traits>
#endif // C++20



OPENMPT_NAMESPACE_BEGIN




namespace mpt {



#if MPT_CXX_AT_LEAST(20)

using std::span;

#else // !C++20

//  Simplified version of gsl::span.
//  Non-owning read-only or read-write view into a contiguous block of T
// objects, i.e. equivalent to a (beg,end) or (data,size) tuple.
//  Can eventually be replaced without further modifications with a full C++20
// std::span.
template <typename T>
class span
{

public:

	using element_type = T;
	using value_type = typename std::remove_cv<T>::type;
	using index_type = std::size_t;
	using pointer = T *;
	using const_pointer = const T *;
	using reference = T &;
	using const_reference = const T &;

	using iterator = pointer;
	using const_iterator = const_pointer;

	using difference_type = typename std::iterator_traits<iterator>::difference_type;

private:

	T * m_beg;
	T * m_end;

public:

	span() noexcept : m_beg(nullptr), m_end(nullptr) { }

	span(pointer beg, pointer end) : m_beg(beg), m_end(end) { }

	span(pointer data, index_type size) : m_beg(data), m_end(data + size) { }

	template <std::size_t N> span(element_type (&arr)[N]) : m_beg(arr), m_end(arr + N) { }

	template <std::size_t N> span(std::array<value_type, N> &arr) : m_beg(arr.data()), m_end(arr.data() + arr.size()) { }

	template <std::size_t N> span(const std::array<value_type, N> &arr) : m_beg(arr.data()), m_end(arr.data() + arr.size()) { }

	template <typename Cont> span(Cont &cont) : m_beg(std::data(cont)), m_end(std::data(cont) + std::size(cont)) { }

	span(const span &other) : m_beg(other.begin()), m_end(other.end()) { }

	template <typename U> span(const span<U> &other) : m_beg(other.begin()), m_end(other.end()) { }

	span & operator = (const span & other) noexcept = default;
	
	iterator begin() const { return iterator(m_beg); }
	iterator end() const { return iterator(m_end); }

	const_iterator cbegin() const { return const_iterator(begin()); }
	const_iterator cend() const { return const_iterator(end()); }

	operator bool () const noexcept { return m_beg != nullptr; }

	reference operator[](index_type index) { return at(index); }
	const_reference operator[](index_type index) const { return at(index); }

	bool operator==(span const & other) const noexcept { return size() == other.size() && (m_beg == other.m_beg || std::equal(begin(), end(), other.begin())); }
	bool operator!=(span const & other) const noexcept { return !(*this == other); }

	reference at(index_type index) { return m_beg[index]; }
	const_reference at(index_type index) const { return m_beg[index]; }

	pointer data() const noexcept { return m_beg; }

	bool empty() const noexcept { return size() == 0; }

	index_type size() const noexcept { return static_cast<index_type>(std::distance(m_beg, m_end)); }
	index_type length() const noexcept { return size(); }

}; // class span

#endif // C++20

template <typename T> inline span<T> as_span(T * beg, T * end) { return span<T>(beg, end); }

template <typename T> inline span<T> as_span(T * data, std::size_t size) { return span<T>(data, size); }

template <typename T, std::size_t N> inline span<T> as_span(T (&arr)[N]) { return span<T>(std::begin(arr), std::end(arr)); }

template <typename T, std::size_t N> inline span<T> as_span(std::array<T, N> & cont) { return span<T>(cont); }

template <typename T, std::size_t N> inline span<const T> as_span(const std::array<T, N> & cont) { return span<const T>(cont); }



} // namespace mpt



OPENMPT_NAMESPACE_END
