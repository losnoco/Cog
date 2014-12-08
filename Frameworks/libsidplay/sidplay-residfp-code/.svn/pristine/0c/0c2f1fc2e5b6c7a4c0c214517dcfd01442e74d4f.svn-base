/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2014 Leandro Nini <drfiemost@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef MD5_FACTORY_H
#define MD5_FACTORY_H

#include "iMd5.h"

#include "sidcxx11.h"

#include <memory>

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef HAVE_LIBGCRYPT
#  include "md5Gcrypt.h"
#else
#  include "md5Internal.h"
#endif

namespace libsidplayfp
{

namespace md5Factory
{
    static std::unique_ptr<iMd5> get()
    {
        return std::unique_ptr<iMd5>(
#ifdef HAVE_LIBGCRYPT
            new md5Gcrypt()
#else
            new md5Internal()
#endif
        );
    }
}

}

#endif // MD5_FACTORY_H
