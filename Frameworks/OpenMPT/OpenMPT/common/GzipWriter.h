/*
 * GzipWriter.h
 * ------------
 * Purpose: Simple wrapper around zlib's Gzip writer
 * Notes  : miniz doesn't implement Gzip writing, so this is only compatible with zlib.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "mptString.h"
#include "mpt/io/io.hpp"
#include "mpt/io/io_stdstream.hpp"

#ifdef MPT_WITH_ZLIB

#include <zlib.h>

OPENMPT_NAMESPACE_BEGIN

inline void WriteGzip(std::ostream &output, std::string &outData, const mpt::ustring &fileName)
{
	z_stream strm{};
	strm.avail_in = static_cast<uInt>(outData.size());
	strm.next_in = reinterpret_cast<Bytef *>(outData.data());
	if(deflateInit2(&strm, Z_BEST_COMPRESSION, Z_DEFLATED, 15 | 16, 9, Z_DEFAULT_STRATEGY) != Z_OK)
		throw std::runtime_error{"zlib init failed"};
	gz_header gzHeader{};
	gzHeader.time = static_cast<uLong>(time(nullptr));
	std::string filenameISO = mpt::ToCharset(mpt::Charset::ISO8859_1, fileName);
	gzHeader.name = reinterpret_cast<Bytef *>(filenameISO.data());
	deflateSetHeader(&strm, &gzHeader);
	try
	{
		do
		{
			std::array<Bytef, mpt::IO::BUFFERSIZE_TINY> buffer;
			strm.avail_out = static_cast<uInt>(buffer.size());
			strm.next_out = buffer.data();
			deflate(&strm, Z_FINISH);
			mpt::IO::WritePartial(output, buffer, buffer.size() - strm.avail_out);
		} while(strm.avail_out == 0);
		deflateEnd(&strm);
	} catch(const std::exception &)
	{
		deflateEnd(&strm);
		throw;
	}
}

OPENMPT_NAMESPACE_END

#endif
