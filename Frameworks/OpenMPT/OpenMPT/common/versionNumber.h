/*
 * versionNumber.h
 * ---------------
 * Purpose: OpenMPT version handling.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"

OPENMPT_NAMESPACE_BEGIN

#define VER_HELPER_STRINGIZE(x) #x
#define VER_STRINGIZE(x)        VER_HELPER_STRINGIZE(x)

//Version definitions. The only thing that needs to be changed when changing version number.
#define VER_MAJORMAJOR  1
#define VER_MAJOR      28
#define VER_MINOR      08
#define VER_MINORMINOR 00

//Numerical value of the version.
#define MPT_VERSION_CURRENT MAKE_VERSION_NUMERIC(VER_MAJORMAJOR,VER_MAJOR,VER_MINOR,VER_MINORMINOR)

OPENMPT_NAMESPACE_END
