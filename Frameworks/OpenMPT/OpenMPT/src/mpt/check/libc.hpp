/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_CHECK_LIBC_HPP
#define MPT_CHECK_LIBC_HPP

#include "mpt/base/detect_os.hpp"
#include "mpt/base/compiletime_warning.hpp"

#ifndef __STDC_CONSTANT_MACROS
#ifndef MPT_CHECK_LIBC_IGNORE_WARNING_NO_STDC_CONSTANT_MACROS
MPT_WARNING("C stdlib does not provide constant macros. Please #define __STDC_CONSTANT_MACROS.")
#endif
#endif

#ifndef __STDC_FORMAT_MACROS
#ifndef MPT_CHECK_LIBC_IGNORE_WARNING_NO_STDC_FORMAT_MACROS
MPT_WARNING("C stdlib does not provide limit macros. Please #define __STDC_FORMAT_MACROS.")
#endif
#endif

#ifndef __STDC_LIMIT_MACROS
#ifndef MPT_CHECK_LIBC_IGNORE_WARNING_NO_STDC_LIMIT_MACROS
MPT_WARNING("C stdlib does not provide limit macros. Please #define __STDC_LIMIT_MACROS.")
#endif
#endif

#ifndef _USE_MATH_DEFINES
#ifndef MPT_CHECK_LIBC_IGNORE_WARNING_NO_USE_MATH_DEFINES
MPT_WARNING("C stdlib does not provide math constants. Please #define _USE_MATH_DEFINES.")
#endif
#endif

#if !MPT_LIBC_MS
#if !defined(_FILE_OFFSET_BITS)
#ifndef MPT_CHECK_LIBC_IGNORE_WARNING_NO_FILE_OFFSET_BITS
MPT_WARNING("C stdlib may not provide 64bit std::FILE access. Please #define _FILE_OFFSET_BITS=64.")
#endif
#endif
#endif

#endif // MPT_CHECK_LIBC_HPP
