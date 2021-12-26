/*
 * FlagSet.h
 * ---------
 * Purpose: A flexible and typesafe flag set class.
 * Notes  : Originally based on http://stackoverflow.com/questions/4226960/type-safer-bitflags-in-c .
 *          Rewritten to be standard-conforming.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"

#include <string>

OPENMPT_NAMESPACE_BEGIN


// Be aware of the required size when specializing this.
// We cannot assert the minimum size because some compilers always allocate an 'int',
// even for enums that would fit in smaller integral types.
template <typename Tenum>
struct enum_traits
{
	using store_type = typename std::make_unsigned<Tenum>::type;
};


// Type-safe wrapper around an enum, that can represent all enum values and bitwise compositions thereof.
// Conversions to and from plain integers as well as conversions to the base enum are always explicit.
template <typename enum_t>
// cppcheck-suppress copyCtorAndEqOperator
class enum_value_type
{
public:
	using enum_type = enum_t;
	using value_type = enum_value_type;
	using store_type = typename enum_traits<enum_t>::store_type;
private:
	store_type bits;
public:
	MPT_CONSTEXPR11_FUN enum_value_type() noexcept : bits(0) { }
	MPT_CONSTEXPR11_FUN enum_value_type(const enum_value_type &x) noexcept : bits(x.bits) { }
	MPT_CONSTEXPR11_FUN enum_value_type(enum_type x) noexcept : bits(static_cast<store_type>(x)) { }
private:
	explicit MPT_CONSTEXPR11_FUN enum_value_type(store_type x) noexcept : bits(x) { } // private in order to prevent accidental conversions. use from_bits.
	MPT_CONSTEXPR11_FUN operator store_type () const noexcept { return bits; }  // private in order to prevent accidental conversions. use as_bits.
public:
	static MPT_CONSTEXPR11_FUN enum_value_type from_bits(store_type bits) noexcept { return value_type(bits); }
	MPT_CONSTEXPR11_FUN enum_type as_enum() const noexcept { return static_cast<enum_t>(bits); }
	MPT_CONSTEXPR11_FUN store_type as_bits() const noexcept { return bits; }
public:
	MPT_CONSTEXPR11_FUN operator bool () const noexcept { return bits != store_type(); }
	MPT_CONSTEXPR11_FUN bool operator ! () const noexcept { return bits == store_type(); }

	MPT_CONSTEXPR11_FUN const enum_value_type operator ~ () const noexcept { return enum_value_type(~bits); }

	friend MPT_CONSTEXPR11_FUN bool operator == (enum_value_type a, enum_value_type b) noexcept { return a.bits == b.bits; }
	friend MPT_CONSTEXPR11_FUN bool operator != (enum_value_type a, enum_value_type b) noexcept { return a.bits != b.bits; }
	
	friend MPT_CONSTEXPR11_FUN bool operator == (enum_value_type a, enum_t b) noexcept { return a == enum_value_type(b); }
	friend MPT_CONSTEXPR11_FUN bool operator != (enum_value_type a, enum_t b) noexcept { return a != enum_value_type(b); }
	
	friend MPT_CONSTEXPR11_FUN bool operator == (enum_t a, enum_value_type b) noexcept { return enum_value_type(a) == b; }
	friend MPT_CONSTEXPR11_FUN bool operator != (enum_t a, enum_value_type b) noexcept { return enum_value_type(a) != b; }
	
	friend MPT_CONSTEXPR11_FUN const enum_value_type operator | (enum_value_type a, enum_value_type b) noexcept { return enum_value_type(a.bits | b.bits); }
	friend MPT_CONSTEXPR11_FUN const enum_value_type operator & (enum_value_type a, enum_value_type b) noexcept { return enum_value_type(a.bits & b.bits); }
	friend MPT_CONSTEXPR11_FUN const enum_value_type operator ^ (enum_value_type a, enum_value_type b) noexcept { return enum_value_type(a.bits ^ b.bits); }
	
	friend MPT_CONSTEXPR11_FUN const enum_value_type operator | (enum_value_type a, enum_t b) noexcept { return a | enum_value_type(b); }
	friend MPT_CONSTEXPR11_FUN const enum_value_type operator & (enum_value_type a, enum_t b) noexcept { return a & enum_value_type(b); }
	friend MPT_CONSTEXPR11_FUN const enum_value_type operator ^ (enum_value_type a, enum_t b) noexcept { return a ^ enum_value_type(b); }
	
	friend MPT_CONSTEXPR11_FUN const enum_value_type operator | (enum_t a, enum_value_type b) noexcept { return enum_value_type(a) | b; }
	friend MPT_CONSTEXPR11_FUN const enum_value_type operator & (enum_t a, enum_value_type b) noexcept { return enum_value_type(a) & b; }
	friend MPT_CONSTEXPR11_FUN const enum_value_type operator ^ (enum_t a, enum_value_type b) noexcept { return enum_value_type(a) ^ b; }
	
	MPT_CONSTEXPR14_FUN enum_value_type &operator |= (enum_value_type b) noexcept { *this = *this | b; return *this; }
	MPT_CONSTEXPR14_FUN enum_value_type &operator &= (enum_value_type b) noexcept { *this = *this & b; return *this; }
	MPT_CONSTEXPR14_FUN enum_value_type &operator ^= (enum_value_type b) noexcept { *this = *this ^ b; return *this; }

	MPT_CONSTEXPR14_FUN enum_value_type &operator |= (enum_t b) noexcept { *this = *this | b; return *this; }
	MPT_CONSTEXPR14_FUN enum_value_type &operator &= (enum_t b) noexcept { *this = *this & b; return *this; }
	MPT_CONSTEXPR14_FUN enum_value_type &operator ^= (enum_t b) noexcept { *this = *this ^ b; return *this; }

};


// Type-safe enum wrapper that allows type-safe bitwise testing.
template <typename enum_t>
class Enum
{
public:
	using self_type = Enum;
	using enum_type = enum_t;
	using value_type = enum_value_type<enum_t>;
	using store_type = typename value_type::store_type;
private:
	enum_type value;
public:
	explicit MPT_CONSTEXPR11_FUN Enum(enum_type val) noexcept : value(val) { }
	MPT_CONSTEXPR11_FUN operator enum_type () const noexcept { return value; }
	MPT_CONSTEXPR14_FUN Enum &operator = (enum_type val) noexcept { value = val; return *this; }
public:
	MPT_CONSTEXPR11_FUN const value_type operator ~ () const { return ~value_type(value); }

	friend MPT_CONSTEXPR11_FUN bool operator == (self_type a, self_type b) noexcept { return value_type(a) == value_type(b); }
	friend MPT_CONSTEXPR11_FUN bool operator != (self_type a, self_type b) noexcept { return value_type(a) != value_type(b); }

	friend MPT_CONSTEXPR11_FUN bool operator == (self_type a, value_type b) noexcept { return value_type(a) == value_type(b); }
	friend MPT_CONSTEXPR11_FUN bool operator != (self_type a, value_type b) noexcept { return value_type(a) != value_type(b); }

	friend MPT_CONSTEXPR11_FUN bool operator == (value_type a, self_type b) noexcept { return value_type(a) == value_type(b); }
	friend MPT_CONSTEXPR11_FUN bool operator != (value_type a, self_type b) noexcept { return value_type(a) != value_type(b); }

	friend MPT_CONSTEXPR11_FUN bool operator == (self_type a, enum_type b) noexcept { return value_type(a) == value_type(b); }
	friend MPT_CONSTEXPR11_FUN bool operator != (self_type a, enum_type b) noexcept { return value_type(a) != value_type(b); }

	friend MPT_CONSTEXPR11_FUN bool operator == (enum_type a, self_type b) noexcept { return value_type(a) == value_type(b); }
	friend MPT_CONSTEXPR11_FUN bool operator != (enum_type a, self_type b) noexcept { return value_type(a) != value_type(b); }

	friend MPT_CONSTEXPR11_FUN const value_type operator | (self_type a, self_type b) noexcept { return value_type(a) | value_type(b); }
	friend MPT_CONSTEXPR11_FUN const value_type operator & (self_type a, self_type b) noexcept { return value_type(a) & value_type(b); }
	friend MPT_CONSTEXPR11_FUN const value_type operator ^ (self_type a, self_type b) noexcept { return value_type(a) ^ value_type(b); }

	friend MPT_CONSTEXPR11_FUN const value_type operator | (self_type a, value_type b) noexcept { return value_type(a) | value_type(b); }
	friend MPT_CONSTEXPR11_FUN const value_type operator & (self_type a, value_type b) noexcept { return value_type(a) & value_type(b); }
	friend MPT_CONSTEXPR11_FUN const value_type operator ^ (self_type a, value_type b) noexcept { return value_type(a) ^ value_type(b); }

	friend MPT_CONSTEXPR11_FUN const value_type operator | (value_type a, self_type b) noexcept { return value_type(a) | value_type(b); }
	friend MPT_CONSTEXPR11_FUN const value_type operator & (value_type a, self_type b) noexcept { return value_type(a) & value_type(b); }
	friend MPT_CONSTEXPR11_FUN const value_type operator ^ (value_type a, self_type b) noexcept { return value_type(a) ^ value_type(b); }

	friend MPT_CONSTEXPR11_FUN const value_type operator | (self_type a, enum_type b) noexcept { return value_type(a) | value_type(b); }
	friend MPT_CONSTEXPR11_FUN const value_type operator & (self_type a, enum_type b) noexcept { return value_type(a) & value_type(b); }
	friend MPT_CONSTEXPR11_FUN const value_type operator ^ (self_type a, enum_type b) noexcept { return value_type(a) ^ value_type(b); }

	friend MPT_CONSTEXPR11_FUN const value_type operator | (enum_type a, self_type b) noexcept { return value_type(a) | value_type(b); }
	friend MPT_CONSTEXPR11_FUN const value_type operator & (enum_type a, self_type b) noexcept { return value_type(a) & value_type(b); }
	friend MPT_CONSTEXPR11_FUN const value_type operator ^ (enum_type a, self_type b) noexcept { return value_type(a) ^ value_type(b); }

};


template <typename enum_t, typename store_t = typename enum_value_type<enum_t>::store_type >
// cppcheck-suppress copyCtorAndEqOperator
class FlagSet
{
public:
	using self_type = FlagSet;
	using enum_type = enum_t;
	using value_type = enum_value_type<enum_t>;
	using store_type = store_t;
	
private:

	// support truncated store_type ... :
	store_type bits_;
	static MPT_CONSTEXPR11_FUN store_type store_from_value(value_type bits) noexcept { return static_cast<store_type>(bits.as_bits()); }
	static MPT_CONSTEXPR11_FUN value_type value_from_store(store_type bits) noexcept { return value_type::from_bits(static_cast<typename value_type::store_type>(bits)); }

	MPT_CONSTEXPR14_FUN FlagSet & store(value_type bits) noexcept { bits_ = store_from_value(bits); return *this; }
	MPT_CONSTEXPR11_FUN value_type load() const noexcept { return value_from_store(bits_); }

public:

	// Default constructor (no flags set)
	MPT_CONSTEXPR11_FUN FlagSet() noexcept : bits_(store_from_value(value_type()))
	{
	}
	
	// Copy constructor
	MPT_CONSTEXPR11_FUN FlagSet(const FlagSet &flags) noexcept
		: bits_(flags.bits_)
	{
	}

	// Value constructor
	MPT_CONSTEXPR11_FUN FlagSet(value_type flags) noexcept : bits_(store_from_value(value_type(flags)))
	{
	}

	MPT_CONSTEXPR11_FUN FlagSet(enum_type flag) noexcept : bits_(store_from_value(value_type(flag)))
	{
	}

	explicit MPT_CONSTEXPR11_FUN FlagSet(store_type flags) noexcept : bits_(store_from_value(value_type::from_bits(flags)))
	{
	}
	
	MPT_CONSTEXPR11_FUN explicit operator bool () const noexcept
	{
		return load();
	}
	MPT_CONSTEXPR11_FUN explicit operator store_type () const noexcept
	{
		return load().as_bits();
	}
	
	MPT_CONSTEXPR11_FUN value_type value() const noexcept
	{
		return load();
	}
	
	MPT_CONSTEXPR11_FUN operator value_type () const noexcept
	{
		return load();
	}

	// Test if one or more flags are set. Returns true if at least one of the given flags is set.
	MPT_CONSTEXPR11_FUN bool operator[] (value_type flags) const noexcept
	{
		return test(flags);
	}

	// String representation of flag set
	std::string to_string() const noexcept
	{
		std::string str(size_bits(), '0');

		for(size_t x = 0; x < size_bits(); ++x)
		{
			str[size_bits() - x - 1] = (load() & (1 << x) ? '1' : '0');
		}

		return str;
	}
	
	// Set one or more flags.
	MPT_CONSTEXPR14_FUN FlagSet &set(value_type flags) noexcept
	{
		return store(load() | flags);
	}

	// Set or clear one or more flags.
	MPT_CONSTEXPR14_FUN FlagSet &set(value_type flags, bool val) noexcept
	{
		return store((val ? (load() | flags) : (load() & ~flags)));
	}

	// Clear or flags.
	MPT_CONSTEXPR14_FUN FlagSet &reset() noexcept
	{
		return store(value_type());
	}

	// Clear one or more flags.
	MPT_CONSTEXPR14_FUN FlagSet &reset(value_type flags) noexcept
	{
		return store(load() & ~flags);
	}

	// Toggle all flags.
	MPT_CONSTEXPR14_FUN FlagSet &flip() noexcept
	{
		return store(~load());
	}

	// Toggle one or more flags.
	MPT_CONSTEXPR14_FUN FlagSet &flip(value_type flags) noexcept
	{
		return store(load() ^ flags);
	}

	// Returns the size of the flag set in bytes
	MPT_CONSTEXPR11_FUN std::size_t size() const noexcept
	{
		return sizeof(store_type);
	}

	// Returns the size of the flag set in bits
	MPT_CONSTEXPR11_FUN std::size_t size_bits() const noexcept
	{
		return size() * 8;
	}
	
	// Test if one or more flags are set. Returns true if at least one of the given flags is set.
	MPT_CONSTEXPR11_FUN bool test(value_type flags) const noexcept
	{
		return (load() & flags);
	}

	// Test if all specified flags are set.
	MPT_CONSTEXPR11_FUN bool test_all(value_type flags) const noexcept
	{
		return (load() & flags) == flags;
	}

	// Test if any but the specified flags are set.
	MPT_CONSTEXPR11_FUN bool test_any_except(value_type flags) const noexcept
	{
		return (load() & ~flags);
	}

	// Test if any flag is set.
	MPT_CONSTEXPR11_FUN bool any() const noexcept
	{
		return load();
	}

	// Test if no flags are set.
	MPT_CONSTEXPR11_FUN bool none() const noexcept
	{
		return !load();
	}
	
	MPT_CONSTEXPR11_FUN store_type GetRaw() const noexcept
	{
		return bits_;
	}

	MPT_CONSTEXPR14_FUN FlagSet & SetRaw(store_type flags) noexcept
	{
		bits_ = flags;
		return *this;
	}

	MPT_CONSTEXPR14_FUN FlagSet &operator = (value_type flags) noexcept
	{
		return store(flags);
	}
	
	MPT_CONSTEXPR14_FUN FlagSet &operator = (enum_type flag) noexcept
	{
		return store(flag);
	}

	MPT_CONSTEXPR14_FUN FlagSet &operator = (FlagSet flags) noexcept
	{
		return store(flags.load());
	}

	MPT_CONSTEXPR14_FUN FlagSet &operator &= (value_type flags) noexcept
	{
		return store(load() & flags);
	}

	MPT_CONSTEXPR14_FUN FlagSet &operator |= (value_type flags) noexcept
	{
		return store(load() | flags);
	}

	MPT_CONSTEXPR14_FUN FlagSet &operator ^= (value_type flags) noexcept
	{
		return store(load() ^ flags);
	}
	
	friend MPT_CONSTEXPR11_FUN bool operator == (self_type a, self_type b) noexcept { return a.load() == b.load(); }
	friend MPT_CONSTEXPR11_FUN bool operator != (self_type a, self_type b) noexcept { return a.load() != b.load(); }

	friend MPT_CONSTEXPR11_FUN bool operator == (self_type a, value_type b) noexcept { return a.load() == value_type(b); }
	friend MPT_CONSTEXPR11_FUN bool operator != (self_type a, value_type b) noexcept { return a.load() != value_type(b); }

	friend MPT_CONSTEXPR11_FUN bool operator == (value_type a, self_type b) noexcept { return value_type(a) == b.load(); }
	friend MPT_CONSTEXPR11_FUN bool operator != (value_type a, self_type b) noexcept { return value_type(a) != b.load(); }

	friend MPT_CONSTEXPR11_FUN bool operator == (self_type a, enum_type b) noexcept { return a.load() == value_type(b); }
	friend MPT_CONSTEXPR11_FUN bool operator != (self_type a, enum_type b) noexcept { return a.load() != value_type(b); }

	friend MPT_CONSTEXPR11_FUN bool operator == (enum_type a, self_type b) noexcept { return value_type(a) == b.load(); }
	friend MPT_CONSTEXPR11_FUN bool operator != (enum_type a, self_type b) noexcept { return value_type(a) != b.load(); }

	friend MPT_CONSTEXPR11_FUN bool operator == (self_type a, Enum<enum_type> b) noexcept { return a.load() == value_type(b); }
	friend MPT_CONSTEXPR11_FUN bool operator != (self_type a, Enum<enum_type> b) noexcept { return a.load() != value_type(b); }

	friend MPT_CONSTEXPR11_FUN bool operator == (Enum<enum_type> a, self_type b) noexcept { return value_type(a) == b.load(); }
	friend MPT_CONSTEXPR11_FUN bool operator != (Enum<enum_type> a, self_type b) noexcept { return value_type(a) != b.load(); }

	friend MPT_CONSTEXPR11_FUN const value_type operator | (self_type a, self_type b) noexcept { return a.load() | b.load(); }
	friend MPT_CONSTEXPR11_FUN const value_type operator & (self_type a, self_type b) noexcept { return a.load() & b.load(); }
	friend MPT_CONSTEXPR11_FUN const value_type operator ^ (self_type a, self_type b) noexcept { return a.load() ^ b.load(); }

	friend MPT_CONSTEXPR11_FUN const value_type operator | (self_type a, value_type b) noexcept { return a.load() | value_type(b); }
	friend MPT_CONSTEXPR11_FUN const value_type operator & (self_type a, value_type b) noexcept { return a.load() & value_type(b); }
	friend MPT_CONSTEXPR11_FUN const value_type operator ^ (self_type a, value_type b) noexcept { return a.load() ^ value_type(b); }

	friend MPT_CONSTEXPR11_FUN const value_type operator | (value_type a, self_type b) noexcept { return value_type(a) | b.load(); }
	friend MPT_CONSTEXPR11_FUN const value_type operator & (value_type a, self_type b) noexcept { return value_type(a) & b.load(); }
	friend MPT_CONSTEXPR11_FUN const value_type operator ^ (value_type a, self_type b) noexcept { return value_type(a) ^ b.load(); }

	friend MPT_CONSTEXPR11_FUN const value_type operator | (self_type a, enum_type b) noexcept { return a.load() | value_type(b); }
	friend MPT_CONSTEXPR11_FUN const value_type operator & (self_type a, enum_type b) noexcept { return a.load() & value_type(b); }
	friend MPT_CONSTEXPR11_FUN const value_type operator ^ (self_type a, enum_type b) noexcept { return a.load() ^ value_type(b); }

	friend MPT_CONSTEXPR11_FUN const value_type operator | (enum_type a, self_type b) noexcept { return value_type(a) | b.load(); }
	friend MPT_CONSTEXPR11_FUN const value_type operator & (enum_type a, self_type b) noexcept { return value_type(a) & b.load(); }
	friend MPT_CONSTEXPR11_FUN const value_type operator ^ (enum_type a, self_type b) noexcept { return value_type(a) ^ b.load(); }

	friend MPT_CONSTEXPR11_FUN const value_type operator | (self_type a, Enum<enum_type> b) noexcept { return a.load() | value_type(b); }
	friend MPT_CONSTEXPR11_FUN const value_type operator & (self_type a, Enum<enum_type> b) noexcept { return a.load() & value_type(b); }
	friend MPT_CONSTEXPR11_FUN const value_type operator ^ (self_type a, Enum<enum_type> b) noexcept { return a.load() ^ value_type(b); }

	friend MPT_CONSTEXPR11_FUN const value_type operator | (Enum<enum_type> a, self_type b) noexcept { return value_type(a) | b.load(); }
	friend MPT_CONSTEXPR11_FUN const value_type operator & (Enum<enum_type> a, self_type b) noexcept { return value_type(a) & b.load(); }
	friend MPT_CONSTEXPR11_FUN const value_type operator ^ (Enum<enum_type> a, self_type b) noexcept { return value_type(a) ^ b.load(); }

};


// Declare typesafe logical operators for enum_t
#define MPT_DECLARE_ENUM(enum_t) \
	MPT_CONSTEXPR11_FUN enum_value_type<enum_t> operator | (enum_t a, enum_t b) noexcept { return enum_value_type<enum_t>(a) | enum_value_type<enum_t>(b); } \
	MPT_CONSTEXPR11_FUN enum_value_type<enum_t> operator & (enum_t a, enum_t b) noexcept { return enum_value_type<enum_t>(a) & enum_value_type<enum_t>(b); } \
	MPT_CONSTEXPR11_FUN enum_value_type<enum_t> operator ^ (enum_t a, enum_t b) noexcept { return enum_value_type<enum_t>(a) ^ enum_value_type<enum_t>(b); } \
	MPT_CONSTEXPR11_FUN enum_value_type<enum_t> operator ~ (enum_t a) noexcept { return ~enum_value_type<enum_t>(a); } \
/**/

// backwards compatibility
#define DECLARE_FLAGSET MPT_DECLARE_ENUM


OPENMPT_NAMESPACE_END
