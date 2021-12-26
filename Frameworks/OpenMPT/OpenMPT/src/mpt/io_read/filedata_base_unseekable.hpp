/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_IO_READ_FILEDATA_BASE_UNSEEKABLE_HPP
#define MPT_IO_READ_FILEDATA_BASE_UNSEEKABLE_HPP



#include "mpt/base/algorithm.hpp"
#include "mpt/base/memory.hpp"
#include "mpt/base/namespace.hpp"
#include "mpt/base/numeric.hpp"
#include "mpt/io/base.hpp"
#include "mpt/io_read/filedata.hpp"

#include <algorithm>
#include <limits>
#include <vector>

#include <cstddef>



namespace mpt {
inline namespace MPT_INLINE_NS {



namespace IO {



class FileDataUnseekable : public IFileData {

private:
	mutable std::vector<std::byte> cache;
	mutable std::size_t cachesize;
	mutable bool streamFullyCached;

protected:
	FileDataUnseekable()
		: cachesize(0), streamFullyCached(false) {
		return;
	}

private:
	enum : std::size_t
	{
		QUANTUM_SIZE = mpt::IO::BUFFERSIZE_SMALL,
		BUFFER_SIZE = mpt::IO::BUFFERSIZE_NORMAL
	};

	void EnsureCacheBuffer(std::size_t requiredbuffersize) const {
		if (cache.size() >= cachesize + requiredbuffersize) {
			return;
		}
		if (cache.size() == 0) {
			cache.resize(mpt::align_up<std::size_t>(cachesize + requiredbuffersize, BUFFER_SIZE));
		} else if (mpt::exponential_grow(cache.size()) < cachesize + requiredbuffersize) {
			cache.resize(mpt::align_up<std::size_t>(cachesize + requiredbuffersize, BUFFER_SIZE));
		} else {
			cache.resize(mpt::exponential_grow(cache.size()));
		}
	}

	void CacheStream() const {
		if (streamFullyCached) {
			return;
		}
		while (!InternalEof()) {
			EnsureCacheBuffer(BUFFER_SIZE);
			std::size_t readcount = InternalReadUnseekable(mpt::span(&cache[cachesize], BUFFER_SIZE)).size();
			cachesize += readcount;
		}
		streamFullyCached = true;
	}


	void CacheStreamUpTo(pos_type pos, pos_type length) const {
		if (streamFullyCached) {
			return;
		}
		if (length > std::numeric_limits<pos_type>::max() - pos) {
			length = std::numeric_limits<pos_type>::max() - pos;
		}
		std::size_t target = mpt::saturate_cast<std::size_t>(pos + length);
		if (target <= cachesize) {
			return;
		}
		std::size_t alignedpos = mpt::align_up<std::size_t>(target, QUANTUM_SIZE);
		std::size_t needcount = alignedpos - cachesize;
		EnsureCacheBuffer(needcount);
		std::size_t readcount = InternalReadUnseekable(mpt::span(&cache[cachesize], alignedpos - cachesize)).size();
		cachesize += readcount;
		if (!InternalEof()) {
			// can read further
			return;
		}
		streamFullyCached = true;
	}

private:
	void ReadCached(pos_type pos, mpt::byte_span dst) const {
		std::copy(cache.begin() + pos, cache.begin() + pos + dst.size(), dst.data());
	}

public:
	bool IsValid() const override {
		return true;
	}

	bool HasFastGetLength() const override {
		return false;
	}

	bool HasPinnedView() const override {
		return true; // we have the cache which is required for seeking anyway
	}

	const std::byte * GetRawData() const override {
		CacheStream();
		return cache.data();
	}

	pos_type GetLength() const override {
		CacheStream();
		return cachesize;
	}

	mpt::byte_span Read(pos_type pos, mpt::byte_span dst) const override {
		CacheStreamUpTo(pos, dst.size());
		if (pos >= IFileData::pos_type(cachesize)) {
			return dst.first(0);
		}
		IFileData::pos_type cache_avail = std::min(IFileData::pos_type(cachesize) - pos, dst.size());
		ReadCached(pos, dst.subspan(0, cache_avail));
		return dst.subspan(0, cache_avail);
	}

	bool CanRead(pos_type pos, std::size_t length) const override {
		CacheStreamUpTo(pos, length);
		if ((pos == IFileData::pos_type(cachesize)) && (length == 0)) {
			return true;
		}
		if (pos >= IFileData::pos_type(cachesize)) {
			return false;
		}
		return length <= IFileData::pos_type(cachesize) - pos;
	}

	std::size_t GetReadableLength(pos_type pos, std::size_t length) const override {
		CacheStreamUpTo(pos, length);
		if (pos >= cachesize) {
			return 0;
		}
		return std::min(static_cast<IFileData::pos_type>(cachesize) - pos, length);
	}

private:
	virtual bool InternalEof() const = 0;
	virtual mpt::byte_span InternalReadUnseekable(mpt::byte_span dst) const = 0;
};



} // namespace IO



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_IO_READ_FILEDATA_BASE_UNSEEKABLE_HPP
