/*
 * openmpt123_exception.hpp
 * ------------------------
 * Purpose: libopenmpt command line player
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef OPENMPT123_EXCEPTION_HPP
#define OPENMPT123_EXCEPTION_HPP

#include "openmpt123_config.hpp"

#include "mpt/string/types.hpp"
#include "mpt/string_transcode/transcode.hpp"

#include <string>

#include <libopenmpt/libopenmpt.hpp>

namespace openmpt123 {

struct exception : public openmpt::exception {
	exception( const mpt::ustring & text ) : openmpt::exception(mpt::transcode<std::string>( mpt::common_encoding::utf8, text )) { }
};

} // namespace openmpt123

#endif // OPENMPT123_EXCEPTION_HPP
