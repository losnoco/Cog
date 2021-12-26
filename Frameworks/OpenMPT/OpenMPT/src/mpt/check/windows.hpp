/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_CHECK_WINDOWS_HPP
#define MPT_CHECK_WINDOWS_HPP

#include "mpt/base/detect_os.hpp"
#include "mpt/base/compiletime_warning.hpp"

#if MPT_OS_WINDOWS

#ifndef UNICODE
#ifndef MPT_CHECK_WINDOWS_IGNORE_WARNING_NO_UNICODE
MPT_WARNING("windows.h uses MBCS TCHAR. Please #define UNICODE.")
#endif
#endif

#ifndef NOMINMAX
#ifndef MPT_CHECK_WINDOWS_IGNORE_WARNING_NO_NOMINMAX
MPT_WARNING("windows.h defines min and max which conflicts with C++. Please #define NOMINMAX.")
#endif
#endif

#endif // MPT_OS_WINDOWS

#endif // MPT_CHECK_WINDOWS_HPP
