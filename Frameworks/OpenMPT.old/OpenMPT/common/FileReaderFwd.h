/*
 * FileReaderFwd.h
 * ---------------
 * Purpose: Forward declaration for class FileReader.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "BuildSettings.h"


OPENMPT_NAMESPACE_BEGIN

class FileReaderTraitsMemory;

class FileReaderTraitsStdStream;

using FileReaderTraitsDefault = FileReaderTraitsStdStream;

namespace detail {

template <typename Ttraits>
class FileReader;

} // namespace detail

using FileReader = detail::FileReader<FileReaderTraitsDefault>;

using MemoryFileReader = detail::FileReader<FileReaderTraitsMemory>;

OPENMPT_NAMESPACE_END

