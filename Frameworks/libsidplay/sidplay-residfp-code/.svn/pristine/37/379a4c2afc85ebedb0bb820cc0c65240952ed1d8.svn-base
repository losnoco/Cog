/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2013 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2000-2002 Simon White
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

#ifndef  HARDSID_H
#define  HARDSID_H

#include "sidplayfp/sidbuilder.h"
#include "sidplayfp/siddefs.h"

/**
 * HardSID Builder Class
 */
class SID_EXTERN HardSIDBuilder : public sidbuilder
{
private:
    static bool m_initialised;

#ifndef _WIN32
    static unsigned int m_count;
#endif

    int init();

public:
    HardSIDBuilder(const char * const name);
    ~HardSIDBuilder();

    /**
     * Available sids.
     *
     * @return the number of available sids, 0 = endless.
     */
    unsigned int availDevices() const;

    const char *credits() const;

    void flush();

    /**
     * enable/disable filter.
     */
    void filter(bool enable);

    /**
     * Create the sid emu.
     *
     * @param sids the number of required sid emu
     */
    unsigned int create(unsigned int sids);
};

#endif // HARDSID_H
