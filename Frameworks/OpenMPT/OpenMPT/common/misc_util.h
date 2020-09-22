/*
 * misc_util.h
 * -----------
 * Purpose: Various useful utility functions.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"

#include "mptAssert.h"
#include "mptBaseMacros.h"
#include "mptBaseTypes.h"
#include "mptBaseUtils.h"
#include "mptString.h"

// old
#include "mptBaseUtils.h"
#include "mptSpan.h"
#include "mptMemory.h"
#include "mptExceptionText.h"
#include "mptStringFormat.h"
#include "mptStringParse.h"
#include "mptCPU.h"
#include "mptOS.h"
#include "mptTime.h"
#include "mptLibrary.h"

#include <vector>

#include <cstdlib>

#include <stdlib.h>


OPENMPT_NAMESPACE_BEGIN


namespace Util
{

	// Insert a range of items [insStart,  insEnd], and possibly shift item fix to the left.
	template<typename T>
	void InsertItem(const T insStart, const T insEnd, T &fix)
	{
		MPT_ASSERT(insEnd >= insStart);
		if(fix >= insStart)
		{
			fix += (insEnd - insStart + 1);
		}
	}

	// Insert a range of items [insStart,  insEnd], and possibly shift items in range [fixStart, fixEnd] to the right.
	template<typename T>
	void InsertRange(const T insStart, const T insEnd, T &fixStart, T &fixEnd)
	{
		MPT_ASSERT(insEnd >= insStart);
		const T insLength = insEnd - insStart + 1;
		if(fixStart >= insEnd)
		{
			fixStart += insLength;
		}
		if(fixEnd >= insEnd)
		{
			fixEnd += insLength;
		}
	}

	// Delete a range of items [delStart,  delEnd], and possibly shift item fix to the left.
	template<typename T>
	void DeleteItem(const T delStart, const T delEnd, T &fix)
	{
		MPT_ASSERT(delEnd >= delStart);
		if(fix > delEnd)
		{
			fix -= (delEnd - delStart + 1);
		}
	}

	// Delete a range of items [delStart,  delEnd], and possibly shift items in range [fixStart, fixEnd] to the left.
	template<typename T>
	void DeleteRange(const T delStart, const T delEnd, T &fixStart, T &fixEnd)
	{
		MPT_ASSERT(delEnd >= delStart);
		const T delLength = delEnd - delStart + 1;
		if(delStart < fixStart  && delEnd < fixStart)
		{
			// cut part is before loop start
			fixStart -= delLength;
			fixEnd -= delLength;
		} else if(delStart < fixStart  && delEnd < fixEnd)
		{
			// cut part is partly before loop start
			fixStart = delStart;
			fixEnd -= delLength;
		} else if(delStart >= fixStart && delEnd < fixEnd)
		{
			// cut part is in the loop
			fixEnd -= delLength;
		} else if(delStart >= fixStart && delStart < fixEnd && delEnd > fixEnd)
		{
			// cut part is partly before loop end
			fixEnd = delStart;
		}
	}

} // namespace Util



namespace Util
{

	template<typename T, std::size_t n>
	class fixed_size_queue
	{
	private:
		T buffer[n+1];
		std::size_t read_position;
		std::size_t write_position;
	public:
		fixed_size_queue() : read_position(0), write_position(0)
		{
			return;
		}
		void clear()
		{
			read_position = 0;
			write_position = 0;
		}
		std::size_t read_size() const
		{
			if ( write_position > read_position )
			{
				return write_position - read_position;
			} else if ( write_position < read_position )
			{
				return write_position - read_position + n + 1;
			} else
			{
				return 0;
			}
		}
		std::size_t write_size() const
		{
			if ( write_position > read_position )
			{
				return read_position - write_position + n;
			} else if ( write_position < read_position )
			{
				return read_position - write_position - 1;
			} else
			{
				return n;
			}
		}
		bool push( const T & v )
		{
			if ( !write_size() )
			{
				return false;
			}
			buffer[write_position] = v;
			write_position = ( write_position + 1 ) % ( n + 1 );
			return true;
		}
		bool pop() {
			if ( !read_size() )
			{
				return false;
			}
			read_position = ( read_position + 1 ) % ( n + 1 );
			return true;
		}
		T peek() {
			if ( !read_size() )
			{
				return T();
			}
			return buffer[read_position];
		}
		const T * peek_p()
		{
			if ( !read_size() )
			{
				return nullptr;
			}
			return &(buffer[read_position]);
		}
		const T * peek_next_p()
		{
			if ( read_size() < 2 )
			{
				return nullptr;
			}
			return &(buffer[(read_position+1)%(n+1)]);
		}
	};

} // namespace Util


namespace Util
{

std::vector<std::byte> HexToBin(const mpt::ustring &src);
mpt::ustring BinToHex(mpt::const_byte_span src);

template <typename T> inline mpt::ustring BinToHex(mpt::span<T> src) { return Util::BinToHex(mpt::byte_cast<mpt::const_byte_span>(src)); }

} // namespace Util


#if defined(MODPLUG_TRACKER) || (defined(LIBOPENMPT_BUILD) && defined(LIBOPENMPT_BUILD_TEST))

namespace mpt
{

// Wrapper around std::getenv.
// Instead of returning null pointer if the environment variable is not set,
// this wrapper returns the provided default value.
std::string getenv(const std::string &env_var, const std::string &def = std::string());

} // namespace mpt

#endif // MODPLUG_TRACKER || (LIBOPENMPT_BUILD && LIBOPENMPT_BUILD_TEST)


OPENMPT_NAMESPACE_END
