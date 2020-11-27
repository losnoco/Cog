/*
 * StdAfx.h
 * --------
 * Purpose: Include file for standard system include files, or project specific include files that are used frequently, but are changed infrequently. Also includes the global build settings from BuildSettings.h.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once


// has to be first
#include "BuildSettings.h"


#if defined(MODPLUG_TRACKER)

#if defined(MPT_WITH_MFC)

// cppcheck-suppress missingInclude
#include <afx.h>            // MFC core
// cppcheck-suppress missingInclude
#include <afxwin.h>         // MFC standard components
// cppcheck-suppress missingInclude
#include <afxext.h>         // MFC extensions
// cppcheck-suppress missingInclude
#include <afxcmn.h>         // MFC support for Windows Common Controls
// cppcheck-suppress missingInclude
#include <afxcview.h>
// cppcheck-suppress missingInclude
#include <afxdlgs.h>
#ifdef MPT_MFC_FULL
// cppcheck-suppress missingInclude
#include <afxlistctrl.h>
#endif // MPT_MFC_FULL
// cppcheck-suppress missingInclude
#include <afxole.h>

#endif // MPT_WITH_MFC

#if MPT_OS_WINDOWS

#include <windows.h>
#include <windowsx.h>
#include <shlwapi.h>
#include <mmsystem.h>

#endif // MPT_OS_WINDOWS

#endif // MODPLUG_TRACKER


#if MPT_COMPILER_MSVC
#include <intrin.h>
#endif


// this will be available everywhere

#include "../common/mptBaseMacros.h"
// <array>
// <iterator>
// <type_traits>
// <cstddef>
// <cstdint>

#include "../common/mptBaseTypes.h"
// "mptBaseMacros.h"
// <array>
// <limits>
// <type_traits>
// <cstdint>

#include "../common/mptAssert.h"
// "mptBaseMacros.h"

#include "../common/mptBaseUtils.h"
// <algorithm>
// <bit>
// <limits>
// <numeric>
// <utility>

#include "../common/mptException.h"
// <exception>
// <new>
// <afx.h>

#include "../common/mptSpan.h"
// "mptBaseTypes.h"
// <array>
// <iterator>

#include "../common/mptMemory.h"
// "mptAssert.h"
// "mptBaseTypes.h"
// "mptSpan.h"
// <utility>
// <type_traits>
// <cstring>

#include "../common/mptAlloc.h"
// "mptBaseMacros.h"
// "mptMemory.h"
// "mptSpan.h"
// <array>
// <memory>
// <new>
// <vector>

#include "../common/mptString.h"
// <algorithm>
// <limits>
// <string>
// <string_view>
// <type_traits>
// <cstring>

#include "../common/mptStringBuffer.h"

#include "../common/mptOSError.h"
// "mptException.h"
// "mptString.h"
// <exception>
// <stdexcept>

#include "../common/mptExceptionText.h"
// "mptException.h"
// "mptString.h"
// <exception>

#include "../common/mptStringFormat.h"

#include "../common/mptPathString.h"

#include "../common/Logging.h"
// <atomic>

#include "../common/misc_util.h"

// for std::abs
#include <cstdlib>
#include <stdlib.h>
#include <cmath>
#include <math.h>


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
