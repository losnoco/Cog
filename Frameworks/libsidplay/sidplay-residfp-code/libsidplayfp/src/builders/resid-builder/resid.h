/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2013 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2001 Simon White
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

#ifndef RESID_H
#define RESID_H

#include "sidplayfp/sidbuilder.h"
#include "sidplayfp/siddefs.h"

/**
 * ReSID Builder Class
 */
class SID_EXTERN ReSIDBuilder : public sidbuilder
{
public:
    ReSIDBuilder(const char * const name) :
        sidbuilder(name) {}
    ~ReSIDBuilder();

    /**
     * Available sids.
     *
     * @return the number of available sids, 0 = endless.
     */
    unsigned int availDevices() const { return 0; }

    unsigned int create(unsigned int sids);

    const char *credits() const;

    /// @name global settings
    /// Settings that affect all SIDs
    //@{
    /**
     * enable/disable filter.
     */
    void filter(bool enable);

    /**
     * The bias is given in millivolts, and a maximum reasonable
     * control range is approximately -500 to 500.
     */
    void bias(double dac_bias);
    //@}
};

#endif // RESID_H
