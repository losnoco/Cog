/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 *  Copyright 2011-2015 Leandro Nini
 *  Copyright 2007-2010 Antti Lankila
 *  Copyright 2000 Simon White
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef SIDINFO_H
#define SIDINFO_H

#include <stdint.h>

#include "sidplayfp/siddefs.h"

/**
 * This interface is used to get sid engine informations.
 */
class SID_EXTERN SidInfo
{
public:
    /// Library name
    const char *name() const;

    /// Library version
    const char *version() const;

    /// Library credits
    //@{
    unsigned int numberOfCredits() const;
    const char *credits(unsigned int i) const;
    //@}

    /// Number of SIDs supported by this library
    unsigned int maxsids() const;

    /// Number of output channels (1-mono, 2-stereo)
    unsigned int channels() const;

    /// Address of the driver
    uint_least16_t driverAddr() const;

    /// Size of the driver in bytes
    uint_least16_t driverLength() const;

    /// \deprecated Power on delay
    SID_DEPRECATED uint_least16_t powerOnDelay() const;

    /// Describes the speed current song is running at
    const char *speedString() const;

    /// Description of the laoded ROM images
    //@{
    const char *kernalDesc() const;
    const char *basicDesc() const;
    const char *chargenDesc() const;
    //@}

private:
    virtual const char *getName() const =0;

    virtual const char *getVersion() const =0;

    virtual unsigned int getNumberOfCredits() const =0;
    virtual const char *getCredits(unsigned int i) const =0;

    virtual unsigned int getMaxsids() const =0;

    virtual unsigned int getChannels() const =0;

    virtual uint_least16_t getDriverAddr() const =0;

    virtual uint_least16_t getDriverLength() const =0;

    virtual const char *getSpeedString() const =0;

    virtual const char *getKernalDesc() const =0;
    virtual const char *getBasicDesc() const =0;
    virtual const char *getChargenDesc() const =0;

protected:
    ~SidInfo() {}
};

#endif  /* SIDINFO_H */
