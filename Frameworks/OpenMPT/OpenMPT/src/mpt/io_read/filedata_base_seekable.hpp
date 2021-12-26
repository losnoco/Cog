/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_IO_READ_FILEDATA_BASE_SEEKABLE_HPP
#define MPT_IO_READ_FILEDATA_BASE_SEEKABLE_HPP



#include "mpt/base/memory.hpp"
#include "mpt/base/namespace.hpp"
#include "mpt/io_read/filedata.hpp"

#include <algorithm>
#include <vector>

#include <cstddef>



namespace mpt {
inline namespace MPT_INLINE_NS {



namespace IO {



class FileDataSeekable : public IFileData {

private:
	pos_type streamLength;

	mutable bool cached;
	mutable std::vector<std::byte> cache;

protected:
	FileDataSeekable(pos_type streamLength_)
		: streamLength(streamLength_)
		, cached(false) {
		return;
	}


private:
	void CacheStream() const {
		if (cached) {
			return;
		}
		cache.resize(streamLength);
		InternalReadSeekable(0, mpt::as_span(cache));
		cached = true;
	}

public:
	bool IsValid() const override {
		return true;
	}

	bool HasFastGetLength() const override {
		return true;
	}

	bool HasPinnedView() const override {
		return cached;
	}

	const std::byte * GetRawData() const override {
		CacheStream();
		return cache.data();
	}

	pos_type GetLength() const override {
		return streamLength;
	}

	mpt::byte_span Read(pos_type pos, mpt::byte_span dst) const override {
		if (cached) {
			IFileData::pos_type cache_avail = std::min(IFileData::pos_type(cache.size()) - pos, dst.size());
			std::copy(cache.begin() + pos, cache.begin() + pos + cache_avail, dst.data());
			return dst.first(cache_avail);
		} else {
			return InternalReadSeekable(pos, dst);
		}
	}

private:
	virtual mpt::byte_span InternalReadSeekable(pos_type pos, mpt::byte_span dst) const = 0;
};



} // namespace IO



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_IO_READ_FILEDATA_BASE_SEEKABLE_HPP
