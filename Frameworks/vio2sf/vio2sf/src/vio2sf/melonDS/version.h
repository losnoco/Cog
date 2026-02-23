/*
    Copyright 2016-2026 melonDS team

    This file is part of melonDS.

    melonDS is free software: you can redistribute it and/or modify it under
    the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    melonDS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with melonDS. If not, see http://www.gnu.org/licenses/.
*/

#ifndef VERSION_H
#define VERSION_H

#define MELONDS_URL            "https://melonds.kuribo64.net"

#define MELONDS_VERSION_BASE   "1.1"
#define MELONDS_VERSION_SUFFIX "-33-gbdd85c9c"
#define MELONDS_VERSION        MELONDS_VERSION_BASE MELONDS_VERSION_SUFFIX

#ifdef MELONDS_EMBED_BUILD_INFO
#define MELONDS_GIT_BRANCH       "master"
#define MELONDS_GIT_HASH         "bdd85c9ccb40c0a3fcaa6103baf79c2d2d52d6ad"
#define MELONDS_BUILD_PROVIDER   "kode54"
#endif

#endif // VERSION_H

